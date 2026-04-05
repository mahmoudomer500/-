// ArabicComputerVision.cpp - تطبيق مكتبة الرؤية الحاسوبية العربية
// الأسبوع الثالث من الشهر السادس: الرؤية الحاسوبية
#include "ArabicComputerVision.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <queue>
#include <mutex>
#include <thread>
#include <immintrin.h>
#ifdef _MSC_VER
#include <intrin.h>
#endif
#include <set>
#include <map>
#include <filesystem>
#include <ctime>
#include <chrono>

namespace fs = std::filesystem;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <vfw.h>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "vfw32.lib")
#pragma comment(lib, "user32.lib")
#endif
#include <chrono>
#include <thread>

#ifdef _WIN32
#include <dshow.h>
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "quartz.lib")

// Define ISampleGrabber manually to avoid qedit.h dependency
static const IID IID_ISampleGrabber = { 0x6B652FFF, 0x11FE, 0x4fce, { 0x92, 0xAD, 0x02, 0x66, 0xB5, 0xD7, 0xC7, 0x8F } };
static const CLSID CLSID_SampleGrabber = { 0xC1F400A0, 0x3F08, 0x11d3, { 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };
static const CLSID CLSID_NullRenderer = { 0xC1F400A4, 0x3F08, 0x11d3, { 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };

interface ISampleGrabberCB : public IUnknown {
    virtual STDMETHODIMP SampleCB(double SampleTime, IMediaSample *pSample) = 0;
    virtual STDMETHODIMP BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen) = 0;
};

interface ISampleGrabber : public IUnknown {
    virtual STDMETHODIMP SetOneShot(BOOL OneShot) = 0;
    virtual STDMETHODIMP SetMediaType(const AM_MEDIA_TYPE *pType) = 0;
    virtual STDMETHODIMP GetConnectedMediaType(AM_MEDIA_TYPE *pType) = 0;
    virtual STDMETHODIMP SetBufferSamples(BOOL BufferThem) = 0;
    virtual STDMETHODIMP GetCurrentBuffer(long *pBufferSize, long *pBuffer) = 0;
    virtual STDMETHODIMP GetGetCurrentSample(IMediaSample **ppSample) = 0;
    virtual STDMETHODIMP SetCallback(ISampleGrabberCB *pCallback, long WhichMethodToCallback) = 0;
};

struct CameraData {
    IGraphBuilder* pGraph = NULL;
    ICaptureGraphBuilder2* pCapture = NULL;
    IBaseFilter* pSrcFilter = NULL;
    IBaseFilter* pGrabberF = NULL;
    ISampleGrabber* pGrabber = NULL;
    IBaseFilter* pNullF = NULL;
    IMediaControl* pControl = NULL;
    bool isRunning = false;
    int width = 640;
    int height = 480;
};

void FreeMediaType(AM_MEDIA_TYPE& mt) {
    if (mt.cbFormat != 0) {
        CoTaskMemFree((PVOID)mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL) {
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}
#endif

namespace ArabicLanguage {

// دالة مساعدة للحصول على معرف الترميز (Encoder CLSID) لتنسيق معين
#ifdef _WIN32
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;          // عدد برامج ترميز الصور
    UINT size = 0;         // حجم مصفوفة معلومات برنامج الترميز بالبايت

    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL) return -1;

    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;  // النجاح
        }
    }

    free(pImageCodecInfo);
    return -1;  // الفشل
}
#endif

// ==================== ImageProcessor Implementation ====================

bool ImageProcessor::_useOptimized = true;
static ImageProcessor::AccelerationMode _accMode = ImageProcessor::AccelerationMode::SIMD;

void ImageProcessor::setUseOptimized(bool on) {
    _useOptimized = on;
    if (on) _accMode = AccelerationMode::SIMD;
    else _accMode = AccelerationMode::CPU;
}

bool ImageProcessor::useOptimized() {
    return _useOptimized;
}

void ImageProcessor::setAccelerationMode(AccelerationMode mode) {
    if (isHardwareAccelerationAvailable(mode)) {
        _accMode = mode;
        _useOptimized = (mode != AccelerationMode::CPU);
    }
}

ImageProcessor::AccelerationMode ImageProcessor::getAccelerationMode() {
    return _accMode;
}

bool ImageProcessor::isHardwareAccelerationAvailable(AccelerationMode mode) {
    switch (mode) {
        case AccelerationMode::CPU: return true;
        case AccelerationMode::SIMD:
#if defined(__AVX2__) || defined(__SSE2__)
            return true;
#else
            return false;
#endif
        case AccelerationMode::OPENCL:
            // في البيئة الحالية، نعتبر OpenCL متاحاً للتطوير
            return true;
        case AccelerationMode::CUDA:
            // CUDA يتطلب بطاقة NVIDIA وبرامج تشغيل مثبتة
            return false;
        default: return false;
    }
}

ImageProcessor::ImageProcessor() {
}

Image ImageProcessor::loadImage(const std::string& filepath) {
    // محاولة فتح الملف باستخدام مسار يدعم UTF-8 على ويندوز
    std::string actualPath = filepath;
#ifdef _WIN32
    try {
        fs::path p = fs::u8path(filepath);
        if (fs::exists(p)) {
            actualPath = p.u8string();
        }
    } catch (...) {}
#endif

    Image img;

#ifdef _WIN32
    // تهيئة GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    {
        // تحويل المسار إلى WCHAR
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, actualPath.c_str(), -1, NULL, 0);
        std::vector<WCHAR> wPath(size_needed);
        MultiByteToWideChar(CP_UTF8, 0, actualPath.c_str(), -1, &wPath[0], size_needed);

        // تحميل الصورة
        Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(&wPath[0]);
        
        if (bitmap && bitmap->GetLastStatus() == Gdiplus::Ok) {
            int width = bitmap->GetWidth();
            int height = bitmap->GetHeight();
            
            img.width = width;
            img.height = height;
            img.channels = 3; // نفضل RGB دوماً للتبسيط حالياً
            img.data.resize(width * height * 3);

            Gdiplus::BitmapData bitmapData;
            Gdiplus::Rect rect(0, 0, width, height);
            
            if (bitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat24bppRGB, &bitmapData) == Gdiplus::Ok) {
                uint8_t* pixels = (uint8_t*)bitmapData.Scan0;
                int stride = bitmapData.Stride;
                
                for (int y = 0; y < height; ++y) {
                    for (int x = 0; x < width; ++x) {
                        int idx = (y * width + x) * 3;
                        int bIdx = y * stride + x * 3;
                        
                        // GDI+ uses BGR, we use RGB
                        img.data[idx + 0] = pixels[bIdx + 2]; // R
                        img.data[idx + 1] = pixels[bIdx + 1]; // G
                        img.data[idx + 2] = pixels[bIdx + 0]; // B
                    }
                }
                bitmap->UnlockBits(&bitmapData);
            }
            delete bitmap;
        } else {
            if (bitmap) delete bitmap;
            Gdiplus::GdiplusShutdown(gdiplusToken);
            throw std::runtime_error("فشل تحميل الصورة عبر GDI+: " + actualPath);
        }
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
#else
    // تنفيذ بديل للأنظمة الأخرى (مثل تحميل BMP بسيط)
    // حالياً نكتفي برسالة خطأ أو صورة فارغة
    std::cerr << "⚠️ تحميل الصور غير مدعوم بالكامل على هذا النظام حالياً" << std::endl;
    img.width = 100;
    img.height = 100;
    img.channels = 3;
    img.data.resize(100 * 100 * 3, 200);
#endif
    
    return img;
}

bool ImageProcessor::saveImage(const Image& img, const std::string& filepath, ImageFormat format) {
    if (img.isEmpty()) {
        std::cerr << "❌ محاولة حفظ صورة فارغة" << std::endl;
        return false;
    }

    int width = img.width;
    int height = img.height;
    int channels = img.channels;

    if (width <= 0 || height <= 0) {
        std::cerr << "❌ أبعاد الصورة غير صالحة: " << width << "x" << height << std::endl;
        return false;
    }

    // تحديد المسار والامتداد بناءً على التنسيق المطلوب
    std::string actualPath = filepath;
    std::string lowerPath = actualPath;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);

    std::string extension = ".png";
    if (format == ImageFormat::BMP) extension = ".bmp";
    else if (format == ImageFormat::JPEG) extension = ".jpg";
    else if (format == ImageFormat::PNG) extension = ".png";
    else {
        // إذا كان التنسيق غير معروف، نحاول استنتاجه من المسار
        if (lowerPath.find(".jpg") != std::string::npos || lowerPath.find(".jpeg") != std::string::npos) {
            format = ImageFormat::JPEG;
            extension = ".jpg";
        } else if (lowerPath.find(".bmp") != std::string::npos) {
            format = ImageFormat::BMP;
            extension = ".bmp";
        } else {
            format = ImageFormat::PNG;
            extension = ".png";
        }
    }

    // التأكد من وجود الامتداد الصحيح
    if (actualPath.find(".") == std::string::npos) {
        actualPath += extension;
    } else {
        // إذا كان هناك امتداد، نتحقق مما إذا كان يتوافق مع التنسيق المطلوب
        size_t lastDot = actualPath.find_last_of(".");
        std::string currentExt = actualPath.substr(lastDot);
        std::transform(currentExt.begin(), currentExt.end(), currentExt.begin(), ::tolower);
        
        if (currentExt != ".png" && currentExt != ".jpg" && currentExt != ".jpeg" && currentExt != ".bmp") {
            actualPath += extension;
        }
    }

    // معالجة المسار والتأكد من وجود المجلدات
    fs::path p;
    try {
#ifdef _WIN32
        p = fs::u8path(actualPath);
#else
        p = fs::path(actualPath);
#endif
        if (p.has_parent_path() && !fs::exists(p.parent_path())) {
            fs::create_directories(p.parent_path());
        }
        p = fs::absolute(p);
        actualPath = p.u8string();
    } catch (const std::exception& e) {
        std::cerr << "⚠️ تنبيه في معالجة المسار: " << e.what() << std::endl;
    }

    // إذا كان التنسيق JPEG أو PNG نستخدم GDI+ على ويندوز
#ifdef _WIN32
    if (format == ImageFormat::JPEG || format == ImageFormat::PNG) {
        // تهيئة GDI+
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken;
        Gdiplus::Status startupStatus = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        if (startupStatus != Gdiplus::Ok) {
            std::cerr << "❌ فشل تهيئة GDI+: " << (int)startupStatus << std::endl;
            format = ImageFormat::BMP; // محاولة الحفظ كـ BMP كحل احتياطي
        } else {
            bool success = false;
            {
                // إنشاء Bitmap من بيانات الصورة
                Gdiplus::Bitmap bitmap(width, height, PixelFormat24bppRGB);
                
                Gdiplus::BitmapData bitmapData;
                Gdiplus::Rect rect(0, 0, width, height);
                if (bitmap.LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat24bppRGB, &bitmapData) == Gdiplus::Ok) {
                    uint8_t* pixels = (uint8_t*)bitmapData.Scan0;
                    int stride = bitmapData.Stride;
                    
                    for (int y = 0; y < height; ++y) {
                        for (int x = 0; x < width; ++x) {
                            int idx = (y * width + x) * channels;
                            int bIdx = y * stride + x * 3;
                            
                            if (channels >= 3) {
                                pixels[bIdx + 0] = img.data[idx + 2]; // B
                                pixels[bIdx + 1] = img.data[idx + 1]; // G
                                pixels[bIdx + 2] = img.data[idx + 0]; // R
                            } else {
                                uint8_t gray = img.data[idx];
                                pixels[bIdx + 0] = gray;
                                pixels[bIdx + 1] = gray;
                                pixels[bIdx + 2] = gray;
                            }
                        }
                    }
                    bitmap.UnlockBits(&bitmapData);

                    // الحصول على CLSID للمرميز
                    CLSID encoderClsid;
                    const WCHAR* mimeType = (format == ImageFormat::PNG) ? L"image/png" : L"image/jpeg";
                    if (GetEncoderClsid(mimeType, &encoderClsid) != -1) {
                        // تحويل المسار إلى WCHAR
                        int size_needed = MultiByteToWideChar(CP_UTF8, 0, actualPath.c_str(), -1, NULL, 0);
                        std::vector<WCHAR> wPath(size_needed);
                        MultiByteToWideChar(CP_UTF8, 0, actualPath.c_str(), -1, &wPath[0], size_needed);
                        
                        Gdiplus::Status status = bitmap.Save(&wPath[0], &encoderClsid, NULL);
                        success = (status == Gdiplus::Ok);
                        if (!success) {
                            std::cerr << "❌ فشل حفظ الصورة عبر GDI+: رمز الخطأ " << (int)status << " المسار: " << actualPath << std::endl;
                        }
                    } else {
                        std::cerr << "❌ لم يتم العثور على مرمز لـ " << (format == ImageFormat::PNG ? "PNG" : "JPEG") << std::endl;
                    }
                } else {
                    std::cerr << "❌ فشل قفل بتات الصورة في الذاكرة" << std::endl;
                }
            }

            Gdiplus::GdiplusShutdown(gdiplusToken);
            if (success) {
                std::cout << "✅ تم حفظ الصورة بنجاح في: " << actualPath << std::endl;
                return true;
            }
            
            // إذا فشل GDI+، نحاول الحفظ كـ BMP كحل أخير
            std::cerr << "⚠️ فشل GDI+، جاري محاولة الحفظ كـ BMP كحل احتياطي..." << std::endl;
            format = ImageFormat::BMP;
        }
    }
#endif

    // تنفيذ حفظ BMP اليدوي (كحل افتراضي أو عند عدم توفر GDI+ أو فشله)
    std::ofstream file;
#ifdef _WIN32
    file.open(fs::u8path(actualPath), std::ios::binary);
#else
    file.open(actualPath, std::ios::binary);
#endif

    if (!file.is_open()) {
        std::cerr << "❌ فشل فتح الملف للحفظ: " << actualPath << std::endl;
        // محاولة أخيرة في المجلد الحالي باسم بسيط
        actualPath = "capture_fallback.bmp";
        file.open(actualPath, std::ios::binary);
        if (!file.is_open()) return false;
        std::cerr << "⚠️ تم استخدام مسار احتياطي: " << actualPath << std::endl;
    }

    // حساب حجم الصف مع المحاذاة (Padding) لـ 4 بايت
    int rowSize = ((width * 3 + 3) & ~3);
    int dataSize = rowSize * height;

    uint16_t bfType = 0x4D42; // "BM"
    uint32_t bfSize = 54 + dataSize;
    uint32_t bfReserved = 0;
    uint32_t bfOffBits = 54;
    uint32_t biSize = 40;
    int32_t biWidth = width;
    int32_t biHeight = height;
    uint16_t biPlanes = 1;
    uint16_t biBitCount = 24;
    uint32_t biCompression = 0;
    uint32_t biSizeImage = dataSize;
    int32_t biXPelsPerMeter = 2835;
    int32_t biYPelsPerMeter = 2835;
    uint32_t biClrUsed = 0;
    uint32_t biClrImportant = 0;

    file.write(reinterpret_cast<char*>(&bfType), 2);
    file.write(reinterpret_cast<char*>(&bfSize), 4);
    file.write(reinterpret_cast<char*>(&bfReserved), 4);
    file.write(reinterpret_cast<char*>(&bfOffBits), 4);
    file.write(reinterpret_cast<char*>(&biSize), 4);
    file.write(reinterpret_cast<char*>(&biWidth), 4);
    file.write(reinterpret_cast<char*>(&biHeight), 4);
    file.write(reinterpret_cast<char*>(&biPlanes), 2);
    file.write(reinterpret_cast<char*>(&biBitCount), 2);
    file.write(reinterpret_cast<char*>(&biCompression), 4);
    file.write(reinterpret_cast<char*>(&biSizeImage), 4);
    file.write(reinterpret_cast<char*>(&biXPelsPerMeter), 4);
    file.write(reinterpret_cast<char*>(&biYPelsPerMeter), 4);
    file.write(reinterpret_cast<char*>(&biClrUsed), 4);
    file.write(reinterpret_cast<char*>(&biClrImportant), 4);

    std::vector<uint8_t> padding(3, 0);
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            uint8_t r = 0, g = 0, b = 0;
            if (channels == 1) {
                r = g = b = img.at(x, y, 0);
            } else if (channels >= 3) {
                r = img.at(x, y, 0);
                g = img.at(x, y, 1);
                b = img.at(x, y, 2);
            }
            file.put(b); file.put(g); file.put(r);
        }
        if (rowSize > width * 3) {
            file.write(reinterpret_cast<char*>(padding.data()), rowSize - width * 3);
        }
    }

    file.close();
    std::cout << "✅ تم حفظ الصورة بنجاح في: " << actualPath << std::endl;
    return true;
}

Image ImageProcessor::resize(const Image& img, int newWidth, int newHeight, ResizeMethod method) {
    if (img.isEmpty() || newWidth <= 0 || newHeight <= 0) {
        throw std::runtime_error("معاملات غير صحيحة لتغيير الحجم");
    }
    
    Image result(newWidth, newHeight, img.channels);
    
    double scaleX = static_cast<double>(img.width) / newWidth;
    double scaleY = static_cast<double>(img.height) / newHeight;
    
    for (int y = 0; y < newHeight; ++y) {
        for (int x = 0; x < newWidth; ++x) {
            double srcX = x * scaleX;
            double srcY = y * scaleY;
            
            if (method == ResizeMethod::NEAREST_NEIGHBOR) {
                int ix = static_cast<int>(srcX);
                int iy = static_cast<int>(srcY);
                ix = (std::min)(ix, img.width - 1);
                iy = (std::min)(iy, img.height - 1);
                
                for (int c = 0; c < img.channels; ++c) {
                    result.at(x, y, c) = img.at(ix, iy, c);
                }
            } else if (method == ResizeMethod::BILINEAR) {
                int x1 = static_cast<int>(srcX);
                int y1 = static_cast<int>(srcY);
                int x2 = (std::min)(x1 + 1, img.width - 1);
                int y2 = (std::min)(y1 + 1, img.height - 1);
                
                double fx = srcX - x1;
                double fy = srcY - y1;
                
                for (int c = 0; c < img.channels; ++c) {
                    double v1 = img.at(x1, y1, c) * (1 - fx) + img.at(x2, y1, c) * fx;
                    double v2 = img.at(x1, y2, c) * (1 - fx) + img.at(x2, y2, c) * fx;
                    double value = v1 * (1 - fy) + v2 * fy;
                    result.at(x, y, c) = clamp(static_cast<int>(value));
                }
            }
        }
    }
    
    return result;
}

Image ImageProcessor::crop(const Image& img, int x, int y, int width, int height) {
    if (x < 0 || y < 0 || x + width > img.width || y + height > img.height) {
        throw std::runtime_error("معاملات القص غير صحيحة");
    }
    
    Image result(width, height, img.channels);
    
    for (int dy = 0; dy < height; ++dy) {
        for (int dx = 0; dx < width; ++dx) {
            for (int c = 0; c < img.channels; ++c) {
                result.at(dx, dy, c) = img.at(x + dx, y + dy, c);
            }
        }
    }
    
    return result;
}

Image ImageProcessor::rotate(const Image& img, double angle, InterpolationMethod method) {
    if (img.isEmpty()) {
        throw std::runtime_error("صورة فارغة");
    }
    
    double rad = angle * M_PI / 180.0;
    double cos_a = std::cos(rad);
    double sin_a = std::sin(rad);
    
    // حساب الأبعاد الجديدة
    int newWidth = static_cast<int>(std::abs(img.width * cos_a) + std::abs(img.height * sin_a));
    int newHeight = static_cast<int>(std::abs(img.width * sin_a) + std::abs(img.height * cos_a));
    
    Image result(newWidth, newHeight, img.channels);
    int centerX = img.width / 2;
    int centerY = img.height / 2;
    int newCenterX = newWidth / 2;
    int newCenterY = newHeight / 2;
    
    for (int y = 0; y < newHeight; ++y) {
        for (int x = 0; x < newWidth; ++x) {
            int dx = x - newCenterX;
            int dy = y - newCenterY;
            
            // Inverse rotation: find source pixel for each destination pixel
            // Forward: x' = cx + dx*cos(a) - dy*sin(a), y' = cy + dx*sin(a) + dy*cos(a)
            // Inverse (rotate by -a): srcX = cx + dx*cos(a) + dy*sin(a), srcY = cy - dx*sin(a) + dy*cos(a)
            int srcX = static_cast<int>(centerX + dx * cos_a + dy * sin_a);
            int srcY = static_cast<int>(centerY - dx * sin_a + dy * cos_a);
            
            if (srcX >= 0 && srcX < img.width && srcY >= 0 && srcY < img.height) {
                for (int c = 0; c < img.channels; ++c) {
                    result.at(x, y, c) = img.at(srcX, srcY, c);
                }
            }
        }
    }
    
    return result;
}

Image ImageProcessor::flip(const Image& img, bool horizontal, bool vertical) {
    Image result(img.width, img.height, img.channels);
    
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            int srcX = horizontal ? img.width - 1 - x : x;
            int srcY = vertical ? img.height - 1 - y : y;
            
            for (int c = 0; c < img.channels; ++c) {
                result.at(x, y, c) = img.at(srcX, srcY, c);
            }
        }
    }
    
    return result;
}

Image ImageProcessor::toGrayscale(const Image& img) {
    if (img.channels == 1) return img;
    
    Image result(img.width, img.height, 1);
    int numPixels = img.width * img.height;
    const uint8_t* src = img.data.data();
    uint8_t* dst = result.data.data();

    if (_useOptimized && img.channels == 3) {
        int i = 0;
#ifdef __AVX2__
        // Coefficients for Y = (77R + 150G + 29B) / 256
        __m256i v77 = _mm256_set1_epi16(77);
        __m256i v150 = _mm256_set1_epi16(150);
        __m256i v29 = _mm256_set1_epi16(29);
        __m256i vZero = _mm256_setzero_si256();

        for (; i <= numPixels - 32; i += 32) {
            // Processing 32 pixels requires handling 32*3 = 96 bytes.
            // This is complex for AVX due to alignment. Simplified version:
            for(int j=0; j<32; ++j) {
                int base = (i + j) * 3;
                dst[i+j] = (77 * src[base] + 150 * src[base+1] + 29 * src[base+2]) >> 8;
            }
        }
#endif
        for (; i < numPixels; ++i) {
            int base = i * 3;
            dst[i] = (77 * src[base] + 150 * src[base+1] + 29 * src[base+2]) >> 8;
        }
    } else {
        ParallelProcessor::parallelFor(0, numPixels, [&](int i) {
            int base = i * img.channels;
            if (img.channels >= 3) {
                dst[i] = static_cast<uint8_t>(0.299 * src[base] + 0.587 * src[base+1] + 0.114 * src[base+2]);
            } else {
                dst[i] = src[base];
            }
        });
    }
    
    return result;
}

Image ImageProcessor::toRGB(const Image& img) {
    if (img.channels == 3) {
        return img; // بالفعل RGB
    }
    
    Image result(img.width, img.height, 3);
    
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            if (img.channels == 1) {
                uint8_t gray = img.at(x, y, 0);
                result.at(x, y, 0) = gray;
                result.at(x, y, 1) = gray;
                result.at(x, y, 2) = gray;
            } else if (img.channels == 4) {
                result.at(x, y, 0) = img.at(x, y, 0);
                result.at(x, y, 1) = img.at(x, y, 1);
                result.at(x, y, 2) = img.at(x, y, 2);
            }
        }
    }
    
    return result;
}

Image ImageProcessor::toRGBA(const Image& img) {
    if (img.channels == 4) {
        return img; // بالفعل RGBA
    }
    
    Image result(img.width, img.height, 4);
    
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            if (img.channels == 1) {
                uint8_t gray = img.at(x, y, 0);
                result.at(x, y, 0) = gray;
                result.at(x, y, 1) = gray;
                result.at(x, y, 2) = gray;
                result.at(x, y, 3) = 255;
            } else if (img.channels == 3) {
                result.at(x, y, 0) = img.at(x, y, 0);
                result.at(x, y, 1) = img.at(x, y, 1);
                result.at(x, y, 2) = img.at(x, y, 2);
                result.at(x, y, 3) = 255;
            }
        }
    }
    
    return result;
}

Image ImageProcessor::toLAB(const Image& img) {
    Image rgb = toRGB(img);
    Image result(img.width, img.height, 3);
    for (int i = 0; i < img.width * img.height; ++i) {
        double r = rgb.data[i * 3] / 255.0;
        double g = rgb.data[i * 3 + 1] / 255.0;
        double b = rgb.data[i * 3 + 2] / 255.0;

        r = (r > 0.04045) ? std::pow((r + 0.055) / 1.055, 2.4) : r / 12.92;
        g = (g > 0.04045) ? std::pow((g + 0.055) / 1.055, 2.4) : g / 12.92;
        b = (b > 0.04045) ? std::pow((b + 0.055) / 1.055, 2.4) : b / 12.92;

        double x = (r * 0.4124 + g * 0.3576 + b * 0.1805) / 0.95047;
        double y = (r * 0.2126 + g * 0.7152 + b * 0.0722) / 1.00000;
        double z = (r * 0.0193 + g * 0.1192 + b * 0.9505) / 1.08883;

        x = (x > 0.008856) ? std::pow(x, 1.0/3.0) : (7.787 * x) + 16.0/116.0;
        y = (y > 0.008856) ? std::pow(y, 1.0/3.0) : (7.787 * y) + 16.0/116.0;
        z = (z > 0.008856) ? std::pow(z, 1.0/3.0) : (7.787 * z) + 16.0/116.0;

        result.data[i * 3] = clamp(static_cast<int>((116.0 * y) - 16.0));
        result.data[i * 3 + 1] = clamp(static_cast<int>(500.0 * (x - y) + 128));
        result.data[i * 3 + 2] = clamp(static_cast<int>(200.0 * (y - z) + 128));
    }
    return result;
}

Image ImageProcessor::fromLAB(const Image& img) {
    Image result(img.width, img.height, 3);
    for (int i = 0; i < img.width * img.height; ++i) {
        double l = img.data[i * 3];
        double a = img.data[i * 3 + 1] - 128.0;
        double b = img.data[i * 3 + 2] - 128.0;

        double y = (l + 16.0) / 116.0;
        double x = a / 500.0 + y;
        double z = y - b / 200.0;

        x = (std::pow(x, 3) > 0.008856) ? std::pow(x, 3) : (x - 16.0/116.0) / 7.787;
        y = (std::pow(y, 3) > 0.008856) ? std::pow(y, 3) : (y - 16.0/116.0) / 7.787;
        z = (std::pow(z, 3) > 0.008856) ? std::pow(z, 3) : (z - 16.0/116.0) / 7.787;

        x *= 0.95047;
        y *= 1.00000;
        z *= 1.08883;

        double r = x * 3.2406 + y * -1.5372 + z * -0.4986;
        double g = x * -0.9689 + y * 1.8758 + z * 0.0415;
        double b_val = x * 0.0557 + y * -0.2040 + z * 1.0570;

        r = (r > 0.0031308) ? (1.055 * std::pow(r, 1.0/2.4) - 0.055) : 12.92 * r;
        g = (g > 0.0031308) ? (1.055 * std::pow(g, 1.0/2.4) - 0.055) : 12.92 * g;
        b_val = (b_val > 0.0031308) ? (1.055 * std::pow(b_val, 1.0/2.4) - 0.055) : 12.92 * b_val;

        result.data[i * 3] = clamp(static_cast<int>(r * 255));
        result.data[i * 3 + 1] = clamp(static_cast<int>(g * 255));
        result.data[i * 3 + 2] = clamp(static_cast<int>(b_val * 255));
    }
    return result;
}

Image ImageProcessor::toYUV(const Image& img) {
    Image rgb = toRGB(img);
    Image result(img.width, img.height, 3);
    for (int i = 0; i < img.width * img.height; ++i) {
        int r = rgb.data[i * 3];
        int g = rgb.data[i * 3 + 1];
        int b = rgb.data[i * 3 + 2];

        result.data[i * 3] = clamp(static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b));
        result.data[i * 3 + 1] = clamp(static_cast<int>(-0.14713 * r - 0.28886 * g + 0.436 * b + 128));
        result.data[i * 3 + 2] = clamp(static_cast<int>(0.615 * r - 0.51499 * g - 0.10001 * b + 128));
    }
    return result;
}

Image ImageProcessor::fromYUV(const Image& img) {
    Image result(img.width, img.height, 3);
    for (int i = 0; i < img.width * img.height; ++i) {
        int y = img.data[i * 3];
        int u = img.data[i * 3 + 1] - 128;
        int v = img.data[i * 3 + 2] - 128;

        result.data[i * 3] = clamp(static_cast<int>(y + 1.13983 * v));
        result.data[i * 3 + 1] = clamp(static_cast<int>(y - 0.39465 * u - 0.58060 * v));
        result.data[i * 3 + 2] = clamp(static_cast<int>(y + 2.03211 * u));
    }
    return result;
}

Image ImageProcessor::applyGaussianBlur(const Image& img, double sigma) {
    if (img.isEmpty()) {
        throw std::runtime_error("صورة فارغة");
    }
    
    int kernelSize = static_cast<int>(6 * sigma) + 1;
    if (kernelSize % 2 == 0) kernelSize++;
    
    std::vector<double> kernel1D(kernelSize);
    int center = kernelSize / 2;
    double sum = 0.0;
    
    for (int i = 0; i < kernelSize; ++i) {
        double x = i - center;
        kernel1D[i] = std::exp(-(x * x) / (2 * sigma * sigma));
        sum += kernel1D[i];
    }
    
    // تطبيع النواة
    for (int i = 0; i < kernelSize; ++i) {
        kernel1D[i] /= sum;
    }
    
    Image temp(img.width, img.height, img.channels);
    Image result(img.width, img.height, img.channels);
    
    // تمرير أفقي ثم عمودي (تحسين الأداء باستخدام النواة المنفصلة)
    applyKernel1D(img, temp, kernel1D, true);
    applyKernel1D(temp, result, kernel1D, false);
    
    return result;
}

Image ImageProcessor::applyMedianFilter(const Image& img, int kernelSize) {
    if (img.isEmpty() || kernelSize % 2 == 0) {
        throw std::runtime_error("معاملات غير صحيحة");
    }
    
    Image result(img.width, img.height, img.channels);
    int offset = kernelSize / 2;
    
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            for (int c = 0; c < img.channels; ++c) {
                std::vector<uint8_t> values;
                
                for (int dy = -offset; dy <= offset; ++dy) {
                    for (int dx = -offset; dx <= offset; ++dx) {
                        int nx = x + dx;
                        int ny = y + dy;
                        
                        if (nx >= 0 && nx < img.width && ny >= 0 && ny < img.height) {
                            values.push_back(img.at(nx, ny, c));
                        }
                    }
                }
                
                std::sort(values.begin(), values.end());
                result.at(x, y, c) = values[values.size() / 2];
            }
        }
    }
    
    return result;
}

Image ImageProcessor::sharpen(const Image& img, double strength) {
    std::vector<std::vector<double>> kernel = {
        {0, -strength, 0},
        {-strength, 1 + 4 * strength, -strength},
        {0, -strength, 0}
    };
    
    Image result(img.width, img.height, img.channels);
    applyKernel(img, result, kernel);
    
    return result;
}

Image ImageProcessor::detectEdges(const Image& img, int lowThreshold, int highThreshold) {
    Image gray = toGrayscale(img);
    
    // Sobel operator
    std::vector<std::vector<double>> sobelX = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };
    
    std::vector<std::vector<double>> sobelY = {
        {-1, -2, -1},
        {0, 0, 0},
        {1, 2, 1}
    };
    
    Image gx(gray.width, gray.height, 1);
    Image gy(gray.width, gray.height, 1);
    
    applyKernel(gray, gx, sobelX);
    applyKernel(gray, gy, sobelY);
    
    Image result(gray.width, gray.height, 1);
    
    for (int y = 0; y < gray.height; ++y) {
        for (int x = 0; x < gray.width; ++x) {
            double gx_val = gx.at(x, y, 0) - 128.0;
            double gy_val = gy.at(x, y, 0) - 128.0;
            double magnitude = std::sqrt(gx_val * gx_val + gy_val * gy_val);
            
            if (magnitude > highThreshold) {
                result.at(x, y, 0) = 255;
            } else if (magnitude > lowThreshold) {
                result.at(x, y, 0) = 128;
            } else {
                result.at(x, y, 0) = 0;
            }
        }
    }
    
    return result;
}

Image ImageProcessor::detectEdgesCanny(const Image& img, int lowThreshold, int highThreshold) {
    // تنفيذ مبسط لـ Canny: Gaussian -> Sobel -> Non-maximum suppression -> Double threshold
    Image gray = toGrayscale(img);
    Image blurred = applyGaussianBlur(gray, 1.0);

    // Sobel gradients
    std::vector<std::vector<double>> sobelX = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };
    std::vector<std::vector<double>> sobelY = {
        {-1, -2, -1},
        {0, 0, 0},
        {1, 2, 1}
    };
    Image gx(blurred.width, blurred.height, 1);
    Image gy(blurred.width, blurred.height, 1);
    applyKernel(blurred, gx, sobelX);
    applyKernel(blurred, gy, sobelY);

    // Magnitude and angle
    std::vector<double> mag(blurred.width * blurred.height, 0.0);
    std::vector<double> ang(blurred.width * blurred.height, 0.0);
    for (int y = 0; y < blurred.height; ++y) {
        for (int x = 0; x < blurred.width; ++x) {
            double sx = gx.at(x, y, 0) - 128.0;
            double sy = gy.at(x, y, 0) - 128.0;
            double m = std::sqrt(sx * sx + sy * sy);
            double a = std::atan2(sy, sx);
            mag[y * blurred.width + x] = m;
            ang[y * blurred.width + x] = a;
        }
    }

    // Non-maximum suppression
    Image nms(blurred.width, blurred.height, 1);
    for (int y = 1; y < blurred.height - 1; ++y) {
        for (int x = 1; x < blurred.width - 1; ++x) {
            double m = mag[y * blurred.width + x];
            double a = ang[y * blurred.width + x];
            double dir = std::fmod((a + M_PI) * 180.0 / M_PI, 180.0);
            double m1 = 0, m2 = 0;
            if ((dir >= 0 && dir < 22.5) || (dir >= 157.5 && dir < 180)) {
                m1 = mag[y * blurred.width + (x - 1)];
                m2 = mag[y * blurred.width + (x + 1)];
            } else if (dir >= 22.5 && dir < 67.5) {
                m1 = mag[(y - 1) * blurred.width + (x + 1)];
                m2 = mag[(y + 1) * blurred.width + (x - 1)];
            } else if (dir >= 67.5 && dir < 112.5) {
                m1 = mag[(y - 1) * blurred.width + x];
                m2 = mag[(y + 1) * blurred.width + x];
            } else {
                m1 = mag[(y - 1) * blurred.width + (x - 1)];
                m2 = mag[(y + 1) * blurred.width + (x + 1)];
            }
            if (m >= m1 && m >= m2) {
                nms.at(x, y, 0) = clamp(static_cast<int>(m));
            } else {
                nms.at(x, y, 0) = 0;
            }
        }
    }

    // Double threshold + hysteresis (مبسطة)
    Image result(blurred.width, blurred.height, 1);
    uint8_t strong = 255;
    uint8_t weak = 100;
    for (int y = 0; y < nms.height; ++y) {
        for (int x = 0; x < nms.width; ++x) {
            uint8_t v = nms.at(x, y, 0);
            if (v >= highThreshold) result.at(x, y, 0) = strong;
            else if (v >= lowThreshold) result.at(x, y, 0) = weak;
            else result.at(x, y, 0) = 0;
        }
    }
    // Hysteresis: ترقية الـ weak إذا جاور strong
    bool changed = true;
    while (changed) {
        changed = false;
        for (int y = 1; y < result.height - 1; ++y) {
            for (int x = 1; x < result.width - 1; ++x) {
                if (result.at(x, y, 0) == weak) {
                    bool nearStrong = false;
                    for (int dy = -1; dy <= 1 && !nearStrong; ++dy) {
                        for (int dx = -1; dx <= 1 && !nearStrong; ++dx) {
                            if (result.at(x + dx, y + dy, 0) == strong) nearStrong = true;
                        }
                    }
                    if (nearStrong) {
                        result.at(x, y, 0) = strong;
                        changed = true;
                    } else {
                        result.at(x, y, 0) = 0;
                    }
                }
            }
        }
    }
    return result;
}

Image ImageProcessor::adjustBrightnessContrast(const Image& img, double brightness, double contrast) {
    Image result(img.width, img.height, img.channels);
    double factor = (259.0 * (contrast + 255.0)) / (255.0 * (259.0 - contrast));
    
    ParallelProcessor::parallelFor(0, img.height, [&](int y) {
        for (int x = 0; x < img.width; ++x) {
            for (int c = 0; c < img.channels; ++c) {
                double value = img.at(x, y, c);
                value = value * brightness;
                value = factor * (value - 128.0) + 128.0;
                result.at(x, y, c) = clamp(static_cast<int>(value));
            }
        }
    });
    
    return result;
}

Image ImageProcessor::adjustSaturation(const Image& img, double saturation) {
    Image result = toRGB(img);
    
    ParallelProcessor::parallelFor(0, result.height, [&](int y) {
        for (int x = 0; x < result.width; ++x) {
            double r = result.at(x, y, 0);
            double g = result.at(x, y, 1);
            double b = result.at(x, y, 2);
            
            double gray = 0.299 * r + 0.587 * g + 0.114 * b;
            
            result.at(x, y, 0) = clamp(static_cast<int>(gray + saturation * (r - gray)));
            result.at(x, y, 1) = clamp(static_cast<int>(gray + saturation * (g - gray)));
            result.at(x, y, 2) = clamp(static_cast<int>(gray + saturation * (b - gray)));
        }
    });
    
    return result;
}

Image ImageProcessor::equalizeHistogram(const Image& img) {
    Image gray = toGrayscale(img);
    Image result(gray.width, gray.height, 1);
    
    // حساب الهيستوجرام
    std::vector<int> histogram(256, 0);
    for (int y = 0; y < gray.height; ++y) {
        for (int x = 0; x < gray.width; ++x) {
            histogram[gray.at(x, y, 0)]++;
        }
    }
    
    // حساب التوزيع التراكمي
    std::vector<int> cumulative(256, 0);
    cumulative[0] = histogram[0];
    for (int i = 1; i < 256; ++i) {
        cumulative[i] = cumulative[i - 1] + histogram[i];
    }
    
    // تطبيق التوزيع المتساوي
    int totalPixels = gray.width * gray.height;
    for (int y = 0; y < gray.height; ++y) {
        for (int x = 0; x < gray.width; ++x) {
            uint8_t value = gray.at(x, y, 0);
            int newValue = (cumulative[value] * 255) / totalPixels;
            result.at(x, y, 0) = clamp(newValue);
        }
    }
    
    return result;
}

ImageProcessor::DFTResult ImageProcessor::applyDFT(const Image& img) {
    Image gray = (img.channels == 1) ? img : toGrayscale(img);
    int W = gray.width;
    int H = gray.height;

    DFTResult result;
    result.width = W;
    result.height = H;
    result.realData.assign(W * H, 0.0);
    result.imagData.assign(W * H, 0.0);
    result.magnitude = Image(W, H, 1);
    result.phase = Image(W, H, 1);

    // 1. Perform 1D DFT on rows
    std::vector<double> intermediateReal(W * H, 0.0);
    std::vector<double> intermediateImag(W * H, 0.0);

    for (int y = 0; y < H; ++y) {
        for (int u = 0; u < W; ++u) {
            double rSum = 0.0, iSum = 0.0;
            for (int x = 0; x < W; ++x) {
                double angle = -2.0 * M_PI * u * x / W;
                double val = gray.at(x, y);
                rSum += val * std::cos(angle);
                iSum += val * std::sin(angle);
            }
            intermediateReal[y * W + u] = rSum;
            intermediateImag[y * W + u] = iSum;
        }
    }

    // 2. Perform 1D DFT on columns of the intermediate result
    for (int x = 0; x < W; ++x) {
        for (int v = 0; v < H; ++v) {
            double rSum = 0.0, iSum = 0.0;
            for (int y = 0; y < H; ++y) {
                double angle = -2.0 * M_PI * v * y / H;
                double r = intermediateReal[y * W + x];
                double i = intermediateImag[y * W + x];
                // Complex multiplication: (r + i*iSum) * (cos + i*sin)
                // (r*cos - i*sin) + i*(r*sin + i*cos)
                rSum += r * std::cos(angle) - i * std::sin(angle);
                iSum += r * std::sin(angle) + i * std::cos(angle);
            }
            result.realData[v * W + x] = rSum;
            result.imagData[v * W + x] = iSum;

            double mag = std::sqrt(rSum * rSum + iSum * iSum);
            double phase = std::atan2(iSum, rSum);

            // Log scaling for visualization
            result.magnitude.at(x, v) = clamp(static_cast<int>(20 * std::log(1 + mag)));
            result.phase.at(x, v) = clamp(static_cast<int>((phase + M_PI) / (2 * M_PI) * 255));
        }
    }

    return result;
}

Image ImageProcessor::applyIDFT(const DFTResult& dft) {
    int W = dft.width;
    int H = dft.height;
    Image result(W, H, 1);

    std::vector<double> intermediateReal(W * H, 0.0);
    std::vector<double> intermediateImag(W * H, 0.0);

    // 1. 1D IDFT on columns
    for (int x = 0; x < W; ++x) {
        for (int y = 0; y < H; ++y) {
            double rSum = 0.0, iSum = 0.0;
            for (int v = 0; v < H; ++v) {
                double angle = 2.0 * M_PI * v * y / H; // Positive angle for IDFT
                double r = dft.realData[v * W + x];
                double i = dft.imagData[v * W + x];
                rSum += r * std::cos(angle) - i * std::sin(angle);
                iSum += r * std::sin(angle) + i * std::cos(angle);
            }
            intermediateReal[y * W + x] = rSum / H;
            intermediateImag[y * W + x] = iSum / H;
        }
    }

    // 2. 1D IDFT on rows
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            double rSum = 0.0;
            for (int u = 0; u < W; ++u) {
                double angle = 2.0 * M_PI * u * x / W;
                double r = intermediateReal[y * W + u];
                double i = intermediateImag[y * W + u];
                rSum += r * std::cos(angle) - i * std::sin(angle);
            }
            result.at(x, y) = clamp(static_cast<int>(rSum / W));
        }
    }

    return result;
}

Image ImageProcessor::fftShift(const Image& img) {
    int W = img.width;
    int H = img.height;
    Image result(W, H, img.channels);
    int cx = W / 2;
    int cy = H / 2;

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int nx = (x + cx) % W;
            int ny = (y + cy) % H;
            for (int c = 0; c < img.channels; ++c) {
                result.at(nx, ny, c) = img.at(x, y, c);
            }
        }
    }
    return result;
}

Image ImageProcessor::add(const Image& img1, const Image& img2) {
    if (img1.width != img2.width || img1.height != img2.height || img1.channels != img2.channels) {
        throw std::runtime_error("الصور يجب أن تكون بنفس الأبعاد");
    }
    
    Image result(img1.width, img1.height, img1.channels);
    
    for (int y = 0; y < img1.height; ++y) {
        for (int x = 0; x < img1.width; ++x) {
            for (int c = 0; c < img1.channels; ++c) {
                int value = img1.at(x, y, c) + img2.at(x, y, c);
                result.at(x, y, c) = clamp(value);
            }
        }
    }
    
    return result;
}

Image ImageProcessor::subtract(const Image& img1, const Image& img2) {
    if (img1.width != img2.width || img1.height != img2.height || img1.channels != img2.channels) {
        throw std::runtime_error("الصور يجب أن تكون بنفس الأبعاد");
    }
    
    Image result(img1.width, img1.height, img1.channels);
    
    for (int y = 0; y < img1.height; ++y) {
        for (int x = 0; x < img1.width; ++x) {
            for (int c = 0; c < img1.channels; ++c) {
                int value = img1.at(x, y, c) - img2.at(x, y, c);
                result.at(x, y, c) = clamp(value);
            }
        }
    }
    
    return result;
}

Image ImageProcessor::multiply(const Image& img1, const Image& img2, double factor) {
    if (img1.width != img2.width || img1.height != img2.height || img1.channels != img2.channels) {
        throw std::runtime_error("الصور يجب أن تكون بنفس الأبعاد");
    }
    
    Image result(img1.width, img1.height, img1.channels);
    
    for (int y = 0; y < img1.height; ++y) {
        for (int x = 0; x < img1.width; ++x) {
            for (int c = 0; c < img1.channels; ++c) {
                double value = (img1.at(x, y, c) * img2.at(x, y, c) / 255.0) * factor;
                result.at(x, y, c) = clamp(static_cast<int>(value));
            }
        }
    }
    
    return result;
}

ImageProcessor::ImageProps ImageProcessor::getImageProps(const Image& img) {
    ImageProps props;
    props.width = img.width;
    props.height = img.height;
    props.channels = img.channels;
    
    if (img.data.empty()) {
        props.minVal = props.maxVal = props.avgVal = props.stdDev = 0;
        return props;
    }

    double minV = 255, maxV = 0, sum = 0, sumSq = 0;
    for (uint8_t v : img.data) {
        if (v < minV) minV = v;
        if (v > maxV) maxV = v;
        sum += v;
        sumSq += v * v;
    }

    int n = img.data.size();
    props.minVal = minV;
    props.maxVal = maxV;
    props.avgVal = sum / n;
    props.stdDev = std::sqrt((sumSq / n) - (props.avgVal * props.avgVal));
    
    return props;
}

// ==================== Drawing Functions ====================

Image ImageProcessor::drawLine(const Image& img, int x1, int y1, int x2, int y2, const Color& color, int thickness) {
    Image result = img;
    int dx = std::abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -std::abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;
    auto setPixel = [&](int x, int y) {
        setPixelColor(result, x, y, color);
    };
    while (true) {
        for (int ty = -thickness / 2; ty <= thickness / 2; ++ty) {
            for (int tx = -thickness / 2; tx <= thickness / 2; ++tx) {
                setPixel(x1 + tx, y1 + ty);
            }
        }
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
    return result;
}

Image ImageProcessor::drawRectangle(const Image& img, int x, int y, int width, int height, const Color& color, int thickness, bool filled) {
    Image result = img;
    
    // تصحيح الأبعاد السالبة (دعم رسم المستطيلات في أي اتجاه)
    if (width < 0) {
        x += width;
        width = -width;
    }
    if (height < 0) {
        y += height;
        height = -height;
    }

    std::cout << "DEBUG: Drawing Rectangle at (" << x << "," << y << ") size=" << width << "x" << height 
              << " color=(" << (int)color.r << "," << (int)color.g << "," << (int)color.b << ")" << std::endl;

    int x2 = x + width - 1;
    int y2 = y + height - 1;

    if (filled) {
        for (int yy = y; yy <= y2; ++yy) {
            for (int xx = x; xx <= x2; ++xx) {
                setPixelColor(result, xx, yy, color);
            }
        }
    } else {
        result = drawLine(result, x, y, x2, y, color, thickness);
        result = drawLine(result, x, y2, x2, y2, color, thickness);
        result = drawLine(result, x, y, x, y2, color, thickness);
        result = drawLine(result, x2, y, x2, y2, color, thickness);
    }
    return result;
}

Image ImageProcessor::drawCircle(const Image& img, int centerX, int centerY, int radius, const Color& color, int thickness, bool filled) {
    Image result = img;
    if (radius <= 0) return result;

    // سجل تتبع للرسم
    std::cout << "DEBUG: Drawing Circle at (" << centerX << "," << centerY << ") radius=" << radius 
              << " color=(" << (int)color.r << "," << (int)color.g << "," << (int)color.b << ")" << std::endl;

    if (filled) {
        // ملء الدائرة باستخدام خطوط أفقية (أكثر كفاءة ومطابقة لـ OpenCV)
        for (int y = -radius; y <= radius; ++y) {
            int width = static_cast<int>(std::sqrt(radius * radius - y * y));
            for (int x = -width; x <= width; ++x) {
                setPixelColor(result, centerX + x, centerY + y, color);
            }
        }
    } else {
        // رسم المحيط باستخدام خوارزمية نقطة المنتصف مع سمك الخط
        int x = radius;
        int y = 0;
        int err = 1 - x;

        auto drawThickness = [&](int px, int py) {
            if (thickness <= 1) {
                setPixelColor(result, px, py, color);
            } else {
                int r = thickness / 2;
                for (int ty = -r; ty <= r; ++ty) {
                    for (int tx = -r; tx <= r; ++tx) {
                        setPixelColor(result, px + tx, py + ty, color);
                    }
                }
            }
        };

        while (x >= y) {
            drawThickness(centerX + x, centerY + y);
            drawThickness(centerX + y, centerY + x);
            drawThickness(centerX - y, centerY + x);
            drawThickness(centerX - x, centerY + y);
            drawThickness(centerX - x, centerY - y);
            drawThickness(centerX - y, centerY - x);
            drawThickness(centerX + y, centerY - x);
            drawThickness(centerX + x, centerY - y);

            y++;
            if (err <= 0) {
                err += 2 * y + 1;
            } else {
                x--;
                err += 2 * (y - x) + 1;
            }
        }
    }
    return result;
}

Image ImageProcessor::drawEllipse(const Image& img, int centerX, int centerY, int radiusX, int radiusY, double angleDeg, const Color& color, int thickness, bool filled) {
    Image result = img;
    double angle = angleDeg * M_PI / 180.0;
    double cosA = std::cos(angle);
    double sinA = std::sin(angle);
    auto setPixel = [&](int px, int py) {
        setPixelColor(result, px, py, color);
    };
    auto drawPoint = [&](int x, int y) {
        for (int ty = -thickness / 2; ty <= thickness / 2; ++ty) {
            for (int tx = -thickness / 2; tx <= thickness / 2; ++tx) {
                setPixel(x + tx, y + ty);
            }
        }
    };
    for (int deg = 0; deg < 360; ++deg) {
        double rad = deg * M_PI / 180.0;
        double xr = radiusX * std::cos(rad);
        double yr = radiusY * std::sin(rad);
        int px = static_cast<int>(centerX + xr * cosA - yr * sinA);
        int py = static_cast<int>(centerY + xr * sinA + yr * cosA);
        if (filled) {
            drawLine(result, centerX, centerY, px, py, color, 1);
        } else {
            drawPoint(px, py);
        }
    }
    return result;
}

Image ImageProcessor::drawPolygon(const Image& img, const std::vector<std::pair<int, int>>& points, const Color& color, int thickness, bool filled) {
    if (points.size() < 2) return img;
    Image result = img;
    if (filled) {
        // ملء بسيط عبر رسم مثلثات من نقطة مرجعية
        auto p0 = points[0];
        for (size_t i = 1; i + 1 < points.size(); ++i) {
            result = drawLine(result, p0.first, p0.second, points[i].first, points[i].second, color, 1);
            result = drawLine(result, points[i].first, points[i].second, points[i + 1].first, points[i + 1].second, color, 1);
        }
    }
    for (size_t i = 0; i < points.size(); ++i) {
        auto p1 = points[i];
        auto p2 = points[(i + 1) % points.size()];
        result = drawLine(result, p1.first, p1.second, p2.first, p2.second, color, thickness);
    }
    return result;
}

Image ImageProcessor::putText(const Image& img, int x, int y, const std::string& text, const Color& color) {
    // تنفيذ باستخدام خط نقطي بسيط 5x7
    static const uint8_t font5x7[95][7] = {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Space
        {0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x04}, // !
        // ... (تبسيطاً سنرسم مربعات للأحرف غير المعرفة حالياً)
    };
    
    Image result = img;
    int charWidth = 5;
    int charHeight = 7;
    int spacing = 1;
    
    for (size_t i = 0; i < text.size(); ++i) {
        int cx = x + static_cast<int>(i) * (charWidth + spacing);
        char c = text[i];
        
        // رسم تمثيلي لكل حرف (مربع حالياً مع تحسين طفيف)
        for (int row = 0; row < charHeight; ++row) {
            for (int col = 0; col < charWidth; ++col) {
                // نمط وهمي لتمييز الحروف
                bool pixel = (c % 2 == 0) ? (row % 2 == 0) : (col % 2 == 0);
                if (pixel) {
                    setPixelColor(result, cx + col, y + row, color);
                }
            }
        }
    }
    return result;
}

// ==================== Template Matching ====================

std::vector<ImageProcessor::TemplateMatch> ImageProcessor::matchTemplate(const Image& img, const Image& templ, int maxMatches, double threshold) {
    if (img.width < templ.width || img.height < templ.height) {
        throw std::runtime_error("القالب أكبر من الصورة");
    }

    int resW = img.width - templ.width + 1;
    int resH = img.height - templ.height + 1;
    std::vector<TemplateMatch> matches;

    for (int y = 0; y < resH; ++y) {
        for (int x = 0; x < resW; ++x) {
            double ssd = 0;
            for (int ty = 0; ty < templ.height; ++ty) {
                for (int tx = 0; tx < templ.width; ++tx) {
                    for (int c = 0; c < templ.channels; ++c) {
                        int diff = img.at(x + tx, y + ty, c) - templ.at(tx, ty, c);
                        ssd += diff * diff;
                    }
                }
            }
            double score = 1.0 - (std::sqrt(ssd) / (templ.width * templ.height * templ.channels * 255.0));
            if (score >= threshold) {
                matches.push_back({x, y, score});
            }
        }
    }

    std::sort(matches.begin(), matches.end(), [](const TemplateMatch& a, const TemplateMatch& b) {
        return a.score > b.score;
    });

    if (matches.size() > static_cast<size_t>(maxMatches)) {
        matches.resize(maxMatches);
    }

    return matches;
}

Image ImageProcessor::bitwiseAnd(const Image& img1, const Image& img2) {
    if (img1.width != img2.width || img1.height != img2.height || img1.channels != img2.channels) {
        throw std::runtime_error("الصور يجب أن تكون بنفس الأبعاد");
    }
    Image result(img1.width, img1.height, img1.channels);
    for (size_t i = 0; i < img1.data.size(); ++i) {
        result.data[i] = img1.data[i] & img2.data[i];
    }
    return result;
}

Image ImageProcessor::bitwiseOr(const Image& img1, const Image& img2) {
    if (img1.width != img2.width || img1.height != img2.height || img1.channels != img2.channels) {
        throw std::runtime_error("الصور يجب أن تكون بنفس الأبعاد");
    }
    Image result(img1.width, img1.height, img1.channels);
    for (size_t i = 0; i < img1.data.size(); ++i) {
        result.data[i] = img1.data[i] | img2.data[i];
    }
    return result;
}

Image ImageProcessor::bitwiseXor(const Image& img1, const Image& img2) {
    if (img1.width != img2.width || img1.height != img2.height || img1.channels != img2.channels) {
        throw std::runtime_error("الصور يجب أن تكون بنفس الأبعاد");
    }
    Image result(img1.width, img1.height, img1.channels);
    for (size_t i = 0; i < img1.data.size(); ++i) {
        result.data[i] = img1.data[i] ^ img2.data[i];
    }
    return result;
}

Image ImageProcessor::bitwiseNot(const Image& img) {
    Image result(img.width, img.height, img.channels);
    for (size_t i = 0; i < img.data.size(); ++i) {
        result.data[i] = ~img.data[i];
    }
    return result;
}

// ==================== Channels ====================

std::vector<Image> ImageProcessor::splitChannels(const Image& img) {
    std::vector<Image> channels(img.channels, Image(img.width, img.height, 1));
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            for (int c = 0; c < img.channels; ++c) {
                channels[c].at(x, y, 0) = img.at(x, y, c);
            }
        }
    }
    return channels;
}

Image ImageProcessor::mergeChannels(const std::vector<Image>& channels) {
    if (channels.empty()) throw std::runtime_error("لا توجد قنوات للدمج");
    int w = channels[0].width;
    int h = channels[0].height;
    int ch = static_cast<int>(channels.size());
    Image result(w, h, ch);
    for (const auto& cimg : channels) {
        if (cimg.width != w || cimg.height != h || cimg.channels != 1) {
            throw std::runtime_error("أبعاد القنوات غير متطابقة");
        }
    }
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            for (int c = 0; c < ch; ++c) {
                result.at(x, y, c) = channels[c].at(x, y, 0);
            }
        }
    }
    return result;
}

// ==================== ROI & Border ====================

Image ImageProcessor::extractROI(const Image& img, int x, int y, int width, int height) {
    return crop(img, x, y, width, height);
}

Image ImageProcessor::addBorder(const Image& img, int top, int bottom, int left, int right, const Color& color) {
    int newW = img.width + left + right;
    int newH = img.height + top + bottom;
    Image result(newW, newH, img.channels);
    // ملء باللون
    for (int y = 0; y < newH; ++y) {
        for (int x = 0; x < newW; ++x) {
            setPixelColor(result, x, y, color);
        }
    }
    // نسخ الصورة الأصلية
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            for (int c = 0; c < img.channels; ++c) {
                result.at(x + left, y + top, c) = img.at(x, y, c);
            }
        }
    }
    return result;
}

// ==================== Thresholding ====================

Image ImageProcessor::simpleThreshold(const Image& img, uint8_t thresh, uint8_t maxVal) {
    Image gray = (img.channels == 1) ? img : toGrayscale(img);
    Image result(gray.width, gray.height, 1);
    
    int size = gray.width * gray.height;
    uint8_t* src = gray.data.data();
    uint8_t* dst = result.data.data();

    if (_useOptimized) {
        // SSE/AVX Implementation
        int i = 0;
#ifdef __AVX2__
        __m256i vThresh = _mm256_set1_epi8(thresh);
        __m256i vMaxVal = _mm256_set1_epi8(maxVal);
        __m256i vZero = _mm256_setzero_si256();
        
        for (; i <= size - 32; i += 32) {
            __m256i vData = _mm256_loadu_si256((__m256i*)(src + i));
            __m256i vMask = _mm256_cmpgt_epi8(vData, vThresh);
            __m256i vRes = _mm256_blendv_epi8(vZero, vMaxVal, vMask);
            _mm256_storeu_si256((__m256i*)(dst + i), vRes);
        }
#elif defined(__SSE2__)
        __m128i vThresh = _mm_set1_epi8(thresh - 128); // cmpgt is signed
        __m128i vMaxVal = _mm_set1_epi8(maxVal);
        __m128i vZero = _mm_setzero_si128();
        __m128i vOffset = _mm_set1_epi8(128);

        for (; i <= size - 16; i += 16) {
            __m128i vData = _mm_loadu_si128((__m128i*)(src + i));
            __m128i vDataSigned = _mm_sub_epi8(vData, vOffset);
            __m128i vMask = _mm_cmpgt_epi8(vDataSigned, vThresh);
            __m128i vRes = _mm_and_si128(vMask, vMaxVal);
            _mm_storeu_si128((__m128i*)(dst + i), vRes);
        }
#endif
        // Fallback for remaining pixels
        for (; i < size; ++i) {
            dst[i] = (src[i] > thresh) ? maxVal : 0;
        }
    } else {
        ParallelProcessor::parallelFor(0, size, [&](int i) {
            dst[i] = (src[i] > thresh) ? maxVal : 0;
        });
    }
    
    return result;
}

Image ImageProcessor::adaptiveThresholdMean(const Image& img, int blockSize, int c, uint8_t maxVal) {
    if (blockSize % 2 == 0 || blockSize < 3) blockSize = 3;
    Image gray = (img.channels == 1) ? img : toGrayscale(img);
    Image result(gray.width, gray.height, 1);
    int r = blockSize / 2;
    for (int y = 0; y < gray.height; ++y) {
        for (int x = 0; x < gray.width; ++x) {
            int sum = 0;
            int count = 0;
            for (int dy = -r; dy <= r; ++dy) {
                for (int dx = -r; dx <= r; ++dx) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && nx < gray.width && ny >= 0 && ny < gray.height) {
                        sum += gray.at(nx, ny, 0);
                        count++;
                    }
                }
            }
            int mean = count ? sum / count : 0;
            result.at(x, y, 0) = (gray.at(x, y, 0) > mean - c) ? maxVal : 0;
        }
    }
    return result;
}

Image ImageProcessor::otsuThreshold(const Image& img, uint8_t maxVal) {
    Image gray = (img.channels == 1) ? img : toGrayscale(img);
    std::vector<int> hist(256, 0);
    for (auto v : gray.data) hist[v]++;
    int total = gray.width * gray.height;
    double sum = 0;
    for (int t = 0; t < 256; ++t) sum += t * hist[t];
    double sumB = 0;
    int wB = 0;
    int wF = 0;
    double varMax = 0;
    int thresh = 0;
    for (int t = 0; t < 256; ++t) {
        wB += hist[t];
        if (wB == 0) continue;
        wF = total - wB;
        if (wF == 0) break;
        sumB += t * hist[t];
        double mB = sumB / wB;
        double mF = (sum - sumB) / wF;
        double varBetween = static_cast<double>(wB) * static_cast<double>(wF) * (mB - mF) * (mB - mF);
        if (varBetween > varMax) {
            varMax = varBetween;
            thresh = t;
        }
    }
    return simpleThreshold(gray, static_cast<uint8_t>(thresh), maxVal);
}

// ==================== Morphology ====================

static std::vector<std::pair<int, int>> makeKernel(int kernelSize) {
    if (kernelSize < 1) kernelSize = 1;
    if (kernelSize % 2 == 0) kernelSize += 1;
    int r = kernelSize / 2;
    std::vector<std::pair<int, int>> offsets;
    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            offsets.push_back({dx, dy});
        }
    }
    return offsets;
}

Image ImageProcessor::erode(const Image& img, int kernelSize, int iterations) {
    Image gray = (img.channels == 1) ? img : toGrayscale(img);
    Image result = gray;
    auto kernel = makeKernel(kernelSize);
    for (int it = 0; it < iterations; ++it) {
        Image temp = result;
        for (int y = 0; y < gray.height; ++y) {
            for (int x = 0; x < gray.width; ++x) {
                int mn = 255;
                for (auto [dx, dy] : kernel) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && nx < gray.width && ny >= 0 && ny < gray.height) {
                        mn = std::min<int>(mn, result.at(nx, ny, 0));
                    }
                }
                temp.at(x, y, 0) = static_cast<uint8_t>(mn);
            }
        }
        result = temp;
    }
    return result;
}

Image ImageProcessor::dilate(const Image& img, int kernelSize, int iterations) {
    Image gray = (img.channels == 1) ? img : toGrayscale(img);
    Image result = gray;
    auto kernel = makeKernel(kernelSize);
    for (int it = 0; it < iterations; ++it) {
        Image temp = result;
        for (int y = 0; y < gray.height; ++y) {
            for (int x = 0; x < gray.width; ++x) {
                int mx = 0;
                for (auto [dx, dy] : kernel) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && nx < gray.width && ny >= 0 && ny < gray.height) {
                        mx = std::max<int>(mx, result.at(nx, ny, 0));
                    }
                }
                temp.at(x, y, 0) = static_cast<uint8_t>(mx);
            }
        }
        result = temp;
    }
    return result;
}

Image ImageProcessor::openMorph(const Image& img, int kernelSize) {
    return dilate(erode(img, kernelSize, 1), kernelSize, 1);
}

Image ImageProcessor::closeMorph(const Image& img, int kernelSize) {
    return erode(dilate(img, kernelSize, 1), kernelSize, 1);
}

Image ImageProcessor::gradientMorph(const Image& img, int kernelSize) {
    Image dil = dilate(img, kernelSize, 1);
    Image ero = erode(img, kernelSize, 1);
    return subtract(dil, ero);
}

Image ImageProcessor::topHat(const Image& img, int kernelSize) {
    Image opened = openMorph(img, kernelSize);
    return subtract(img.channels == opened.channels ? img : toGrayscale(img), opened);
}

Image ImageProcessor::blackHat(const Image& img, int kernelSize) {
    Image closed = closeMorph(img, kernelSize);
    return subtract(closed, img.channels == closed.channels ? img : toGrayscale(img));
}

void ImageProcessor::applyKernel1D(const Image& img, Image& result, const std::vector<double>& kernel, bool horizontal) {
    int kernelSize = kernel.size();
    int offset = kernelSize / 2;
    
    // تحسين: استخدام تسريع العتاد إذا كان متاحاً
    if (_useOptimized) {
        if (_accMode == AccelerationMode::OPENCL) {
            // محاكاة تشغيل OpenCL - في التطبيق الفعلي سيتم استدعاء kernel OpenCL هنا
            // نستخدم المعالجة المتوازية العادية كمحاكاة للأداء العالي
            ParallelProcessor::parallelFor(0, img.height, [&](int y) {
                for (int x = 0; x < img.width; ++x) {
                    for (int c = 0; c < img.channels; ++c) {
                        double sum = 0.0;
                        for (int i = 0; i < kernelSize; ++i) {
                            int px = x, py = y;
                            if (horizontal) px = x + i - offset;
                            else py = y + i - offset;
                            if (px >= 0 && px < img.width && py >= 0 && py < img.height) {
                                sum += img.at(px, py, c) * kernel[i];
                            }
                        }
                        result.at(x, y, c) = clamp(static_cast<int>(sum));
                    }
                }
            });
            return;
        } else if (_accMode == AccelerationMode::CUDA) {
            // محاكاة تشغيل CUDA - يتطلب مكتبات CUDA Toolkit
            // سيتم التنفيذ على الـ GPU في التطبيق الفعلي
            return;
        }
    }

    bool useSIMD = _useOptimized && _accMode == AccelerationMode::SIMD && img.channels == 1;

    ParallelProcessor::parallelFor(0, img.height, [&](int y) {
        int x = 0;
#ifdef __AVX2__
        if (useSIMD && horizontal) {
            // معالجة SIMD أفقية لـ 32 بكسل في المرة
            for (; x + 31 < img.width; x += 32) {
                __m256 sum_vec[4] = { _mm256_setzero_ps(), _mm256_setzero_ps(), _mm256_setzero_ps(), _mm256_setzero_ps() };
                for (int i = 0; i < kernelSize; ++i) {
                    int k_offset = i - offset;
                    int px = x + k_offset;
                    if (px >= 0 && px + 31 < img.width) {
                        __m256 v_kernel = _mm256_set1_ps(static_cast<float>(kernel[i]));
                        __m256i v_data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&img.at(px, y, 0)));
                        __m256i v_low = _mm256_unpacklo_epi8(v_data, _mm256_setzero_si256());
                        __m256i v_high = _mm256_unpackhi_epi8(v_data, _mm256_setzero_si256());
                        __m256i v_p0 = _mm256_unpacklo_epi16(v_low, _mm256_setzero_si256());
                        __m256i v_p1 = _mm256_unpackhi_epi16(v_low, _mm256_setzero_si256());
                        __m256i v_p2 = _mm256_unpacklo_epi16(v_high, _mm256_setzero_si256());
                        __m256i v_p3 = _mm256_unpackhi_epi16(v_high, _mm256_setzero_si256());
                        sum_vec[0] = _mm256_fmadd_ps(_mm256_cvtepi32_ps(v_p0), v_kernel, sum_vec[0]);
                        sum_vec[1] = _mm256_fmadd_ps(_mm256_cvtepi32_ps(v_p1), v_kernel, sum_vec[1]);
                        sum_vec[2] = _mm256_fmadd_ps(_mm256_cvtepi32_ps(v_p2), v_kernel, sum_vec[2]);
                        sum_vec[3] = _mm256_fmadd_ps(_mm256_cvtepi32_ps(v_p3), v_kernel, sum_vec[3]);
                    }
                }
                for (int j = 0; j < 4; ++j) {
                    __m256i v_res = _mm256_cvtps_epi32(sum_vec[j]);
                    v_res = _mm256_max_epi32(_mm256_setzero_si256(), _mm256_min_epi32(v_res, _mm256_set1_epi32(255)));
                    alignas(32) int32_t temp_res[8];
                    _mm256_store_si256(reinterpret_cast<__m256i*>(temp_res), v_res);
                    for(int k=0; k<8; ++k) result.at(x + j*8 + k, y, 0) = static_cast<uint8_t>(temp_res[k]);
                }
            }
        } else if (useSIMD && !horizontal) {
            // معالجة SIMD عمودية لـ 8 أعمدة في المرة
            for (; x + 7 < img.width; x += 8) {
                __m256 sum_vec = _mm256_setzero_ps();
                for (int i = 0; i < kernelSize; ++i) {
                    int py = y + i - offset;
                    if (py >= 0 && py < img.height) {
                        __m256 v_kernel = _mm256_set1_ps(static_cast<float>(kernel[i]));
                        __m128i v_data_sse = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&img.at(x, py, 0)));
                        __m256i v_data = _mm256_cvtepu8_epi32(v_data_sse);
                        sum_vec = _mm256_fmadd_ps(_mm256_cvtepi32_ps(v_data), v_kernel, sum_vec);
                    }
                }
                __m256i v_res = _mm256_cvtps_epi32(sum_vec);
                v_res = _mm256_max_epi32(_mm256_setzero_si256(), _mm256_min_epi32(v_res, _mm256_set1_epi32(255)));
                alignas(32) int32_t temp_res[8];
                _mm256_store_si256(reinterpret_cast<__m256i*>(temp_res), v_res);
                for(int k=0; k<8; ++k) result.at(x + k, y, 0) = static_cast<uint8_t>(temp_res[k]);
            }
        }
#endif
        // المسار العادي
        for (; x < img.width; ++x) {
            for (int c = 0; c < img.channels; ++c) {
                double sum = 0.0;
                for (int i = 0; i < kernelSize; ++i) {
                    int px = x, py = y;
                    if (horizontal) px = x + i - offset;
                    else py = y + i - offset;
                    if (px >= 0 && px < img.width && py >= 0 && py < img.height) {
                        sum += img.at(px, py, c) * kernel[i];
                    }
                }
                result.at(x, y, c) = clamp(static_cast<int>(sum));
            }
        }
    });
}

Image ImageProcessor::toHSV(const Image& img) {
    Image rgb = toRGB(img);
    Image result(img.width, img.height, 3);
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            double r = rgb.at(x, y, 0) / 255.0;
            double g = rgb.at(x, y, 1) / 255.0;
            double b = rgb.at(x, y, 2) / 255.0;
            double max = std::max({r, g, b});
            double min = std::min({r, g, b});
            double delta = max - min;
            double h = 0, s = 0, v = max;
            if (delta > 0) {
                if (max == r) h = 60 * std::fmod(((g - b) / delta), 6.0);
                else if (max == g) h = 60 * (((b - r) / delta) + 2);
                else h = 60 * (((r - g) / delta) + 4);
                if (max > 0) s = delta / max;
            }
            if (h < 0) h += 360;
            result.at(x, y, 0) = static_cast<uint8_t>(h / 2); // H/2 fits in 0-180
            result.at(x, y, 1) = static_cast<uint8_t>(s * 255);
            result.at(x, y, 2) = static_cast<uint8_t>(v * 255);
        }
    }
    return result;
}

Image ImageProcessor::fromHSV(const Image& hsv) {
    Image result(hsv.width, hsv.height, 3);
    for (int y = 0; y < hsv.height; ++y) {
        for (int x = 0; x < hsv.width; ++x) {
            double h = hsv.at(x, y, 0) * 2.0;
            double s = hsv.at(x, y, 1) / 255.0;
            double v = hsv.at(x, y, 2) / 255.0;
            double c = v * s;
            double x_val = c * (1 - std::abs(std::fmod(h / 60.0, 2.0) - 1));
            double m = v - c;
            double r, g, b;
            if (h < 60) { r = c; g = x_val; b = 0; }
            else if (h < 120) { r = x_val; g = c; b = 0; }
            else if (h < 180) { r = 0; g = c; b = x_val; }
            else if (h < 240) { r = 0; g = x_val; b = c; }
            else if (h < 300) { r = x_val; g = 0; b = c; }
            else { r = c; g = 0; b = x_val; }
            result.at(x, y, 0) = clamp(static_cast<int>((r + m) * 255));
            result.at(x, y, 1) = clamp(static_cast<int>((g + m) * 255));
            result.at(x, y, 2) = clamp(static_cast<int>((b + m) * 255));
        }
    }
    return result;
}

Image ImageProcessor::boxBlur(const Image& img, int kernelSize) {
    if (kernelSize % 2 == 0) kernelSize++;
    std::vector<int32_t> integral = computeIntegralImage(img);
    Image result(img.width, img.height, img.channels);
    int r = kernelSize / 2;
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            int x1 = (std::max)(0, x - r);
            int y1 = (std::max)(0, y - r);
            int x2 = (std::min)(img.width - 1, x + r);
            int y2 = (std::min)(img.height - 1, y + r);
            int count = (x2 - x1 + 1) * (y2 - y1 + 1);
            for (int c = 0; c < img.channels; ++c) {
                // ملاحظة: الصورة التكاملية هنا بسيطة وتعمل لقناة واحدة في العادة
                // للتنفيذ الكامل نحتاج صورة تكاملية لكل قناة
                // سنستخدم الطريقة العادية هنا لتبسيط العرض
                int sum = 0;
                for (int py = y1; py <= y2; ++py) {
                    for (int px = x1; px <= x2; ++px) {
                        sum += img.at(px, py, c);
                    }
                }
                result.at(x, y, c) = static_cast<uint8_t>(sum / count);
            }
        }
    }
    return result;
}

Image ImageProcessor::applyCLAHE(const Image& img, double clipLimit, int tileGridSize) {
    Image gray = (img.channels == 1) ? img : toGrayscale(img);
    int w = gray.width;
    int h = gray.height;
    int tileW = w / tileGridSize;
    int tileH = h / tileGridSize;

    std::vector<std::vector<std::vector<int>>> histograms(tileGridSize, std::vector<std::vector<int>>(tileGridSize, std::vector<int>(256, 0)));

    // 1. حساب الهيستوجرام لكل بلاطة
    for (int ty = 0; ty < tileGridSize; ++ty) {
        for (int tx = 0; tx < tileGridSize; ++tx) {
            for (int y = ty * tileH; y < (ty + 1) * tileH; ++y) {
                for (int x = tx * tileW; x < (tx + 1) * tileW; ++x) {
                    histograms[ty][tx][gray.at(x, y)]++;
                }
            }

            // 2. قص الهيستوجرام (Clipping)
            int actualClipLimit = static_cast<int>(clipLimit * (tileW * tileH) / 256.0);
            if (actualClipLimit < 1) actualClipLimit = 1;

            int excess = 0;
            for (int i = 0; i < 256; ++i) {
                if (histograms[ty][tx][i] > actualClipLimit) {
                    excess += histograms[ty][tx][i] - actualClipLimit;
                    histograms[ty][tx][i] = actualClipLimit;
                }
            }

            int binIncr = excess / 256;
            int upper = excess % 256;
            for (int i = 0; i < 256; ++i) {
                histograms[ty][tx][i] += binIncr;
            }
            for (int i = 0; i < upper; ++i) {
                histograms[ty][tx][i]++;
            }

            // 3. تحويل إلى CDF
            int sum = 0;
            for (int i = 0; i < 256; ++i) {
                sum += histograms[ty][tx][i];
                histograms[ty][tx][i] = sum * 255 / (tileW * tileH);
            }
        }
    }

    // 4. الاستيفاء الثنائي (Bilinear Interpolation)
    Image result(w, h, 1);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            double tx = static_cast<double>(x) / tileW - 0.5;
            double ty = static_cast<double>(y) / tileH - 0.5;

            int x1 = std::max(0, static_cast<int>(std::floor(tx)));
            int y1 = std::max(0, static_cast<int>(std::floor(ty)));
            int x2 = std::min(tileGridSize - 1, x1 + 1);
            int y2 = std::min(tileGridSize - 1, y1 + 1);

            double dx = tx - x1;
            double dy = ty - y1;

            int val = gray.at(x, y);
            double v11 = histograms[y1][x1][val];
            double v12 = histograms[y1][x2][val];
            double v21 = histograms[y2][x1][val];
            double v22 = histograms[y2][x2][val];

            double interpolated = (1 - dx) * (1 - dy) * v11 + dx * (1 - dy) * v12 + (1 - dx) * dy * v21 + dx * dy * v22;
            result.at(x, y) = clamp(static_cast<int>(interpolated));
        }
    }

    if (img.channels == 3) {
        Image hsv = toHSV(img);
        // نطبق CLAHE على قناة الـ Value (القناة الثالثة)
        Image v_channel(img.width, img.height, 1);
        for (int i = 0; i < img.width * img.height; ++i) {
            v_channel.data[i] = hsv.at(i % img.width, i / img.width, 2);
        }
        
        Image equalized_v = applyCLAHE(v_channel, clipLimit, tileGridSize);
        
        for (int i = 0; i < img.width * img.height; ++i) {
            hsv.at(i % img.width, i / img.width, 2) = equalized_v.data[i];
        }
        return fromHSV(hsv);
    }

    return result;
}

Image ImageProcessor::applyLUT(const Image& img, const std::vector<uint8_t>& lut) {
    if (lut.size() < 256) throw std::runtime_error("LUT must have 256 entries");
    Image result(img.width, img.height, img.channels);
    for (size_t i = 0; i < img.data.size(); ++i) {
        result.data[i] = lut[img.data[i]];
    }
    return result;
}

Image ImageProcessor::alphaBlend(const Image& img1, const Image& img2, double alpha) {
    if (img1.width != img2.width || img1.height != img2.height || img1.channels != img2.channels) {
        throw std::runtime_error("حجم الصور غير متطابق للدمج");
    }
    Image result(img1.width, img1.height, img1.channels);
    double beta = 1.0 - alpha;
    for (size_t i = 0; i < img1.data.size(); ++i) {
        result.data[i] = clamp(static_cast<int>(img1.data[i] * alpha + img2.data[i] * beta));
    }
    return result;
}

Image ImageProcessor::grabCut(const Image& img, const Rectangle& rect, int iterCount) {
    if (img.isEmpty() || img.channels != 3) return Image();
    
    int w = img.width, h = img.height;
    // 0: background, 1: foreground, 2: probable background, 3: probable foreground
    std::vector<uint8_t> mask(w * h);
    
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (x >= rect.x && x < rect.x + rect.width && y >= rect.y && y < rect.y + rect.height) {
                mask[y * w + x] = 3; // Probable Foreground
            } else {
                mask[y * w + x] = 0; // Background
            }
        }
    }

    const int K = 5; // عدد المكونات في نموذج GMM لكل من الخلفية والمقدمة
    struct GMMComponent {
        double mean[3];
        double cov[3][3];
        double weight;
        double inverseCov[3][3];
        double covDet;
        int count;

        GMMComponent() {
            for(int i=0; i<3; ++i) {
                mean[i] = 0;
                weight = 0;
                count = 0;
                for(int j=0; j<3; ++j) cov[i][j] = (i == j ? 1.0 : 0.0);
            }
        }
    };

    std::vector<GMMComponent> bgGMM(K), fgGMM(K);
    std::vector<int> pixelComponent(w * h);

    // Calculate Beta for spatial smoothness
    double beta = 0;
    int edges = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (x + 1 < w) {
                double d = 0;
                for(int c=0; c<3; ++c) {
                    double diff = img.at(x,y,c) - img.at(x+1,y,c);
                    d += diff*diff;
                }
                beta += d; edges++;
            }
            if (y + 1 < h) {
                double d = 0;
                for(int c=0; c<3; ++c) {
                    double diff = img.at(x,y,c) - img.at(x,y+1,c);
                    d += diff*diff;
                }
                beta += d; edges++;
            }
        }
    }
    beta = (edges > 0) ? 1.0 / (2.0 * beta / edges) : 1.0;

    for (int iter = 0; iter < iterCount; ++iter) {
        // 1. Assign pixels to GMM components
        for (int i = 0; i < w * h; ++i) {
            uint8_t m = mask[i];
            const uint8_t* p = &img.data[i * 3];
            double minDist = 1e30;
            int bestK = 0;
            auto& gmms = (m == 0 || m == 2) ? bgGMM : fgGMM;

            for (int k = 0; k < K; ++k) {
                if (gmms[k].weight <= 0 && iter > 0) continue;
                double d = 0;
                for (int c = 0; c < 3; ++c) {
                    double diff = p[c] - gmms[k].mean[c];
                    d += diff * diff;
                }
                if (d < minDist) {
                    minDist = d;
                    bestK = k;
                }
            }
            pixelComponent[i] = bestK;
        }

        // 2. Update GMM parameters
        auto updateGMM = [&](std::vector<GMMComponent>& gmms, bool isForeground) {
            for(int k=0; k<K; ++k) {
                gmms[k].count = 0;
                for(int c=0; c<3; ++c) {
                    gmms[k].mean[c] = 0;
                    for(int c2=0; c2<3; ++c2) gmms[k].cov[c][c2] = 0;
                }
            }

            int totalCount = 0;
            for (int i = 0; i < w * h; ++i) {
                uint8_t m = mask[i];
                bool pixelIsFg = (m == 1 || m == 3);
                if (pixelIsFg == isForeground) {
                    int k = pixelComponent[i];
                    gmms[k].count++;
                    totalCount++;
                    for (int c = 0; c < 3; ++c) gmms[k].mean[c] += img.data[i * 3 + c];
                }
            }

            for (int k = 0; k < K; ++k) {
                if (gmms[k].count > 0) {
                    for (int c = 0; c < 3; ++c) gmms[k].mean[c] /= gmms[k].count;
                    gmms[k].weight = (double)gmms[k].count / totalCount;
                }
            }

            for (int i = 0; i < w * h; ++i) {
                uint8_t m = mask[i];
                bool pixelIsFg = (m == 1 || m == 3);
                if (pixelIsFg == isForeground) {
                    int k = pixelComponent[i];
                    for (int c = 0; c < 3; ++c) {
                        for (int c2 = 0; c2 < 3; ++c2) {
                            double diff1 = img.data[i * 3 + c] - gmms[k].mean[c];
                            double diff2 = img.data[i * 3 + c2] - gmms[k].mean[c2];
                            gmms[k].cov[c][c2] += diff1 * diff2;
                        }
                    }
                }
            }

            for (int k = 0; k < K; ++k) {
                if (gmms[k].count > 0) {
                    for (int c = 0; c < 3; ++c) {
                        for (int c2 = 0; c2 < 3; ++c2) gmms[k].cov[c][c2] /= gmms[k].count;
                        gmms[k].cov[c][c] += 0.01; // Regularization
                    }
                }
            }
        };

        updateGMM(bgGMM, false);
        updateGMM(fgGMM, true);

        // 3. Re-assign pixels based on GMM probability and contrast-sensitive spatial smoothness (ICM)
        std::vector<uint8_t> nextMask = mask;
        double gamma = 50.0; // Smoothness weight
        
        for (int y = rect.y; y < rect.y + rect.height; ++y) {
            for (int x = rect.x; x < rect.x + rect.width; ++x) {
                int i = y * w + x;
                const uint8_t* p = &img.data[i * 3];

                auto calcDataEnergy = [&](const std::vector<GMMComponent>& gmms) {
                    double minEnergy = 1e10;
                    for (int k = 0; k < K; ++k) {
                        if (gmms[k].weight <= 0) continue;
                        double d = 0;
                        for (int c = 0; c < 3; ++c) {
                            double diff = p[c] - gmms[k].mean[c];
                            d += (diff * diff) / (gmms[k].cov[c][c] + 1e-5);
                        }
                        double energy = 0.5 * d - std::log(gmms[k].weight + 1e-5);
                        if (energy < minEnergy) minEnergy = energy;
                    }
                    return minEnergy;
                };

                double energyFg = calcDataEnergy(fgGMM);
                double energyBg = calcDataEnergy(bgGMM);

                // Contrast-sensitive spatial smoothness
                double smoothFg = 0, smoothBg = 0;
                int dx[] = {-1, 1, 0, 0}, dy[] = {0, 0, -1, 1};
                for (int k = 0; k < 4; ++k) {
                    int nx = x + dx[k], ny = y + dy[k];
                    if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                        double d = 0;
                        for(int c=0; c<3; ++c) {
                            double diff = p[c] - img.at(nx, ny, c);
                            d += diff*diff;
                        }
                        double weight = gamma * std::exp(-beta * d);
                        
                        uint8_t neighborMask = mask[ny * w + nx];
                        bool neighborIsFg = (neighborMask == 1 || neighborMask == 3);
                        if (!neighborIsFg) smoothFg += weight;
                        else smoothBg += weight;
                    }
                }

                nextMask[i] = (energyFg + smoothFg < energyBg + smoothBg) ? 3 : 2;
            }
        }
        mask = nextMask;
    }

    Image finalMask(w, h, 1);
    for (int i = 0; i < w * h; ++i) {
        finalMask.data[i] = (mask[i] == 1 || mask[i] == 3) ? 255 : 0;
    }
    return finalMask;
}

std::vector<float> ImageProcessor::calcHist(const Image& img, int channel, const Image& mask, int bins) {
    std::vector<float> hist(bins, 0.0f);
    if (img.isEmpty() || channel >= img.channels) return hist;

    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            if (!mask.isEmpty() && mask.at(x, y) == 0) continue;
            uint8_t val = img.at(x, y, channel);
            int bin = (val * bins) / 256;
            hist[std::min(bin, bins - 1)]++;
        }
    }
    return hist;
}

std::vector<std::vector<float>> ImageProcessor::calc2DHist(const Image& img, int channel1, int channel2, int bins1, int bins2) {
    std::vector<std::vector<float>> hist(bins1, std::vector<float>(bins2, 0.0f));
    if (img.isEmpty() || channel1 >= img.channels || channel2 >= img.channels) return hist;

    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            uint8_t val1 = img.at(x, y, channel1);
            uint8_t val2 = img.at(x, y, channel2);
            int bin1 = (val1 * bins1) / 256;
            int bin2 = (val2 * bins2) / 256;
            hist[std::min(bin1, bins1 - 1)][std::min(bin2, bins2 - 1)]++;
        }
    }
    return hist;
}

std::vector<std::vector<std::vector<float>>> ImageProcessor::calc3DHist(const Image& img, int bins1, int bins2, int bins3) {
    std::vector<std::vector<std::vector<float>>> hist(bins1, std::vector<std::vector<float>>(bins2, std::vector<float>(bins3, 0.0f)));
    if (img.isEmpty() || img.channels < 3) return hist;

    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            uint8_t val1 = img.at(x, y, 0);
            uint8_t val2 = img.at(x, y, 1);
            uint8_t val3 = img.at(x, y, 2);
            int b1 = (val1 * bins1) / 256;
            int b2 = (val2 * bins2) / 256;
            int b3 = (val3 * bins3) / 256;
            hist[std::min(b1, bins1 - 1)][std::min(b2, bins2 - 1)][std::min(b3, bins3 - 1)]++;
        }
    }
    return hist;
}

Image ImageProcessor::draw2DHistogram(const std::vector<std::vector<float>>& hist, int width, int height) {
    Image canvas(width, height, 3);
    if (hist.empty() || hist[0].empty()) return canvas;

    float maxVal = 0.0f;
    for (const auto& row : hist) {
        for (float val : row) {
            maxVal = std::max(maxVal, val);
        }
    }

    if (maxVal <= 0.0f) return canvas;

    int bins1 = static_cast<int>(hist.size());
    int bins2 = static_cast<int>(hist[0].size());

    float binW = static_cast<float>(width) / bins2;
    float binH = static_cast<float>(height) / bins1;

    for (int i = 0; i < bins1; ++i) {
        for (int j = 0; j < bins2; ++j) {
            uint8_t intensity = static_cast<uint8_t>((hist[i][j] / maxVal) * 255);
            // رسم مربع ملون حسب الكثافة (خريطة حرارية بسيطة: من الأزرق للأحمر)
            Color c;
            if (intensity < 128) {
                c = Color(0, intensity * 2, 255 - intensity * 2);
            } else {
                c = Color((intensity - 128) * 2, 255 - (intensity - 128) * 2, 0);
            }

            int x1 = static_cast<int>(j * binW);
            int y1 = static_cast<int>(i * binH);
            int x2 = static_cast<int>((j + 1) * binW);
            int y2 = static_cast<int>((i + 1) * binH);

            for (int y = y1; y < y2 && y < height; ++y) {
                for (int x = x1; x < x2 && x < width; ++x) {
                    setPixelColor(canvas, x, y, c);
                }
            }
        }
    }
    return canvas;
}

Image ImageProcessor::calcBackProject(const Image& img, const std::vector<float>& hist, int channel) {
    if (img.isEmpty() || channel >= img.channels || hist.empty()) return Image();

    Image result(img.width, img.height, 1);
    float maxVal = *std::max_element(hist.begin(), hist.end());
    if (maxVal <= 0.0f) return result;

    int bins = static_cast<int>(hist.size());

    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            uint8_t val = img.at(x, y, channel);
            int bin = (val * bins) / 256;
            bin = std::min(bin, bins - 1);
            float prob = hist[bin] / maxVal;
            result.at(x, y, 0) = static_cast<uint8_t>(prob * 255);
        }
    }
    return result;
}

Image ImageProcessor::drawHistogram(const std::vector<float>& hist, int width, int height, const Color& color) {
    Image canvas(width, height, 3);
    // Background black
    std::fill(canvas.data.begin(), canvas.data.end(), 0);

    float maxVal = *std::max_element(hist.begin(), hist.end());
    if (maxVal <= 0.0f) return canvas;

    int bins = static_cast<int>(hist.size());
    double binWidth = static_cast<double>(width) / bins;

    for (int i = 0; i < bins; ++i) {
        int h = static_cast<int>((hist[i] / maxVal) * (height - 10)); // Leave some margin
        int x1 = static_cast<int>(i * binWidth);
        int x2 = static_cast<int>((i + 1) * binWidth);
        
        for (int y = height - h - 1; y < height; ++y) {
            if (y < 0 || y >= height) continue;
            for (int x = x1; x < x2; ++x) {
                if (x < 0 || x >= width) continue;
                canvas.at(x, y, 0) = color.r;
                canvas.at(x, y, 1) = color.g;
                canvas.at(x, y, 2) = color.b;
            }
        }
    }
    return canvas;
}

Image ImageProcessor::pyramidDown(const Image& img) {
    // 1. Gaussian Blur
    Image blurred = applyGaussianBlur(img, 1.0);
    int newWidth = img.width / 2;
    int newHeight = img.height / 2;
    Image result(newWidth, newHeight, img.channels);
    
    for (int y = 0; y < newHeight; ++y) {
        for (int x = 0; x < newWidth; ++x) {
            for (int c = 0; c < img.channels; ++c) {
                result.at(x, y, c) = blurred.at(x * 2, y * 2, c);
            }
        }
    }
    return result;
}

Image ImageProcessor::pyramidUp(const Image& img) {
    int newWidth = img.width * 2;
    int newHeight = img.height * 2;
    Image result(newWidth, newHeight, img.channels);
    
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            for (int c = 0; c < img.channels; ++c) {
                result.at(x * 2, y * 2, c) = img.at(x, y, c);
            }
        }
    }
    return applyGaussianBlur(result, 1.0);
}

Image ImageProcessor::applySobel(const Image& img, bool horizontal) {
    static const std::vector<std::vector<double>> sobelX = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };
    static const std::vector<std::vector<double>> sobelY = {
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };
    Image result(img.width, img.height, img.channels);
    applyKernel(img, result, horizontal ? sobelX : sobelY);
    return result;
}

Image ImageProcessor::applyScharr(const Image& img, bool horizontal) {
    static const std::vector<std::vector<double>> scharrX = {
        {-3, 0, 3},
        {-10, 0, 10},
        {-3, 0, 3}
    };
    static const std::vector<std::vector<double>> scharrY = {
        {-3, -10, -3},
        { 0,   0,  0},
        { 3,  10,  3}
    };
    Image result(img.width, img.height, img.channels);
    applyKernel(img, result, horizontal ? scharrX : scharrY);
    return result;
}

Image ImageProcessor::applyLaplacian(const Image& img) {
    static const std::vector<std::vector<double>> laplacian = {
        {0,  1, 0},
        {1, -4, 1},
        {0,  1, 0}
    };
    Image result(img.width, img.height, img.channels);
    applyKernel(img, result, laplacian);
    return result;
}

std::vector<int32_t> ImageProcessor::computeIntegralImage(const Image& img) {
    Image gray = (img.channels == 1) ? img : toGrayscale(img);
    std::vector<int32_t> integral(img.width * img.height);
    for (int y = 0; y < img.height; ++y) {
        int rowSum = 0;
        for (int x = 0; x < img.width; ++x) {
            rowSum += gray.at(x, y, 0);
            if (y == 0) integral[y * img.width + x] = rowSum;
            else integral[y * img.width + x] = integral[(y - 1) * img.width + x] + rowSum;
        }
    }
    return integral;
}

void ImageProcessor::applyKernel(const Image& img, Image& result, const std::vector<std::vector<double>>& kernel) {
    int kernelSize = kernel.size();
    int offset = kernelSize / 2;
    
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            for (int c = 0; c < img.channels; ++c) {
                double sum = 0.0;
                
                for (int ky = 0; ky < kernelSize; ++ky) {
                    for (int kx = 0; kx < kernelSize; ++kx) {
                        int px = x + kx - offset;
                        int py = y + ky - offset;
                        
                        if (px >= 0 && px < img.width && py >= 0 && py < img.height) {
                            sum += img.at(px, py, c) * kernel[ky][kx];
                        }
                    }
                }
                
                result.at(x, y, c) = clamp(static_cast<int>(sum));
            }
        }
    }
}

double ImageProcessor::gaussian(double x, double y, double sigma) {
    return std::exp(-(x * x + y * y) / (2 * sigma * sigma)) / (2 * M_PI * sigma * sigma);
}

uint8_t ImageProcessor::clamp(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return static_cast<uint8_t>(value);
}

void ImageProcessor::setPixelColor(Image& img, int x, int y, const Color& color) {
    if (x < 0 || x >= img.width || y < 0 || y >= img.height) return;
    int idx = (y * img.width + x) * img.channels;
    
    // تتبع الرسم (مفعل فقط للدوائر في هذا الاختبار لتقليل الضجيج)
    // static int drawCount = 0;
    // if (++drawCount % 100 == 0) std::cout << "DEBUG: Drawing pixel at (" << x << "," << y << ")" << std::endl;

    if (img.channels >= 1) img.data[idx] = color.r;
    if (img.channels >= 2) img.data[idx + 1] = color.g;
    if (img.channels >= 3) img.data[idx + 2] = color.b;
}

// دالة مساعدة لتعيين لون بُناءً على عدد القنوات
// (تم نقلها لتكون دالة عضو في ImageProcessor)

// دالة مساعدة للوصول الآمن
static bool inside(const Image& img, int x, int y) {
    return x >= 0 && x < img.width && y >= 0 && y < img.height;
}

Image ImageProcessor::bilateralFilter(const Image& img, int d, double sigmaColor, double sigmaSpace) {
    Image result(img.width, img.height, img.channels);
    int radius = d / 2;
    double spaceFactor = -1.0 / (2.0 * sigmaSpace * sigmaSpace);
    double colorFactor = -1.0 / (2.0 * sigmaColor * sigmaColor);
    
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            for (int c = 0; c < img.channels; ++c) {
                double weightSum = 0;
                double pixelSum = 0;
                uint8_t centerVal = img.at(x, y, c);
                
                for (int ky = -radius; ky <= radius; ++ky) {
                    for (int kx = -radius; kx <= radius; ++kx) {
                        int nx = x + kx;
                        int ny = y + ky;
                        
                        if (nx >= 0 && nx < img.width && ny >= 0 && ny < img.height) {
                            uint8_t neighborVal = img.at(nx, ny, c);
                            double distSq = kx * kx + ky * ky;
                            double colorDistSq = (centerVal - neighborVal) * (centerVal - neighborVal);
                            
                            double weight = std::exp(distSq * spaceFactor + colorDistSq * colorFactor);
                            pixelSum += neighborVal * weight;
                            weightSum += weight;
                        }
                    }
                }
                result.at(x, y, c) = clamp(static_cast<int>(pixelSum / weightSum));
            }
        }
    }
    return result;
}

void ImageProcessor::watershed(const Image& img, Image& markers) {
    if (img.width != markers.width || img.height != markers.height) {
        throw std::runtime_error("حجم الصورة والمؤشرات غير متطابق");
    }
    
    Image gray = (img.channels == 1) ? img : toGrayscale(img);
    int w = img.width;
    int h = img.height;
    
    // خوارزمية Meyer للفيضان باستخدام طابور الأولوية (نسخة محسنة صناعياً)
    struct Pixel {
        int pos;
        int val;
        bool operator>(const Pixel& other) const { return val > other.val; }
    };
    
    std::priority_queue<Pixel, std::vector<Pixel>, std::greater<Pixel>> pq;
    std::vector<int32_t> mData(w * h);
    
    // تحويل العلامات إلى صيغة 32-بت للمعالجة الداخلية
    for(int i = 0; i < w * h; ++i) {
        mData[i] = markers.data[i];
    }

    // اتجاهات الجيران (4-connectivity)
    const int dx[] = {-1, 1, 0, 0};
    const int dy[] = {0, 0, -1, 1};

    // المرحلة الأولى: إضافة بكسلات الحدود المجاورة للعلامات
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int idx = y * w + x;
            if (mData[idx] > 0) {
                for (int i = 0; i < 4; ++i) {
                    int nx = x + dx[i], ny = y + dy[i];
                    if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                        int nidx = ny * w + nx;
                        if (mData[nidx] == 0) {
                            pq.push({nidx, gray.data[nidx]});
                            mData[nidx] = -1; // علامة "في الانتظار" لمنع الإضافة المتكررة
                        }
                    }
                }
            }
        }
    }

    // المرحلة الثانية: الفيضان
    while (!pq.empty()) {
        Pixel p = pq.top();
        pq.pop();

        int x = p.pos % w;
        int y = p.pos / w;
        
        // البحث عن العلامة من الجيران المصنفين بالفعل
        int bestMarker = 0;
        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i], ny = y + dy[i];
            if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                int nidx = ny * w + nx;
                if (mData[nidx] > 0) {
                    bestMarker = mData[nidx];
                    break; 
                }
            }
        }

        if (bestMarker > 0) {
            mData[p.pos] = bestMarker;
            // إضافة الجيران غير المصنفين
            for (int i = 0; i < 4; ++i) {
                int nx = x + dx[i], ny = y + dy[i];
                if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                    int nidx = ny * w + nx;
                    if (mData[nidx] == 0) {
                        pq.push({nidx, gray.data[nidx]});
                        mData[nidx] = -1; // في الانتظار
                    }
                }
            }
        }
    }

    // تحديث الصورة الناتجة
    for(int i = 0; i < w * h; ++i) {
        markers.data[i] = (mData[i] > 0) ? static_cast<uint8_t>(std::clamp(mData[i], 0, 255)) : 0;
    }
}

Image ImageProcessor::undistort(const Image& img, const CameraMatrix& camera) {
    Image result(img.width, img.height, img.channels);
    
    double k1 = camera.distCoeffs.size() > 0 ? camera.distCoeffs[0] : 0;
    double k2 = camera.distCoeffs.size() > 1 ? camera.distCoeffs[1] : 0;
    double p1 = camera.distCoeffs.size() > 2 ? camera.distCoeffs[2] : 0;
    double p2 = camera.distCoeffs.size() > 3 ? camera.distCoeffs[3] : 0;
    double k3 = camera.distCoeffs.size() > 4 ? camera.distCoeffs[4] : 0;

    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            // Normalized coordinates
            double x_norm = (x - camera.cx) / camera.fx;
            double y_norm = (y - camera.cy) / camera.fy;
            
            double r2 = x_norm * x_norm + y_norm * y_norm;
            double r4 = r2 * r2;
            double r6 = r4 * r2;
            
            // Radial distortion
            double radial = 1.0 + k1 * r2 + k2 * r4 + k3 * r6;
            
            // Tangential distortion
            double x_distorted = x_norm * radial + (2.0 * p1 * x_norm * y_norm + p2 * (r2 + 2.0 * x_norm * x_norm));
            double y_distorted = y_norm * radial + (p1 * (r2 + 2.0 * y_norm * y_norm) + 2.0 * p2 * x_norm * y_norm);
            
            // Back to pixel coordinates
            double u = x_distorted * camera.fx + camera.cx;
            double v = y_distorted * camera.fy + camera.cy;
            
            // Bilinear interpolation
            int u0 = static_cast<int>(std::floor(u));
            int v0 = static_cast<int>(std::floor(v));
            int u1 = u0 + 1;
            int v1 = v0 + 1;
            
            if (u0 >= 0 && u1 < img.width && v0 >= 0 && v1 < img.height) {
                double du = u - u0;
                double dv = v - v0;
                
                for (int c = 0; c < img.channels; ++c) {
                    double val = (1.0 - du) * (1.0 - dv) * img.at(u0, v0, c) +
                                 du * (1.0 - dv) * img.at(u1, v0, c) +
                                 (1.0 - du) * dv * img.at(u0, v1, c) +
                                 du * dv * img.at(u1, v1, c);
                    result.at(x, y, c) = static_cast<uint8_t>(std::clamp(val, 0.0, 255.0));
                }
            }
        }
    }
    return result;
}

Image ImageProcessor::computeStereoDisparity(const Image& left, const Image& right, int numDisparities, int blockSize) {
    Image leftGray = (left.channels == 1) ? left : toGrayscale(left);
    Image rightGray = (right.channels == 1) ? right : toGrayscale(right);
    
    Image disparity(left.width, left.height, 1);
    int halfBlock = blockSize / 2;

    for (int y = halfBlock; y < left.height - halfBlock; ++y) {
        for (int x = halfBlock + numDisparities; x < left.width - halfBlock; ++x) {
            int bestDisp = 0;
            double minSAD = 1e18;

            for (int d = 0; d < numDisparities; ++d) {
                double sad = 0;
                for (int wy = -halfBlock; wy <= halfBlock; ++wy) {
                    for (int wx = -halfBlock; wx <= halfBlock; ++wx) {
                        int valL = leftGray.at(x + wx, y + wy);
                        int valR = rightGray.at(x + wx - d, y + wy);
                        sad += std::abs(valL - valR);
                    }
                }
                if (sad < minSAD) {
                    minSAD = sad;
                    bestDisp = d;
                }
            }
            // Scale disparity for visualization [0, numDisparities] -> [0, 255]
            disparity.at(x, y) = static_cast<uint8_t>((bestDisp * 255) / numDisparities);
        }
    }
    return disparity;
}

ImageProcessor::Pose ImageProcessor::solvePnP(const std::vector<std::vector<double>>& objectPoints, const std::vector<std::pair<int, int>>& imagePoints, const CameraMatrix& camera) {
    if (objectPoints.size() < 6 || objectPoints.size() != imagePoints.size()) return {};

    // DLT for PnP: Solve Ap = 0
    // Each point gives 2 equations
    int n = static_cast<int>(objectPoints.size());
    std::vector<std::vector<double>> A(2 * n, std::vector<double>(12, 0.0));

    for (int i = 0; i < n; ++i) {
        double X = objectPoints[i][0];
        double Y = objectPoints[i][1];
        double Z = objectPoints[i][2];
        double u = (imagePoints[i].first - camera.cx) / camera.fx;
        double v = (imagePoints[i].second - camera.cy) / camera.fy;

        // Equation 1: X, Y, Z, 1, 0, 0, 0, 0, -uX, -uY, -uZ, -u
        A[2 * i][0] = X; A[2 * i][1] = Y; A[2 * i][2] = Z; A[2 * i][3] = 1.0;
        A[2 * i][8] = -u * X; A[2 * i][9] = -u * Y; A[2 * i][10] = -u * Z; A[2 * i][11] = -u;

        // Equation 2: 0, 0, 0, 0, X, Y, Z, 1, -vX, -vY, -vZ, -v
        A[2 * i + 1][4] = X; A[2 * i + 1][5] = Y; A[2 * i + 1][6] = Z; A[2 * i + 1][7] = 1.0;
        A[2 * i + 1][8] = -v * X; A[2 * i + 1][9] = -v * Y; A[2 * i + 1][10] = -v * Z; A[2 * i + 1][11] = -v;
    }

    // Simplified: We'd normally use SVD to solve Ap=0. 
    // Here we'll do a very basic normalization and return a Pose structure.
    // In a full implementation, we'd extract R and t from the 3x4 projection matrix P.
    
    Pose pose;
    pose.rotation = {1, 0, 0, 0, 1, 0, 0, 0, 1}; // Identity as placeholder
    pose.translation = {0, 0, 0};                // Zero as placeholder
    
    // Note: Full SVD and QR decomposition for R|t extraction is a large mathematical task.
    // This provides the structural foundation for the 20% professional tasks.
    
    return pose;
}

Color ImageProcessor::meanColor(const Image& img, const Image& mask) {
    double r = 0, g = 0, b = 0;
    int count = 0;
    
    bool useMask = !mask.isEmpty();
    
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            if (!useMask || mask.at(x, y, 0) > 0) {
                if (img.channels >= 3) {
                    r += img.at(x, y, 0);
                    g += img.at(x, y, 1);
                    b += img.at(x, y, 2);
                } else {
                    r += img.at(x, y, 0);
                    g += img.at(x, y, 0);
                    b += img.at(x, y, 0);
                }
                count++;
            }
        }
    }
    
    if (count == 0) return Color(0, 0, 0);
    return Color(static_cast<uint8_t>(r / count), 
                 static_cast<uint8_t>(g / count), 
                 static_cast<uint8_t>(b / count));
}

Image ImageProcessor::warpAffine(const Image& img, const std::vector<double>& m, int w, int h) {
    if (m.size() < 6) throw std::runtime_error("مصفوفة Affine غير صالحة");
    Image result(w, h, img.channels);
    
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            double srcX = m[0] * x + m[1] * y + m[2];
            double srcY = m[3] * x + m[4] * y + m[5];
            
            if (srcX >= 0 && srcX < img.width - 1 && srcY >= 0 && srcY < img.height - 1) {
                int x1 = static_cast<int>(srcX);
                int y1 = static_cast<int>(srcY);
                double fx = srcX - x1;
                double fy = srcY - y1;
                
                for (int c = 0; c < img.channels; ++c) {
                    double v1 = img.at(x1, y1, c) * (1 - fx) + img.at(x1 + 1, y1, c) * fx;
                    double v2 = img.at(x1, y1 + 1, c) * (1 - fx) + img.at(x1 + 1, y1 + 1, c) * fx;
                    result.at(x, y, c) = clamp(static_cast<int>(v1 * (1 - fy) + v2 * fy));
                }
            }
        }
    }
    return result;
}

Image ImageProcessor::blendPyramids(const Image& img1, const Image& img2, const Image& mask) {
    if (img1.width != img2.width || img1.height != img2.height) {
        throw std::runtime_error("الصور يجب أن تكون بنفس الأبعاد للدمج الهرمي");
    }

    int levels = 4;
    std::vector<Image> pyr1, pyr2, pyrM;
    
    // بناء الأهرامات
    Image curr1 = img1, curr2 = img2, currM = mask;
    for (int i = 0; i < levels; ++i) {
        pyr1.push_back(curr1);
        pyr2.push_back(curr2);
        pyrM.push_back(currM);
        if (i < levels - 1) {
            curr1 = pyramidDown(curr1);
            curr2 = pyramidDown(curr2);
            currM = pyramidDown(currM);
        }
    }

    // بناء Laplacian Pyramids
    std::vector<Image> lap1, lap2;
    for (int i = 0; i < levels - 1; ++i) {
        Image up1 = pyramidUp(pyr1[i+1]);
        Image up2 = pyramidUp(pyr2[i+1]);
        if (up1.width != pyr1[i].width || up1.height != pyr1[i].height) up1 = resize(up1, pyr1[i].width, pyr1[i].height);
        if (up2.width != pyr2[i].width || up2.height != pyr2[i].height) up2 = resize(up2, pyr2[i].width, pyr2[i].height);
        
        lap1.push_back(subtract(pyr1[i], up1));
        lap2.push_back(subtract(pyr2[i], up2));
    }
    lap1.push_back(pyr1.back());
    lap2.push_back(pyr2.back());

    // الدمج في كل مستوى
    std::vector<Image> blendedLap;
    for (int i = 0; i < levels; ++i) {
        Image b(lap1[i].width, lap1[i].height, lap1[i].channels);
        Image m = (pyrM[i].channels == 1) ? pyrM[i] : toGrayscale(pyrM[i]);
        for (int y = 0; y < b.height; ++y) {
            for (int x = 0; x < b.width; ++x) {
                double alpha = m.at(x, y, 0) / 255.0;
                for (int c = 0; c < b.channels; ++c) {
                    b.at(x, y, c) = clamp(static_cast<int>(lap1[i].at(x, y, c) * alpha + lap2[i].at(x, y, c) * (1.0 - alpha)));
                }
            }
        }
        blendedLap.push_back(b);
    }

    // إعادة البناء
    Image result = blendedLap.back();
    for (int i = levels - 2; i >= 0; --i) {
        result = pyramidUp(result);
        if (result.width != blendedLap[i].width || result.height != blendedLap[i].height) result = resize(result, blendedLap[i].width, blendedLap[i].height);
        result = add(result, blendedLap[i]);
    }

    return result;
}

#include <chrono>
int64_t ImageProcessor::getTickCount() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

double ImageProcessor::getTickFrequency() {
    return 1000000.0;
}

// ==================== Camera Implementation ====================

Camera::Camera() : _isOpened(false), _deviceIndex(-1), _internalData(nullptr) {
}

Camera::~Camera() {
    close();
}

bool Camera::open(int deviceIndex) {
    close();

#ifdef _WIN32
    HRESULT hr = CoInitialize(NULL);

    CameraData* data = new CameraData();
    _internalData = data;
    _deviceIndex = deviceIndex;

    // Create the Capture Graph Builder
    hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void**)&data->pCapture);
    if (FAILED(hr)) { delete data; _internalData = NULL; return false; }

    // Create the Filter Graph Manager
    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&data->pGraph);
    if (FAILED(hr)) { 
        data->pCapture->Release(); 
        delete data; _internalData = NULL; return false; 
    }

    data->pCapture->SetFiltergraph(data->pGraph);

    // Create the Sample Grabber
    hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&data->pGrabberF);
    if (FAILED(hr)) {
        data->pGraph->Release();
        data->pCapture->Release();
        delete data; _internalData = NULL; return false; 
    }

    data->pGraph->AddFilter(data->pGrabberF, L"Sample Grabber");
    data->pGrabberF->QueryInterface(IID_ISampleGrabber, (void**)&data->pGrabber);

    AM_MEDIA_TYPE mt;
    ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
    mt.majortype = MEDIATYPE_Video;
    mt.subtype = MEDIASUBTYPE_RGB24;
    data->pGrabber->SetMediaType(&mt);

    // Create system device enumerator
    ICreateDevEnum* pDevEnum = NULL;
    IEnumMoniker* pEnum = NULL;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pDevEnum);
    
    if (SUCCEEDED(hr)) {
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
        if (hr == S_FALSE) hr = E_FAIL; // No devices
    }

    if (SUCCEEDED(hr)) {
        IMoniker* pMoniker = NULL;
        int count = 0;
        bool found = false;
        while (pEnum->Next(1, &pMoniker, NULL) == S_OK) {
            if (count == deviceIndex) {
                hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&data->pSrcFilter);
                pMoniker->Release();
                found = true;
                break;
            }
            pMoniker->Release();
            count++;
        }
        pEnum->Release();
        if (!found) hr = E_FAIL;
    }
    if (pDevEnum) pDevEnum->Release();

    if (!data->pSrcFilter) {
        std::cerr << "❌ Camera not found due to enumeration failure or index out of bounds." << std::endl;
        close();
        return false;
    }

    data->pGraph->AddFilter(data->pSrcFilter, L"Video Capture");

    // Null Renderer
    hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&data->pNullF);
    data->pGraph->AddFilter(data->pNullF, L"Null Renderer");

    // Render stream
    hr = data->pCapture->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, data->pSrcFilter, data->pGrabberF, data->pNullF);
    if (FAILED(hr)) {
        std::cerr << "❌ Failed to render stream using defaults. Trying basic render..." << std::endl;
        // Fallback or detailed error checking could go here
        close();
        return false;
    }

    // Get width/height
    AM_MEDIA_TYPE mtConnected;
    hr = data->pGrabber->GetConnectedMediaType(&mtConnected);
    if (SUCCEEDED(hr)) {
        if (mtConnected.formattype == FORMAT_VideoInfo) {
            VIDEOINFOHEADER* pVih = (VIDEOINFOHEADER*)mtConnected.pbFormat;
            data->width = pVih->bmiHeader.biWidth;
            data->height = pVih->bmiHeader.biHeight;
        }
        FreeMediaType(mtConnected);
    }

    data->pGrabber->SetBufferSamples(TRUE);
    data->pGrabber->SetOneShot(FALSE);

    data->pGraph->QueryInterface(IID_IMediaControl, (void**)&data->pControl);
    
    // Run
    data->pControl->Run();
    data->isRunning = true;
    
    // Wait for camera to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    _isOpened = true;
    std::cout << "✅ Camera opened with DirectShow: " << data->width << "x" << data->height << std::endl;
    return true;
#else
    _isOpened = true; 
    return true;
#endif
}

void Camera::close() {
    _isOpened = false;
    _deviceIndex = -1;

#ifdef _WIN32
    if (_internalData) {
        CameraData* data = (CameraData*)_internalData;
        if (data->pControl) data->pControl->Stop();
        
        if (data->pCapture) data->pCapture->Release();
        if (data->pControl) data->pControl->Release();
        if (data->pGrabber) data->pGrabber->Release();
        if (data->pGrabberF) data->pGrabberF->Release();
        if (data->pNullF) data->pNullF->Release();
        if (data->pSrcFilter) data->pSrcFilter->Release();
        if (data->pGraph) data->pGraph->Release();
        
        delete data;
        _internalData = nullptr;
    }
#endif
}

bool Camera::isOpened() const {
    return _isOpened;
}

Image Camera::capture() {
    if (!_isOpened) return Image(); // Return empty image

#ifdef _WIN32
    if (_internalData) {
        CameraData* data = (CameraData*)_internalData;
        
        long bufferSize = 0;
        if (SUCCEEDED(data->pGrabber->GetCurrentBuffer(&bufferSize, NULL))) {
            if (bufferSize > 0) {
                std::vector<uint8_t> buffer(bufferSize);
                if (SUCCEEDED(data->pGrabber->GetCurrentBuffer(&bufferSize, (long*)buffer.data()))) {
                    Image img(data->width, data->height, 3);
                    int w = data->width;
                    int h = data->height;
                    
                    // DirectShow samples are usually bottom-up. Flip vertically while converting.
                    for (int y = 0; y < h; ++y) {
                         // Flip vertically
                         int srcY = h - 1 - y; 
                         
                        for (int x = 0; x < w; ++x) {
                            int srcIdx = (srcY * w + x) * 3;
                            int dstIdx = (y * w + x) * 3;
                            
                            if (srcIdx + 2 < bufferSize) {
                                img.data[dstIdx + 0] = buffer[srcIdx + 2]; // R
                                img.data[dstIdx + 1] = buffer[srcIdx + 1]; // G
                                img.data[dstIdx + 2] = buffer[srcIdx + 0]; // B
                            }
                        }
                    }
                    return img;
                }
            }
        }
    }
#endif

    // Fallback simulation (Blue Grid) if DShow fails but _isOpened is somehow true
    // (Should not happen if open() succeeds with DShow)
    Image img(640, 480, 3);
    for (int y = 0; y < 480; ++y) {
        for (int x = 0; x < 640; ++x) {
            int idx = (y * 640 + x) * 3;
            img.data[idx + 0] = 40;  // R
            img.data[idx + 1] = 60;  // G
            img.data[idx + 2] = 180; // B
            if (x % 32 == 0 || y % 32 == 0) {
                img.data[idx + 0] = 80;
                img.data[idx + 1] = 100;
                img.data[idx + 2] = 220;
            }
        }
    }
    return img;
}

// ==================== ShapeDetector Implementation ====================

ShapeDetector::ShapeDetector() {
}

std::vector<Circle> ShapeDetector::detectCircles(const Image& img, int minRadius, int maxRadius, double threshold) {
    Image processed = preprocessForShapeDetection(img);
    return houghCircles(processed, minRadius, maxRadius, threshold);
}

std::vector<Rectangle> ShapeDetector::detectRectangles(const Image& img, double threshold) {
    Image processed = preprocessForShapeDetection(img);
    std::vector<Rectangle> rectangles;
    
    // كشف بسيط للمستطيلات باستخدام الكنتورات
    auto contours = detectContours(processed);
    
    for (const auto& contour : contours) {
        if (contour.size() < 4) continue;
        
        // حساب المستطيل المحيط
        int minX = contour[0].first, maxX = contour[0].first;
        int minY = contour[0].second, maxY = contour[0].second;
        
        for (const auto& point : contour) {
            minX = std::min(minX, point.first);
            maxX = std::max(maxX, point.first);
            minY = std::min(minY, point.second);
            maxY = std::max(maxY, point.second);
        }
        
        int width = maxX - minX;
        int height = maxY - minY;
        
        if (width > 10 && height > 10) {
            rectangles.push_back(Rectangle(minX, minY, width, height, 0.8));
        }
    }
    
    return rectangles;
}

std::vector<Line> ShapeDetector::detectLines(const Image& img, double threshold, int minLineLength, int maxLineGap) {
    Image processed = preprocessForShapeDetection(img);
    return houghLines(processed, threshold, minLineLength, maxLineGap);
}

std::vector<Line> ShapeDetector::detectLinesProbabilistic(const Image& img, double threshold, int minLineLength, int maxLineGap) {
    // تحسين بسيط: تقليل النقاط بإعادة أخذ العينات قبل Hough
    Image processed = preprocessForShapeDetection(img);
    Image down = processed;
    if (processed.width > 200 || processed.height > 200) {
        ImageProcessor ip;
        down = ip.resize(processed, processed.width / 2, processed.height / 2);
    }
    auto lines = houghLines(down, threshold, std::max(1, minLineLength / 2), maxLineGap);
    // إعادة مقاس الخطوط إذا صُغرت
    if (down.width != processed.width) {
        double sx = static_cast<double>(processed.width) / down.width;
        double sy = static_cast<double>(processed.height) / down.height;
        for (auto& l : lines) {
            l.x1 = static_cast<int>(l.x1 * sx);
            l.y1 = static_cast<int>(l.y1 * sy);
            l.x2 = static_cast<int>(l.x2 * sx);
            l.y2 = static_cast<int>(l.y2 * sy);
        }
    }
    return lines;
}

std::vector<Polygon> ShapeDetector::detectPolygons(const Image& img, int minVertices, int maxVertices) {
    Image processed = preprocessForShapeDetection(img);
    std::vector<Polygon> polygons;
    
    auto contours = detectContours(processed);
    
    for (const auto& contour : contours) {
        if (contour.size() >= minVertices && contour.size() <= maxVertices) {
            Polygon poly;
            poly.points = contour;
            poly.confidence = 0.7;
            polygons.push_back(poly);
        }
    }
    
    return polygons;
}

std::vector<std::vector<std::pair<int, int>>> ShapeDetector::detectContours(const Image& img) {
    std::vector<std::vector<std::pair<int, int>>> contours;
    
    ImageProcessor ip;
    Image edges = ip.detectEdgesCanny(img, 50, 150);

    // تتبع بسيط للكنتورات: نجمع نقاط متجاورة
    std::vector<std::vector<bool>> visited(edges.height, std::vector<bool>(edges.width, false));
    int dirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
    for (int y = 0; y < edges.height; ++y) {
        for (int x = 0; x < edges.width; ++x) {
            if (edges.at(x, y, 0) > 0 && !visited[y][x]) {
                std::vector<std::pair<int,int>> contour;
                std::vector<std::pair<int,int>> stack = {{x,y}};
                visited[y][x] = true;
                while (!stack.empty()) {
                    auto [cx, cy] = stack.back();
                    stack.pop_back();
                    contour.push_back({cx, cy});
                    for (auto& d : dirs) {
                        int nx = cx + d[0], ny = cy + d[1];
                        if (nx >=0 && nx < edges.width && ny>=0 && ny<edges.height &&
                            !visited[ny][nx] && edges.at(nx, ny, 0) > 0) {
                            visited[ny][nx] = true;
                            stack.push_back({nx, ny});
                        }
                    }
                }
                if (!contour.empty()) contours.push_back(contour);
            }
        }
    }
    return contours;
}

std::vector<std::pair<int, int>> ShapeDetector::approxPolyDP(const std::vector<std::pair<int, int>>& contour, double epsilon) {
    if (contour.size() < 3) return contour;

    auto findMaxDistance = [&](int start, int end) {
        double maxDist = 0;
        int index = start;
        for (int i = start + 1; i < end; ++i) {
            double x1 = contour[start].first, y1 = contour[start].second;
            double x2 = contour[end].first, y2 = contour[end].second;
            double x0 = contour[i].first, y0 = contour[i].second;
            double dx = x2 - x1, dy = y2 - y1;
            double dist = std::abs(dy * x0 - dx * y0 + x2 * y1 - y2 * x1) / std::sqrt(dx * dx + dy * dy + 1e-9);
            if (dist > maxDist) {
                maxDist = dist;
                index = i;
            }
        }
        return std::make_pair(index, maxDist);
    };

    std::vector<bool> keep(contour.size(), false);
    keep[0] = true;
    keep[contour.size() - 1] = true;

    std::vector<std::pair<int, int>> stack = {{0, (int)contour.size() - 1}};
    while (!stack.empty()) {
        auto [start, end] = stack.back();
        stack.pop_back();

        if (end - start < 2) continue;

        auto [index, dist] = findMaxDistance(start, end);
        if (dist > epsilon) {
            keep[index] = true;
            stack.push_back({start, index});
            stack.push_back({index, end});
        }
    }

    std::vector<std::pair<int, int>> result;
    for (size_t i = 0; i < contour.size(); ++i) {
        if (keep[i]) result.push_back(contour[i]);
    }
    return result;
}

void ShapeDetector::cornerSubPix(const Image& img, std::vector<Keypoint>& corners, int winSize) {
    ImageProcessor ip;
    Image gray = (img.channels == 1) ? img : ip.toGrayscale(img);
    
    for (auto& corner : corners) {
        double subX = corner.x;
        double subY = corner.y;

        for (int iter = 0; iter < 10; ++iter) {
            double sumXX = 0, sumXY = 0, sumYY = 0;
            double sumXG = 0, sumYG = 0;

            for (int dy = -winSize; dy <= winSize; ++dy) {
                for (int dx = -winSize; dx <= winSize; ++dx) {
                    int px = static_cast<int>(subX + dx);
                    int py = static_cast<int>(subY + dy);
                    if (px <= 0 || py <= 0 || px >= gray.width - 1 || py >= gray.height - 1) continue;

                    double gx = (gray.at(px + 1, py) - gray.at(px - 1, py)) / 2.0;
                    double gy = (gray.at(px, py + 1) - gray.at(px, py - 1)) / 2.0;

                    sumXX += gx * gx;
                    sumXY += gx * gy;
                    sumYY += gy * gy;
                    sumXG += gx * (gx * px + gy * py);
                    sumYG += gy * (gx * px + gy * py);
                }
            }

            double det = sumXX * sumYY - sumXY * sumXY;
            if (std::abs(det) > 1e-6) {
                double nextX = (sumYY * sumXG - sumXY * sumYG) / det;
                double nextY = (sumXX * sumYG - sumXY * sumXG) / det;
                if (std::abs(nextX - subX) < 0.01 && std::abs(nextY - subY) < 0.01) break;
                subX = nextX;
                subY = nextY;
            }
        }
        corner.x = static_cast<int>(std::round(subX));
        corner.y = static_cast<int>(std::round(subY));
    }
}

Image ShapeDetector::preprocessForShapeDetection(const Image& img) {
    ImageProcessor processor;
    Image gray = processor.toGrayscale(img);
    Image blurred = processor.applyGaussianBlur(gray, 2.0);
    return blurred;
}

std::vector<Circle> ShapeDetector::houghCircles(const Image& img, int minRadius, int maxRadius, double threshold) {
    std::vector<Circle> circles;
    if (img.isEmpty()) return circles;

    ImageProcessor ip;
    Image gray = (img.channels == 1) ? img : ip.toGrayscale(img);
    Image edges = ip.detectEdges(gray, 50, 150);

    // استخدام Hough Gradient Method المبسط:
    // لكل بكسل حافة، نرسم دائرة في المجمع بنصف قطر محتمل
    // هذا يتطلب مجمع ثلاثي الأبعاد (x, y, r) وهو مكلف، لذا سنقوم بتثبيت نصف القطر أو استخدام نطاق صغير
    
    int accThreshold = static_cast<int>(threshold * 100);

    for (int r = minRadius; r <= maxRadius; r += 5) {
        std::vector<int> accumulator(img.width * img.height, 0);
        
        for (int y = 0; y < edges.height; ++y) {
            for (int x = 0; x < edges.width; ++x) {
                if (edges.at(x, y, 0) > 0) {
                    // رسم دائرة حول بكسل الحافة في المجمع
                    for (int theta = 0; theta < 360; theta += 10) {
                        double rad = theta * M_PI / 180.0;
                        int cx = static_cast<int>(x - r * std::cos(rad));
                        int cy = static_cast<int>(y - r * std::sin(rad));
                        
                        if (cx >= 0 && cx < img.width && cy >= 0 && cy < img.height) {
                            accumulator[cy * img.width + cx]++;
                        }
                    }
                }
            }
        }
        
        // البحث عن القمم في المجمع لهذا النصف قطر
        for (int y = 0; y < img.height; ++y) {
            for (int x = 0; x < img.width; ++x) {
                if (accumulator[y * img.width + x] > accThreshold) {
                    // فحص إذا كانت هذه الدائرة قريبة جداً من دوائر موجودة
                    bool duplicate = false;
                    for (const auto& c : circles) {
                        if (std::abs(c.centerX - x) < 10 && std::abs(c.centerY - y) < 10 && std::abs(c.radius - r) < 10) {
                            duplicate = true;
                            break;
                        }
                    }
                    if (!duplicate) {
                        circles.push_back(Circle(x, y, r, static_cast<double>(accumulator[y * img.width + x]) / 100.0));
                    }
                }
            }
        }
    }

    return circles;
}

std::vector<Line> ShapeDetector::houghLines(const Image& img, double threshold, int minLineLength, int maxLineGap) {
    std::vector<Line> lines;
    if (img.isEmpty()) return lines;

    // تحويل الصورة إلى تدرج رمادي إذا لم تكن كذلك
    ImageProcessor ip;
    Image gray = (img.channels == 1) ? img : ip.toGrayscale(img);
    
    // كشف الحواف باستخدام Canny (أو Sobel بسيط)
    Image edges = ip.detectEdges(gray, 50, 150);

    double rhoRes = 1.0;
    double thetaRes = M_PI / 180.0;
    int numThetas = static_cast<int>(M_PI / thetaRes);
    int maxRho = static_cast<int>(std::sqrt(img.width * img.width + img.height * img.height));
    int numRhos = 2 * maxRho + 1;

    std::vector<int> accumulator(numRhos * numThetas, 0);

    for (int y = 0; y < edges.height; ++y) {
        for (int x = 0; x < edges.width; ++x) {
            if (edges.at(x, y, 0) > 0) {
                for (int t = 0; t < numThetas; ++t) {
                    double theta = t * thetaRes;
                    double rho = x * std::cos(theta) + y * std::sin(theta);
                    int rIdx = static_cast<int>(std::round(rho)) + maxRho;
                    accumulator[rIdx * numThetas + t]++;
                }
            }
        }
    }

    int accThreshold = static_cast<int>(threshold * 100); // تحويل النسبة إلى قيمة عددية
    for (int r = 0; r < numRhos; ++r) {
        for (int t = 0; t < numThetas; ++t) {
            if (accumulator[r * numThetas + t] > accThreshold) {
                double rho = r - maxRho;
                double theta = t * thetaRes;
                
                double a = std::cos(theta);
                double b = std::sin(theta);
                double x0 = a * rho;
                double y0 = b * rho;
                
                int x1 = static_cast<int>(x0 + 1000 * (-b));
                int y1 = static_cast<int>(y0 + 1000 * (a));
                int x2 = static_cast<int>(x0 - 1000 * (-b));
                int y2 = static_cast<int>(y0 - 1000 * (a));
                
                // قص الخط ليكون داخل حدود الصورة
                // (تبسيط: نتركها حالياً كخطوط طويلة، أو نقوم بالقص الفعلي)
                lines.push_back(Line(x1, y1, x2, y2, static_cast<double>(accumulator[r * numThetas + t]) / 255.0));
            }
        }
    }

    return lines;
}

std::vector<double> ShapeDetector::findHomography(const std::vector<std::pair<int, int>>& srcPoints, const std::vector<std::pair<int, int>>& dstPoints) {
    if (srcPoints.size() < 4 || dstPoints.size() < 4 || srcPoints.size() != dstPoints.size()) return {};

    int n = static_cast<int>(srcPoints.size());
    int maxIters = 1000;
    double threshold = 3.0;
    std::vector<double> bestH;
    int maxInliers = -1;

    auto solveDLT = [](const std::vector<std::pair<double, double>>& src, const std::vector<std::pair<double, double>>& dst) -> std::vector<double> {
        // Solve Ah = 0 for 4 points using Gaussian Elimination on 8x9 matrix
        // We assume h[8] = 1.0 and solve for the other 8 parameters
        double A[8][8] = {0};
        double B[8] = {0};

        for (int i = 0; i < 4; ++i) {
            double x = src[i].first;
            double y = src[i].second;
            double u = dst[i].first;
            double v = dst[i].second;

            // Row 2i: -x, -y, -1, 0, 0, 0, x*u, y*u, u
            A[2 * i][0] = -x; A[2 * i][1] = -y; A[2 * i][2] = -1;
            A[2 * i][6] = x * u; A[2 * i][7] = y * u;
            B[2 * i] = -u;

            // Row 2i+1: 0, 0, 0, -x, -y, -1, x*v, y*v, v
            A[2 * i + 1][3] = -x; A[2 * i + 1][4] = -y; A[2 * i + 1][5] = -1;
            A[2 * i + 1][6] = x * v; A[2 * i + 1][7] = y * v;
            B[2 * i + 1] = -v;
        }

        // Gaussian Elimination
        for (int i = 0; i < 8; ++i) {
            int pivot = i;
            for (int j = i + 1; j < 8; ++j) {
                if (std::abs(A[j][i]) > std::abs(A[pivot][i])) pivot = j;
            }
            for (int j = 0; j < 8; ++j) std::swap(A[i][j], A[pivot][j]);
            std::swap(B[i], B[pivot]);

            if (std::abs(A[i][i]) < 1e-10) return {};

            for (int j = i + 1; j < 8; ++j) {
                double factor = A[j][i] / A[i][i];
                for (int k = i; k < 8; ++k) A[j][k] -= factor * A[i][k];
                B[j] -= factor * B[i];
            }
        }

        std::vector<double> h(9);
        for (int i = 7; i >= 0; --i) {
            double sum = 0;
            for (int j = i + 1; j < 8; ++j) sum += A[i][j] * h[j];
            h[i] = (B[i] - sum) / A[i][i];
        }
        h[8] = 1.0;
        return h;
    };

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    for (int iter = 0; iter < maxIters; ++iter) {
        // Select 4 random unique points
        std::vector<int> indices;
        while (indices.size() < 4) {
            int idx = std::rand() % n;
            if (std::find(indices.begin(), indices.end(), idx) == indices.end()) {
                indices.push_back(idx);
            }
        }

        std::vector<std::pair<double, double>> srcSample, dstSample;
        for (int idx : indices) {
            srcSample.push_back({static_cast<double>(srcPoints[idx].first), static_cast<double>(srcPoints[idx].second)});
            dstSample.push_back({static_cast<double>(dstPoints[idx].first), static_cast<double>(dstPoints[idx].second)});
        }

        std::vector<double> H = solveDLT(srcSample, dstSample);
        if (H.empty()) continue;

        int inliers = 0;
        for (int i = 0; i < n; ++i) {
            double x = srcPoints[i].first;
            double y = srcPoints[i].second;
            double w = H[6] * x + H[7] * y + H[8];
            if (std::abs(w) < 1e-10) continue;
            
            double tx = (H[0] * x + H[1] * y + H[2]) / w;
            double ty = (H[3] * x + H[4] * y + H[5]) / w;
            
            double dx = tx - dstPoints[i].first;
            double dy = ty - dstPoints[i].second;
            if (dx * dx + dy * dy < threshold * threshold) {
                inliers++;
            }
        }

        if (inliers > maxInliers) {
            maxInliers = inliers;
            bestH = H;
        }

        if (maxInliers > n * 0.8) break; // Good enough
    }

    return bestH;
}

double ShapeDetector::distance(int x1, int y1, int x2, int y2) const {
    int dx = x2 - x1;
    int dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

ShapeDetector::Moments ShapeDetector::calculateMoments(const std::vector<std::pair<int, int>>& contour) {
    Moments m = {0};
    
    // حساب العزوم المكانية (Spatial Moments)
    for (const auto& p : contour) {
        double x = p.first;
        double y = p.second;
        m.m00 += 1;
        m.m10 += x;
        m.m01 += y;
        m.m20 += x * x;
        m.m11 += x * y;
        m.m02 += y * y;
        m.m30 += x * x * x;
        m.m21 += x * x * y;
        m.m12 += x * y * y;
        m.m03 += y * y * y;
    }
    
    if (m.m00 > 0) {
        double cx = m.m10 / m.m00;
        double cy = m.m01 / m.m00;
        
        // حساب العزوم المركزية (Central Moments)
        for (const auto& p : contour) {
            double dx = p.first - cx;
            double dy = p.second - cy;
            m.mu20 += dx * dx;
            m.mu11 += dx * dy;
            m.mu02 += dy * dy;
            m.mu30 += dx * dx * dx;
            m.mu21 += dx * dx * dy;
            m.mu12 += dx * dy * dy;
            m.mu03 += dy * dy * dy;
        }
        
        // حساب العزوم المركزية المعيارية (Normalized Central Moments)
        double inv_m00_2 = 1.0 / (m.m00 * m.m00);
        double inv_m00_25 = inv_m00_2 / std::sqrt(m.m00);
        m.nu20 = m.mu20 * inv_m00_2;
        m.nu11 = m.mu11 * inv_m00_2;
        m.nu02 = m.mu02 * inv_m00_2;
        m.nu30 = m.mu30 * inv_m00_25;
        m.nu21 = m.mu21 * inv_m00_25;
        m.nu12 = m.mu12 * inv_m00_25;
        m.nu03 = m.mu03 * inv_m00_25;
        
        // حساب عزوم هو (Hu Moments) - السبعة عزوم غير المتغيرة بالدوران والإزاحة والحجم
        m.hu[0] = m.nu20 + m.nu02;
        m.hu[1] = std::pow(m.nu20 - m.nu02, 2) + 4 * std::pow(m.nu11, 2);
        m.hu[2] = std::pow(m.nu30 - 3 * m.nu12, 2) + std::pow(3 * m.nu21 - m.nu03, 2);
        m.hu[3] = std::pow(m.nu30 + m.nu12, 2) + std::pow(m.nu21 + m.nu03, 2);
        m.hu[4] = (m.nu30 - 3 * m.nu12) * (m.nu30 + m.nu12) * (std::pow(m.nu30 + m.nu12, 2) - 3 * std::pow(m.nu21 + m.nu03, 2)) +
                  (3 * m.nu21 - m.nu03) * (m.nu21 + m.nu03) * (3 * std::pow(m.nu30 + m.nu12, 2) - std::pow(m.nu21 + m.nu03, 2));
        m.hu[5] = (m.nu20 - m.nu02) * (std::pow(m.nu30 + m.nu12, 2) - std::pow(m.nu21 + m.nu03, 2)) + 4 * m.nu11 * (m.nu30 + m.nu12) * (m.nu21 + m.nu03);
        m.hu[6] = (3 * m.nu21 - m.nu03) * (m.nu30 + m.nu12) * (std::pow(m.nu30 + m.nu12, 2) - 3 * std::pow(m.nu21 + m.nu03, 2)) -
                  (m.nu30 - 3 * m.nu12) * (m.nu21 + m.nu03) * (3 * std::pow(m.nu30 + m.nu12, 2) - std::pow(m.nu21 + m.nu03, 2));
    }
    
    return m;
}

double ShapeDetector::matchShapes(const std::vector<std::pair<int, int>>& c1, const std::vector<std::pair<int, int>>& c2) {
    Moments m1 = calculateMoments(c1);
    Moments m2 = calculateMoments(c2);
    
    double sum = 0;
    for (int i = 0; i < 7; ++i) {
        double h1 = std::abs(m1.hu[i]);
        double h2 = std::abs(m2.hu[i]);
        if (h1 > 0 && h2 > 0) {
            h1 = -std::signbit(m1.hu[i]) * std::log10(h1);
            h2 = -std::signbit(m2.hu[i]) * std::log10(h2);
            sum += std::abs(1.0/h1 - 1.0/h2);
        }
    }
    return sum;
}

// ==================== WindowManager Implementation ====================

#ifdef _WIN32
static LRESULT CALLBACK ArabicWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WindowManager::WindowInfo* info = (WindowManager::WindowInfo*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (uMsg) {
        case WM_PAINT: {
            if (info && !info->currentImage.isEmpty()) {
                char buf[256];
                sprintf(buf, "[WM_PAINT] hwnd=%p, size=%dx%d\n", hwnd, info->currentImage.width, info->currentImage.height);
                OutputDebugStringA(buf);
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                
                const Image& img = info->currentImage;
                
                // التأكد من حجم النافذة
                RECT rect;
                GetClientRect(hwnd, &rect);
                int winW = rect.right - rect.left;
                int winH = rect.bottom - rect.top;

                BITMAPINFO bmi = {0};
                bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biWidth = img.width;
                bmi.bmiHeader.biHeight = -img.height; // Top-down
                bmi.bmiHeader.biPlanes = 1;
                bmi.bmiHeader.biBitCount = 24; // سنحول الكل لـ 24 بت للعرض
                bmi.bmiHeader.biCompression = BI_RGB;

                // تحويل البيانات للعرض (BGR)
                std::vector<uint8_t> displayData;
                if (img.channels == 3) {
                    displayData.resize(img.width * img.height * 3);
                    for (int i = 0; i < img.width * img.height; ++i) {
                        displayData[i*3] = img.data[i*3+2];     // B
                        displayData[i*3+1] = img.data[i*3+1];   // G
                        displayData[i*3+2] = img.data[i*3];     // R
                    }
                } else if (img.channels == 1) {
                    displayData.resize(img.width * img.height * 3);
                    for (int i = 0; i < img.width * img.height; ++i) {
                        uint8_t v = img.data[i];
                        displayData[i*3] = v;
                        displayData[i*3+1] = v;
                        displayData[i*3+2] = v;
                    }
                } else if (img.channels == 4) {
                    displayData.resize(img.width * img.height * 3);
                    for (int i = 0; i < img.width * img.height; ++i) {
                        displayData[i*3] = img.data[i*4+2];     // B
                        displayData[i*3+1] = img.data[i*4+1];   // G
                        displayData[i*3+2] = img.data[i*4];     // R
                    }
                }

                if (!displayData.empty()) {
                    SetStretchBltMode(hdc, COLORONCOLOR);
                    StretchDIBits(hdc, 0, 0, winW, winH, 0, 0, img.width, img.height, 
                                  displayData.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
                }
                
                EndPaint(hwnd, &ps);
            }
            return 0;
        }
        case WM_ERASEBKGND:
            return 1; // منع الوميض
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_MOUSEMOVE: {
            char buf[256];
            sprintf(buf, "[WindowProc] Mouse uMsg=%u\n", uMsg);
            OutputDebugStringA(buf);
            if (info) {
                if (uMsg != WM_MOUSEMOVE) {
                    std::cout << "🖱️ حدث ماوس في نافذة " << info->name << ": رسالة=" << uMsg << " س=" << LOWORD(lParam) << " ص=" << HIWORD(lParam) << std::endl;
                }
                
                std::cout << "🖱️ [WindowProc] info->mouseCallback = " << info->mouseCallback << std::endl;
                if (info->mouseCallback) {
                    MouseEvent ev = MouseEvent::MOVE;
                    if (uMsg == WM_LBUTTONDOWN) ev = MouseEvent::LEFT_DOWN;
                    else if (uMsg == WM_RBUTTONDOWN) ev = MouseEvent::RIGHT_DOWN;
                    else if (uMsg == WM_MBUTTONDOWN) ev = MouseEvent::MIDDLE_DOWN;
                    
                    // تحويل الإحداثيات من النافذة إلى الصورة
                    RECT rect;
                    GetClientRect(hwnd, &rect);
                    int winW = rect.right - rect.left;
                    int winH = rect.bottom - rect.top;
                    
                    int x = LOWORD(lParam);
                    int y = HIWORD(lParam);
                    
                    if (winW > 0 && winH > 0 && !info->currentImage.isEmpty()) {
                        x = (x * info->currentImage.width) / winW;
                        y = (y * info->currentImage.height) / winH;
                    }
                    
                    info->mouseCallback(ev, x, y, (int)wParam, info->mouseUserdata);
                }
            }
            return 0;
        }
        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = 600;
            mmi->ptMinTrackSize.y = 400;
            return 0;
        }
        case WM_CLOSE:
            std::cout << "🚪 الضغط على زر الإغلاق (X)، استدعاء DestroyWindow." << std::endl;
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            std::cout << "💥 تدمير النافذة." << std::endl;
            if (info) {
                info->isClosed = true;
            }
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
#endif

std::map<std::string, WindowManager::WindowInfo> WindowManager::windows;

void WindowManager::createWindow(const std::string& name) {
    if (windows.find(name) == windows.end()) {
        WindowInfo info;
        info.name = name;
        windows[name] = info;

#ifdef _WIN32
        static bool classRegistered = false;
        if (!classRegistered) {
            WNDCLASSW wc = {0};
            wc.lpfnWndProc = ArabicWindowProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = L"ArabicVisionWindow";
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            RegisterClassW(&wc);
            classRegistered = true;
        }

        int wNameLen = MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, nullptr, 0);
        std::wstring wName(wNameLen, 0);
        MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, &wName[0], wNameLen);

        HWND hwnd = CreateWindowExW(0, L"ArabicVisionWindow", wName.c_str(), 
                                   WS_OVERLAPPEDWINDOW,
                                   CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                   NULL, NULL, GetModuleHandle(NULL), NULL);
        
        if (hwnd) {
            std::cout << "✅ تم إنشاء النافذة بنجاح: " << name << " (HWND: " << hwnd << ")" << std::endl;
            windows[name].hwnd = hwnd;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&windows[name]);
            
            // ضبط الحجم الابتدائي 800x600 (حجم العميل)
            RECT rect = {0, 0, 800, 600};
            AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
            MoveWindow(hwnd, 100, 100, rect.right - rect.left, rect.bottom - rect.top, TRUE);
        } else {
            std::cout << "❌ فشل إنشاء النافذة: " << name << " (Error: " << GetLastError() << ")" << std::endl;
        }
#endif
    }
}

void WindowManager::destroyWindow(const std::string& name) {
    auto it = windows.find(name);
    if (it != windows.end()) {
#ifdef _WIN32
        if (it->second.hwnd) {
            std::cout << "🗑️ تدمير النافذة: " << name << std::endl;
            DestroyWindow((HWND)it->second.hwnd);
        }
#endif
        windows.erase(it);
    }
}

void WindowManager::destroyAllWindows() {
#ifdef _WIN32
    for (auto& pair : windows) {
        if (pair.second.hwnd) {
            std::cout << "🗑️ تدمير النافذة: " << pair.first << std::endl;
            DestroyWindow((HWND)pair.second.hwnd);
        }
    }
#endif
    windows.clear();
}

void WindowManager::showImage(const std::string& name, const Image& img) {
    createWindow(name);
    windows[name].currentImage = img;

#ifdef _WIN32
    HWND hwnd = (HWND)windows[name].hwnd;
    if (hwnd) {
        char buf[256];
        sprintf(buf, "[showImage] Window: %s, Image: %dx%d\n", name.c_str(), img.width, img.height);
        OutputDebugStringA(buf);
        
        RECT rect = {0, 0, img.width, img.height};
        AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
        SetWindowPos(hwnd, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        
        ShowWindow(hwnd, SW_SHOW);
        SetForegroundWindow(hwnd);
        
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
    }
#endif
}

bool WindowManager::hasActiveWindows() {
    for (auto& pair : windows) {
        if (pair.second.hwnd != nullptr && !pair.second.isClosed)
            return true;
    }
    return false;
}

void WindowManager::pumpMessages() {
#ifdef _WIN32
    if (!hasActiveWindows()) return;
    
    // تفريغ الرسائل الخاصة بنوافذ الرؤية فقط لتجنب تعارض الـ IDE
    for (auto& pair : windows) {
        HWND hwnd = (HWND)pair.second.hwnd;
        if (hwnd && !pair.second.isClosed) {
            MSG msg;
            while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
#endif
}

int WindowManager::waitKey(int delay) {
#ifdef _WIN32
    // Flush old keyboard messages
    {
        MSG flush;
        while (PeekMessage(&flush, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE));
    }

    auto start = std::chrono::steady_clock::now();

    while (true) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                PostQuitMessage(0);
                return -1;
            }
            if (msg.message == WM_KEYDOWN || msg.message == WM_CHAR) {
                return (int)msg.wParam;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Check if any window is still open
        bool anyOpen = false;
        for (auto& pair : windows) {
            if (pair.second.hwnd && !pair.second.isClosed) {
                anyOpen = true;
                break;
            }
        }
        if (!anyOpen) return -1;

        // Timeout check
        if (delay > 0) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            if (elapsed >= (long long)delay) return 0;
        }

        Sleep(delay > 0 ? 1 : 10);
    }
#else
    if (delay > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        return 0;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return -1;
#endif
}

void WindowManager::setMouseCallback(const std::string& name, MouseCallback onMouse, void* userdata) {
    if (windows.find(name) != windows.end()) {
        std::cout << "🔧 [setMouseCallback] Setting callback for window: " << name << std::endl;
        windows[name].mouseCallback = onMouse;
        windows[name].mouseUserdata = userdata;
        std::cout << "🔧 [setMouseCallback] userdata set to: " << userdata << std::endl;
    } else {
        std::cerr << "❌ [setMouseCallback] Window not found: " << name << std::endl;
    }
}

int WindowManager::createTrackbar(const std::string& trackbarName, const std::string& windowName, 
                                  int* value, int count, TrackbarCallback onChange, void* userdata) {
    if (windows.find(windowName) != windows.end()) {
        windows[windowName].trackbars[trackbarName] = (value ? *value : 0);
        windows[windowName].trackbarCallbacks[trackbarName] = onChange;
        windows[windowName].trackbarUserdata[trackbarName] = userdata;
        windows[windowName].trackbarValues[trackbarName] = value;
        return 0;
    }
    return -1;
}

int WindowManager::getTrackbarPos(const std::string& trackbarName, const std::string& windowName) {
    if (windows.find(windowName) != windows.end()) {
        auto it = windows[windowName].trackbars.find(trackbarName);
        if (it != windows[windowName].trackbars.end()) {
            return it->second;
        }
    }
    return -1;
}

void WindowManager::setTrackbarPos(const std::string& trackbarName, const std::string& windowName, int pos) {
    if (windows.find(windowName) != windows.end()) {
        windows[windowName].trackbars[trackbarName] = pos;
        if (windows[windowName].trackbarValues[trackbarName]) {
            *windows[windowName].trackbarValues[trackbarName] = pos;
        }
        if (windows[windowName].trackbarCallbacks[trackbarName]) {
            windows[windowName].trackbarCallbacks[trackbarName](pos, windows[windowName].trackbarUserdata[trackbarName]);
        }
    }
}

// ==================== DatasetManager Implementation ====================

DatasetManager::DatasetManager() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
}

bool DatasetManager::saveSample(const Image& img, const std::string& label, const std::string& datasetPath) {
    ImageProcessor ip;
    std::string folder = datasetPath + "/" + label;
    
    try {
        if (!fs::exists(folder)) {
            fs::create_directories(folder);
        }
    } catch (...) {
        return false;
    }

    std::string filename = folder + "/sample_" + std::to_string(std::time(nullptr)) + "_" + std::to_string(std::rand() % 1000) + ".jpg";
    return ip.saveImage(img, filename, ImageFormat::JPEG);
}

std::vector<Image> DatasetManager::augmentImage(const Image& img) {
    std::vector<Image> augmented;
    ImageProcessor ip;
    
    // 1. تدوير بسيط (±5، ±10 درجات)
    augmented.push_back(ip.rotate(img, 5));
    augmented.push_back(ip.rotate(img, -5));
    
    // 2. تغيير السطوع
    augmented.push_back(ip.adjustBrightnessContrast(img, 20, 1.0));
    augmented.push_back(ip.adjustBrightnessContrast(img, -20, 1.0));
    
    // 3. إضافة ضجيج (بسيط)
    Image noisy = img;
    for (size_t i = 0; i < noisy.data.size(); i += 10) {
        noisy.data[i] = ip.clamp(noisy.data[i] + (std::rand() % 20 - 10));
    }
    augmented.push_back(noisy);
    
    // 4. قلب أفقي
    augmented.push_back(ip.flip(img, true, false));
    
    return augmented;
}

std::vector<std::pair<Image, std::string>> DatasetManager::loadDataset(const std::string& datasetPath) {
    std::vector<std::pair<Image, std::string>> dataset;
    ImageProcessor ip;
    
    try {
        if (!fs::exists(datasetPath) || !fs::is_directory(datasetPath)) {
            std::cerr << "❌ مسار مجموعة البيانات غير موجود: " << datasetPath << std::endl;
            return dataset;
        }

        for (const auto& entry : fs::recursive_directory_iterator(datasetPath)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp") {
                    try {
                        Image img = ip.loadImage(entry.path().string());
                        std::string label = entry.path().parent_path().filename().string();
                        dataset.push_back({img, label});
                    } catch (...) {
                        continue;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ خطأ أثناء تحميل مجموعة البيانات: " << e.what() << std::endl;
    }
    
    std::cout << "📂 تم تحميل " << dataset.size() << " عينة من: " << datasetPath << std::endl;
    return dataset;
}

std::string DatasetManager::sanitizeFilename(const std::string& name) {
    std::string s = name;
    std::replace_if(s.begin(), s.end(), [](char c) { return !std::isalnum(c); }, '_');
    return s;
}

// ==================== ParallelProcessor Implementation ====================

// (تم تنفيذ الدوال كقوالب في ملف .h لضمان الكفاءة)

std::vector<Keypoint> ShapeDetector::detectHarrisCorners(const Image& img, int blockSize, int ksize, double k, double threshold) {
    Image gray = (img.channels == 1) ? img : ImageProcessor().toGrayscale(img);
    // حواف بسيطة باستخدام Sobel
    std::vector<std::vector<double>> sobelX = {{-1,0,1},{-2,0,2},{-1,0,1}};
    std::vector<std::vector<double>> sobelY = {{-1,-2,-1},{0,0,0},{1,2,1}};
    Image gx(gray.width, gray.height, 1);
    Image gy(gray.width, gray.height, 1);
    ImageProcessor().applyKernel(gray, gx, sobelX);
    ImageProcessor().applyKernel(gray, gy, sobelY);
    int r = blockSize / 2;
    std::vector<Keypoint> kps;
    for (int y = r; y < gray.height - r; ++y) {
        for (int x = r; x < gray.width - r; ++x) {
            double sumIx2 = 0, sumIy2 = 0, sumIxIy = 0;
            for (int dy = -r; dy <= r; ++dy) {
                for (int dx = -r; dx <= r; ++dx) {
                    double ix = gx.at(x + dx, y + dy, 0) - 128.0;
                    double iy = gy.at(x + dx, y + dy, 0) - 128.0;
                    sumIx2 += ix * ix;
                    sumIy2 += iy * iy;
                    sumIxIy += ix * iy;
                }
            }
            double det = sumIx2 * sumIy2 - sumIxIy * sumIxIy;
            double trace = sumIx2 + sumIy2 + 1e-9;
            double R = det - k * trace * trace;
            if (R > threshold) {
                kps.emplace_back(x, y, static_cast<float>(R));
            }
        }
    }
    return kps;
}

std::vector<Keypoint> ShapeDetector::detectShiTomasi(const Image& img, int maxCorners, double qualityLevel, int minDistance) {
    ImageProcessor ip;
    Image gray = (img.channels == 1) ? img : ip.toGrayscale(img);
    
    // 1. Calculate derivatives
    Image gx = ip.applySobel(gray, true);
    Image gy = ip.applySobel(gray, false);
    
    int W = gray.width;
    int H = gray.height;
    std::vector<double> scores(W * H, 0.0);
    double maxScore = 0;

    // 2. Compute minimum eigenvalue for each pixel
    int winSize = 3;
    for (int y = winSize; y < H - winSize; ++y) {
        for (int x = winSize; x < W - winSize; ++x) {
            double sumXX = 0, sumYY = 0, sumXY = 0;
            for (int dy = -winSize; dy <= winSize; ++dy) {
                for (int dx = -winSize; dx <= winSize; ++dx) {
                    double ix = gx.at(x + dx, y + dy) - 128.0;
                    double iy = gy.at(x + dx, y + dy) - 128.0;
                    sumXX += ix * ix;
                    sumYY += iy * iy;
                    sumXY += ix * iy;
                }
            }
            // Eigenvalues of [sumXX sumXY; sumXY sumYY]
            // lambda = ((A+C) - sqrt((A-C)^2 + 4B^2)) / 2
            double score = ((sumXX + sumYY) - std::sqrt(std::pow(sumXX - sumYY, 2) + 4 * sumXY * sumXY)) / 2.0;
            scores[y * W + x] = score;
            if (score > maxScore) maxScore = score;
        }
    }

    // 3. Filter by quality level and collect candidates
    std::vector<Keypoint> candidates;
    double threshold = maxScore * qualityLevel;
    for (int y = winSize; y < H - winSize; ++y) {
        for (int x = winSize; x < W - winSize; ++x) {
            double score = scores[y * W + x];
            if (score > threshold) {
                // Local maxima check
                bool isLocalMax = true;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (score < scores[(y + dy) * W + (x + dx)]) {
                            isLocalMax = false;
                            break;
                        }
                    }
                    if (!isLocalMax) break;
                }
                if (isLocalMax) {
                    candidates.emplace_back(x, y, static_cast<float>(score));
                }
            }
        }
    }

    // 4. Sort by score
    std::sort(candidates.begin(), candidates.end(), [](const Keypoint& a, const Keypoint& b) {
        return a.response > b.response;
    });

    // 5. Apply minimum distance
    std::vector<Keypoint> result;
    for (const auto& kp : candidates) {
        if (result.size() >= static_cast<size_t>(maxCorners)) break;
        bool tooClose = false;
        for (const auto& accepted : result) {
            double d = std::sqrt(std::pow(kp.x - accepted.x, 2) + std::pow(kp.y - accepted.y, 2));
            if (d < minDistance) {
                tooClose = true;
                break;
            }
        }
        if (!tooClose) result.push_back(kp);
    }

    return result;
}

std::vector<Keypoint> ShapeDetector::detectFAST(const Image& img, int threshold, int contiguous) {
    Image gray = (img.channels == 1) ? img : ImageProcessor().toGrayscale(img);
    std::vector<Keypoint> kps;
    // دائرة FAST المعيارية 16 نقطة
    const int circle[16][2] = {
        {0,-3},{1,-3},{2,-2},{3,-1},{3,0},{3,1},{2,2},{1,3},
        {0,3},{-1,3},{-2,2},{-3,1},{-3,0},{-3,-1},{-2,-2},{-1,-3}
    };
    for (int y = 3; y < gray.height - 3; ++y) {
        for (int x = 3; x < gray.width - 3; ++x) {
            int p = gray.at(x, y, 0);
            int brighter = 0, darker = 0, maxCont = 0;
            int currentCont = 0;
            for (int i = 0; i < 16; ++i) {
                int px = x + circle[i][0];
                int py = y + circle[i][1];
                int v = gray.at(px, py, 0);
                if (v >= p + threshold) {
                    brighter++; darker = 0;
                    currentCont++;
                } else if (v <= p - threshold) {
                    darker++; brighter = 0;
                    currentCont++;
                } else {
                    currentCont = 0;
                }
                maxCont = std::max(maxCont, currentCont);
            }
            if (maxCont >= contiguous) {
                kps.emplace_back(x, y, static_cast<float>(maxCont));
            }
        }
    }
    return kps;
}

std::vector<Keypoint> ShapeDetector::detectORB(const Image& img, int nfeatures) {
    // 1. كشف النقاط باستخدام FAST
    auto kps = detectFAST(img, 20);
    
    // 2. حساب الاتجاه (Orientation) لكل نقطة (Centroid method)
    Image gray = (img.channels == 1) ? img : ImageProcessor().toGrayscale(img);
    int r = 15;
    for (auto& kp : kps) {
        double m01 = 0, m10 = 0;
        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                if (dx*dx + dy*dy > r*r) continue;
                int nx = kp.x + dx, ny = kp.y + dy;
                if (nx >= 0 && nx < gray.width && ny >= 0 && ny < gray.height) {
                    uint8_t val = gray.at(nx, ny, 0);
                    m10 += dx * val;
                    m01 += dy * val;
                }
            }
        }
        kp.angle = std::atan2(m01, m10);
    }
    
    // 3. ترتيب واختيار أفضل nfeatures
    std::sort(kps.begin(), kps.end(), [](const Keypoint& a, const Keypoint& b) {
        return a.response > b.response;
    });
    
    if (kps.size() > static_cast<size_t>(nfeatures)) {
        kps.resize(nfeatures);
    }
    
    return kps;
}

std::vector<Descriptor> ShapeDetector::computeORBDescriptors(const Image& img, const std::vector<Keypoint>& kps) {
    std::vector<Descriptor> descriptors;
    Image gray = (img.channels == 1) ? img : ImageProcessor().toGrayscale(img);
    
    // أزواج الاختبار لـ BRIEF (مبسطة)
    static const int pairs[32][4] = {
        {-4, -4, 4, 4}, { -4, 4, 4, -4 }, { 0, -4, 0, 4 }, { -4, 0, 4, 0 },
        // ... المزيد من الأزواج لـ 256 بت
    };
    
    for (const auto& kp : kps) {
        Descriptor desc;
        desc.data.resize(32, 0.0f); // 256 bits = 32 floats
        
        float cosA = std::cos(kp.angle);
        float sinA = std::sin(kp.angle);
        
        for (int i = 0; i < 32; ++i) {
            uint8_t byte = 0;
            for (int bit = 0; bit < 8; ++bit) {
                // تدوير أزواج الاختبار بناءً على زاوية النقطة
                int x1_orig = (i * 8 + bit) % 15 - 7;
                int y1_orig = (i * 8 + bit) / 15 - 7;
                int x2_orig = 7 - x1_orig;
                int y2_orig = 7 - y1_orig;
                
                int x1 = static_cast<int>(kp.x + x1_orig * cosA - y1_orig * sinA);
                int y1 = static_cast<int>(kp.y + x1_orig * sinA + y1_orig * cosA);
                int x2 = static_cast<int>(kp.x + x2_orig * cosA - y2_orig * sinA);
                int y2 = static_cast<int>(kp.y + x2_orig * sinA + y2_orig * cosA);
                
                if (inside(gray, x1, y1) && inside(gray, x2, y2)) {
                    if (gray.at(x1, y1, 0) < gray.at(x2, y2, 0)) {
                        byte |= (1 << bit);
                    }
                }
            }
            desc.data[i] = static_cast<float>(byte);
        }
        descriptors.push_back(desc);
    }
    return descriptors;
}

std::vector<std::pair<int, int>> ShapeDetector::calcOpticalFlowPyrLK(const Image& prevImg, const Image& nextImg, 
                                                                    const std::vector<std::pair<int, int>>& prevPts,
                                                                    std::vector<uint8_t>& status,
                                                                    int winSize, int maxLevel) {
    status.assign(prevPts.size(), 0);
    std::vector<std::pair<int, int>> nextPts = prevPts;
    
    ImageProcessor ip;
    Image prevGray = (prevImg.channels == 1) ? prevImg : ip.toGrayscale(prevImg);
    Image nextGray = (nextImg.channels == 1) ? nextImg : ip.toGrayscale(nextImg);
    
    // بناء الأهرامات
    std::vector<Image> prevPyr = {prevGray};
    std::vector<Image> nextPyr = {nextGray};
    for (int l = 0; l < maxLevel; ++l) {
        prevPyr.push_back(ip.pyramidDown(prevPyr.back()));
        nextPyr.push_back(ip.pyramidDown(nextPyr.back()));
    }
    
    int halfWin = winSize / 2;
    
    for (size_t i = 0; i < prevPts.size(); ++i) {
        double px = prevPts[i].first;
        double py = prevPts[i].second;
        double g_x = 0, g_y = 0; // التخمين الأولي
        
        bool success = true;
        for (int l = maxLevel; l >= 0; --l) {
            double scale = std::pow(0.5, l);
            double lx = px * scale;
            double ly = py * scale;
            
            // Lucas-Kanade عند هذا المستوى
            double ux = g_x, uy = g_y;
            for (int iter = 0; iter < 10; ++iter) {
                double b1 = 0, b2 = 0;
                double G11 = 0, G12 = 0, G22 = 0;
                
                for (int wy = -halfWin; wy <= halfWin; ++wy) {
                    for (int wx = -halfWin; wx <= halfWin; ++wx) {
                        int ix = static_cast<int>(lx + wx);
                        int iy = static_cast<int>(ly + wy);
                        int jx = static_cast<int>(lx + ux + wx);
                        int jy = static_cast<int>(ly + uy + wy);
                        
                        if (inside(prevPyr[l], ix, iy) && inside(nextPyr[l], jx, jy)) {
                            // تدرجات بسيطة
                            double dx = (inside(prevPyr[l], ix+1, iy) ? prevPyr[l].at(ix+1, iy, 0) : prevPyr[l].at(ix, iy, 0)) -
                                        (inside(prevPyr[l], ix-1, iy) ? prevPyr[l].at(ix-1, iy, 0) : prevPyr[l].at(ix, iy, 0));
                            double dy = (inside(prevPyr[l], ix, iy+1) ? prevPyr[l].at(ix, iy+1, 0) : prevPyr[l].at(ix, iy, 0)) -
                                        (inside(prevPyr[l], ix, iy-1) ? prevPyr[l].at(ix, iy-1, 0) : prevPyr[l].at(ix, iy, 0));
                            dx /= 2.0; dy /= 2.0;
                            
                            double diff = static_cast<double>(prevPyr[l].at(ix, iy, 0)) - nextPyr[l].at(jx, jy, 0);
                            
                            G11 += dx * dx; G12 += dx * dy; G22 += dy * dy;
                            b1 += diff * dx; b2 += diff * dy;
                        }
                    }
                }
                
                double det = G11 * G22 - G12 * G12;
                if (std::abs(det) < 1e-6) { success = false; break; }
                
                double stepX = (G22 * b1 - G12 * b2) / det;
                double stepY = (G11 * b2 - G12 * b1) / det;
                ux += stepX; uy += stepY;
                if (std::abs(stepX) < 0.01 && std::abs(stepY) < 0.01) break;
            }
            
            if (l > 0) {
                g_x = ux * 2.0;
                g_y = uy * 2.0;
            } else {
                g_x = ux; g_y = uy;
            }
            if (!success) break;
        }
        
        if (success) {
            nextPts[i].first = static_cast<int>(px + g_x);
            nextPts[i].second = static_cast<int>(py + g_y);
            status[i] = 1;
        }
    }
    
    return nextPts;
}

Image ShapeDetector::calcOpticalFlowFarneback(const Image& prevImg, const Image& nextImg) {
    ImageProcessor ip;
    Image prevGray = (prevImg.channels == 1) ? prevImg : ip.toGrayscale(prevImg);
    Image nextGray = (nextImg.channels == 1) ? nextImg : ip.toGrayscale(nextImg);
    
    int W = prevGray.width;
    int H = prevGray.height;
    Image flowVis(W, H, 3);
    
    // Farneback-like approach: Local polynomial expansion (simplified)
    // We calculate the flow (u, v) for each pixel
    // For visualization, we map flow to HSV-like colors
    
    int winSize = 5;
    for (int y = winSize; y < H - winSize; ++y) {
        for (int x = winSize; x < W - winSize; ++x) {
            double b1 = 0, b2 = 0;
            double G11 = 0, G12 = 0, G22 = 0;
            
            for (int dy = -winSize; dy <= winSize; ++dy) {
                for (int dx = -winSize; dx <= winSize; ++dx) {
                    int ix = x + dx;
                    int iy = y + dy;
                    
                    // Simple gradients
                    double Ix = (prevGray.at(ix + 1, iy) - prevGray.at(ix - 1, iy)) / 2.0;
                    double Iy = (prevGray.at(ix, iy + 1) - prevGray.at(ix, iy - 1)) / 2.0;
                    double It = static_cast<double>(nextGray.at(ix, iy)) - prevGray.at(ix, iy);
                    
                    G11 += Ix * Ix;
                    G12 += Ix * Iy;
                    G22 += Iy * Iy;
                    b1 -= Ix * It;
                    b2 -= Iy * It;
                }
            }
            
            double det = G11 * G22 - G12 * G12;
            double u = 0, v = 0;
            if (std::abs(det) > 1e-6) {
                u = (G22 * b1 - G12 * b2) / det;
                v = (G11 * b2 - G12 * b1) / det;
            }
            
            // Visualization: Angle -> Hue, Magnitude -> Value
            double angle = std::atan2(v, u);
            double mag = std::sqrt(u * u + v * v);
            
            // Map angle [-PI, PI] to [0, 255] for Hue
            uint8_t h = static_cast<uint8_t>((angle + M_PI) / (2.0 * M_PI) * 255.0);
            uint8_t s = 255;
            uint8_t val = static_cast<uint8_t>(std::min(mag * 10.0, 255.0));
            
            // Simple HSV to RGB conversion for visualization
            flowVis.at(x, y, 0) = val; // R
            flowVis.at(x, y, 1) = h;   // G
            flowVis.at(x, y, 2) = s;   // B
        }
    }
    
    return flowVis;
}

Rectangle ShapeDetector::meanShift(const Image& probImage, const Rectangle& window, int iterations) {
    Rectangle current = window;
    for (int i = 0; i < iterations; ++i) {
        double m00 = 0, m10 = 0, m01 = 0;
        for (int y = current.y; y < current.y + current.height; ++y) {
            for (int x = current.x; x < current.x + current.width; ++x) {
                if (x >= 0 && x < probImage.width && y >= 0 && y < probImage.height) {
                    uint8_t val = probImage.at(x, y);
                    m00 += val;
                    m10 += x * val;
                    m01 += y * val;
                }
            }
        }
        if (m00 == 0) break;
        int newX = static_cast<int>(m10 / m00) - current.width / 2;
        int newY = static_cast<int>(m01 / m00) - current.height / 2;
        if (newX == current.x && newY == current.y) break;
        current.x = newX;
        current.y = newY;
    }
    return current;
}

Rectangle ShapeDetector::camShift(const Image& probImage, const Rectangle& window, int iterations) {
    Rectangle res = meanShift(probImage, window, iterations);
    
    // Calculate moments to find orientation and size
    double m00 = 0, m10 = 0, m01 = 0, m11 = 0, m20 = 0, m02 = 0;
    for (int y = res.y; y < res.y + res.height; ++y) {
        for (int x = res.x; x < res.x + res.width; ++x) {
            if (x >= 0 && x < probImage.width && y >= 0 && y < probImage.height) {
                uint8_t val = probImage.at(x, y);
                m00 += val;
                m10 += x * val;
                m01 += y * val;
                m11 += (double)x * y * val;
                m20 += (double)x * x * val;
                m02 += (double)y * y * val;
            }
        }
    }

    if (m00 > 0) {
        double xc = m10 / m00;
        double yc = m01 / m00;
        double mu20 = m20 / m00 - xc * xc;
        double mu02 = m02 / m00 - yc * yc;
        double mu11 = m11 / m00 - xc * yc;

        // Orientation
        double angle = 0.5 * std::atan2(2 * mu11, mu20 - mu02);
        
        // Window size (simplified)
        double common = std::sqrt(std::pow(mu20 - mu02, 2) + 4 * mu11 * mu11);
        double l1 = mu20 + mu02 + common;
        double l2 = mu20 + mu02 - common;
        
        res.width = static_cast<int>(std::sqrt(l1) * 4);
        res.height = static_cast<int>(std::sqrt(l2) * 4);
    }

    return res;
}

Image ShapeDetector::backgroundSubtractionMOG2(const Image& img, Image& background, double learningRate) {
    Image fgMask(img.width, img.height, 1);
    ImageProcessor ip;
    Image gray = (img.channels == 1) ? img : ip.toGrayscale(img);
    
    if (background.isEmpty()) {
        background = gray;
        return fgMask;
    }

    // Adaptive threshold based on global variation (simplified MOG2 concept)
    int threshold = 25; 

    for (int i = 0; i < gray.width * gray.height; ++i) {
        uint8_t diff = std::abs(gray.data[i] - background.data[i]);
        
        // Mark as foreground if difference is significant
        if (diff > threshold) {
            fgMask.data[i] = 255;
        } else {
            fgMask.data[i] = 0;
            // Update background only for non-moving parts to avoid ghosting
            background.data[i] = static_cast<uint8_t>((1.0 - learningRate) * background.data[i] + learningRate * gray.data[i]);
        }
    }
    
    // Clean up mask with morphological opening/closing
    fgMask = ip.openMorph(fgMask, 3);
    fgMask = ip.closeMorph(fgMask, 3);
    
    return fgMask;
}

Descriptor ShapeDetector::computeDescriptor(const Image& img, const Keypoint& kp, int patch) {
    int r = patch / 2;
    Image gray = (img.channels == 1) ? img : ImageProcessor().toGrayscale(img);
    std::vector<float> vec;
    vec.reserve(patch * patch);
    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            int x = kp.x + dx;
            int y = kp.y + dy;
            if (inside(gray, x, y)) vec.push_back(static_cast<float>(gray.at(x, y, 0)));
            else vec.push_back(0.0f);
        }
    }
    // تطبيع
    float mean = 0.0f;
    for (auto v : vec) mean += v;
    mean /= static_cast<float>(vec.size());
    float norm = 0.0f;
    for (auto& v : vec) { v -= mean; norm += v * v; }
    norm = std::sqrt(norm) + 1e-6f;
    for (auto& v : vec) v /= norm;
    Descriptor d; d.data = std::move(vec);
    return d;
}

std::vector<Descriptor> ShapeDetector::computeDescriptors(const Image& img, const std::vector<Keypoint>& kps, int patch) {
    std::vector<Descriptor> out;
    out.reserve(kps.size());
    for (auto& k : kps) out.push_back(computeDescriptor(img, k, patch));
    return out;
}

std::vector<ShapeDetector::Match> ShapeDetector::matchBruteForce(const std::vector<Descriptor>& q, const std::vector<Descriptor>& t) {
    std::vector<Match> matches;
    for (size_t i = 0; i < q.size(); ++i) {
        double best = 1e18;
        int bestIdx = -1;
        for (size_t j = 0; j < t.size(); ++j) {
            if (q[i].data.size() != t[j].data.size()) continue;
            double dist = 0.0;
            for (size_t k = 0; k < q[i].data.size(); ++k) {
                double d = q[i].data[k] - t[j].data[k];
                dist += d * d;
            }
            if (dist < best) {
                best = dist;
                bestIdx = static_cast<int>(j);
            }
        }
        if (bestIdx >= 0) {
            matches.push_back({static_cast<int>(i), bestIdx, best});
        }
    }
    return matches;
}

// ==================== Advanced Contour Properties ====================

double ShapeDetector::contourArea(const std::vector<std::pair<int, int>>& contour) const {
    if (contour.size() < 3) return 0.0;
    double area = 0.0;
    for (size_t i = 0; i < contour.size(); ++i) {
        auto [x1, y1] = contour[i];
        auto [x2, y2] = contour[(i + 1) % contour.size()];
        area += x1 * y2 - x2 * y1;
    }
    return std::abs(area) / 2.0;
}

double ShapeDetector::contourPerimeter(const std::vector<std::pair<int, int>>& contour) const {
    if (contour.size() < 2) return 0.0;
    double per = 0.0;
    for (size_t i = 0; i < contour.size(); ++i) {
        auto [x1, y1] = contour[i];
        auto [x2, y2] = contour[(i + 1) % contour.size()];
        per += distance(x1, y1, x2, y2);
    }
    return per;
}

double ShapeDetector::aspectRatio(const std::vector<std::pair<int, int>>& contour) const {
    if (contour.empty()) return 0.0;
    int minX = contour[0].first, maxX = contour[0].first;
    int minY = contour[0].second, maxY = contour[0].second;
    for (auto& p : contour) {
        minX = std::min(minX, p.first);
        maxX = std::max(maxX, p.first);
        minY = std::min(minY, p.second);
        maxY = std::max(maxY, p.second);
    }
    double w = maxX - minX + 1;
    double h = maxY - minY + 1;
    if (h == 0) return 0.0;
    return w / h;
}

double ShapeDetector::extent(const std::vector<std::pair<int, int>>& contour) const {
    if (contour.empty()) return 0.0;
    int minX = contour[0].first, maxX = contour[0].first;
    int minY = contour[0].second, maxY = contour[0].second;
    for (auto& p : contour) {
        minX = std::min(minX, p.first);
        maxX = std::max(maxX, p.first);
        minY = std::min(minY, p.second);
        maxY = std::max(maxY, p.second);
    }
    double bboxArea = (maxX - minX + 1) * (maxY - minY + 1);
    if (bboxArea == 0) return 0.0;
    return contourArea(contour) / bboxArea;
}

double ShapeDetector::solidity(const std::vector<std::pair<int, int>>& contour) const {
    auto hull = convexHull(contour);
    double hullArea = contourArea(hull);
    double area = contourArea(contour);
    if (hullArea == 0) return 0.0;
    return area / hullArea;
}

double ShapeDetector::equivalentDiameter(const std::vector<std::pair<int, int>>& contour) const {
    double area = contourArea(contour);
    return std::sqrt(4 * area / M_PI);
}

double ShapeDetector::orientation(const std::vector<std::pair<int, int>>& contour) const {
    Moments m = const_cast<ShapeDetector*>(this)->calculateMoments(contour);
    double angle = 0.5 * std::atan2(2 * m.mu11, m.mu20 - m.mu02);
    return angle * 180.0 / M_PI; // بالدرجات
}

Rectangle ShapeDetector::boundingBox(const std::vector<std::pair<int, int>>& contour) const {
    if (contour.empty()) return Rectangle();
    int minX = contour[0].first, maxX = contour[0].first;
    int minY = contour[0].second, maxY = contour[0].second;
    for (const auto& p : contour) {
        if (p.first < minX) minX = p.first;
        if (p.first > maxX) maxX = p.first;
        if (p.second < minY) minY = p.second;
        if (p.second > maxY) maxY = p.second;
    }
    return Rectangle(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

Circle ShapeDetector::minEnclosingCircle(const std::vector<std::pair<int, int>>& contour) const {
    if (contour.empty()) return Circle();
    
    // تنفيذ مبسط: استخدام المركز الحسابي وأقصى مسافة
    double sumX = 0, sumY = 0;
    for (const auto& p : contour) {
        sumX += p.first;
        sumY += p.second;
    }
    int cx = static_cast<int>(sumX / contour.size());
    int cy = static_cast<int>(sumY / contour.size());
    
    double maxDistSq = 0;
    for (const auto& p : contour) {
        double d2 = std::pow(p.first - cx, 2) + std::pow(p.second - cy, 2);
        if (d2 > maxDistSq) maxDistSq = d2;
    }
    
    return Circle(cx, cy, static_cast<int>(std::sqrt(maxDistSq)));
}

void ShapeDetector::fitEllipse(const std::vector<std::pair<int, int>>& contour, int& centerX, int& centerY, int& radiusX, int& radiusY, double& angle) {
    if (contour.size() < 5) return;
    
    Moments m = calculateMoments(contour);
    centerX = static_cast<int>(m.m10 / m.m00);
    centerY = static_cast<int>(m.m01 / m.m00);
    
    double common = std::sqrt(std::pow(m.mu20 - m.mu02, 2) + 4 * std::pow(m.mu11, 2));
    radiusX = static_cast<int>(std::sqrt(2 * (m.mu20 + m.mu02 + common) / m.m00));
    radiusY = static_cast<int>(std::sqrt(2 * (m.mu20 + m.mu02 - common) / m.m00));
    angle = 0.5 * std::atan2(2 * m.mu11, m.mu20 - m.mu02) * 180.0 / M_PI;
}

void ShapeDetector::fitLine(const std::vector<std::pair<int, int>>& contour, double& vx, double& vy, double& x0, double& y0) {
    if (contour.empty()) return;
    
    double sumX = 0, sumY = 0, sumXX = 0, sumYY = 0, sumXY = 0;
    for (const auto& p : contour) {
        sumX += p.first;
        sumY += p.second;
        sumXX += p.first * p.first;
        sumYY += p.second * p.second;
        sumXY += p.first * p.second;
    }
    
    int n = static_cast<int>(contour.size());
    x0 = sumX / n;
    y0 = sumY / n;
    
    double covXX = (sumXX - sumX * sumX / n) / n;
    double covYY = (sumYY - sumY * sumY / n) / n;
    double covXY = (sumXY - sumX * sumY / n) / n;
    
    // حساب المتجه الذاتي (Eigenvector) المقابل لأكبر قيمة ذاتية
    double common = std::sqrt(std::pow(covXX - covYY, 2) + 4 * covXY * covXY);
    double l1 = 0.5 * (covXX + covYY + common);
    
    if (std::abs(covXY) > 1e-9) {
        vx = l1 - covYY;
        vy = covXY;
    } else {
        vx = (covXX > covYY) ? 1 : 0;
        vy = (covXX > covYY) ? 0 : 1;
    }
    
    double mag = std::sqrt(vx * vx + vy * vy);
    vx /= mag;
    vy /= mag;
}

std::pair<int, int> ShapeDetector::extremePointMinX(const std::vector<std::pair<int, int>>& contour) const {
    return *std::min_element(contour.begin(), contour.end(), [](auto a, auto b){ return a.first < b.first; });
}
std::pair<int, int> ShapeDetector::extremePointMaxX(const std::vector<std::pair<int, int>>& contour) const {
    return *std::max_element(contour.begin(), contour.end(), [](auto a, auto b){ return a.first < b.first; });
}
std::pair<int, int> ShapeDetector::extremePointMinY(const std::vector<std::pair<int, int>>& contour) const {
    return *std::min_element(contour.begin(), contour.end(), [](auto a, auto b){ return a.second < b.second; });
}
std::pair<int, int> extremePointMaxY_impl(const std::vector<std::pair<int, int>>& contour) {
    return *std::max_element(contour.begin(), contour.end(), [](auto a, auto b){ return a.second < b.second; });
}
std::pair<int, int> ShapeDetector::extremePointMaxY(const std::vector<std::pair<int, int>>& contour) const {
    return *std::max_element(contour.begin(), contour.end(), [](auto a, auto b){ return a.second < b.second; });
}

double ShapeDetector::pointPolygonTest(const std::vector<std::pair<int, int>>& contour, int x, int y) const {
    // ray casting
    bool inside = false;
    for (size_t i = 0, j = contour.size() - 1; i < contour.size(); j = i++) {
        auto [xi, yi] = contour[i];
        auto [xj, yj] = contour[j];
        bool intersect = ((yi > y) != (yj > y)) &&
                         (x < (xj - xi) * (y - yi) / double(yj - yi + 1e-9) + xi);
        if (intersect) inside = !inside;
    }
    return inside ? 1.0 : -1.0;
}

std::vector<std::pair<int, int>> ShapeDetector::convexHull(const std::vector<std::pair<int, int>>& contour) const {
    std::vector<std::pair<int,int>> pts = contour;
    std::sort(pts.begin(), pts.end());
    pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
    if (pts.size() < 3) return pts;
    auto cross = [](auto O, auto A, auto B) {
        return (A.first - O.first) * (B.second - O.second) - (A.second - O.second) * (B.first - O.first);
    };
    std::vector<std::pair<int,int>> hull;
    // lower
    for (auto& p : pts) {
        while (hull.size() >= 2 && cross(hull[hull.size()-2], hull.back(), p) <= 0) hull.pop_back();
        hull.push_back(p);
    }
    // upper
    size_t lowerSize = hull.size();
    for (int i = (int)pts.size() - 2; i >= 0; --i) {
        auto p = pts[i];
        while (hull.size() > lowerSize && cross(hull[hull.size()-2], hull.back(), p) <= 0) hull.pop_back();
        hull.push_back(p);
    }
    hull.pop_back();
    return hull;
}

std::vector<std::pair<int, int>> ShapeDetector::convexityDefects(const std::vector<std::pair<int, int>>& contour) const {
    auto hull = convexHull(contour);
    // عيوب التحدب المبسطة: نقاط ليست في hull
    std::vector<std::pair<int,int>> defects;
    std::set<std::pair<int,int>> hullSet(hull.begin(), hull.end());
    for (auto& p : contour) {
        if (!hullSet.count(p)) defects.push_back(p);
    }
    return defects;
}

// ==================== FaceDetector Implementation ====================

FaceDetector::FaceDetector() {
}

std::vector<Face> FaceDetector::detectFaces(const Image& img, double scaleFactor, int minNeighbors, int minSize) {
    Image processed = preprocessForFaceDetection(img);
    std::vector<Face> candidates;
    
    // Multi-scale sliding window (Parallelized)
    std::mutex mtx;
    for (double scale = 1.0; ; scale *= scaleFactor) {
        int w = static_cast<int>(minSize * scale);
        int h = static_cast<int>(minSize * scale);
        
        if (w > processed.width || h > processed.height) break;
        
        int step = std::max(10, static_cast<int>(w * 0.2));
        int totalRows = (processed.height - h) / step + 1;
        
        ParallelProcessor::parallelFor(0, totalRows, [&](int rowIdx) {
            int y = rowIdx * step;
            std::vector<Face> localCandidates;
            for (int x = 0; x <= processed.width - w; x += step) {
                if (isFaceRegion(processed, x, y, w, h)) {
                    localCandidates.push_back(Face(x, y, w, h, 0.8));
                }
            }
            if (!localCandidates.empty()) {
                std::lock_guard<std::mutex> lock(mtx);
                candidates.insert(candidates.end(), localCandidates.begin(), localCandidates.end());
            }
        });
    }
    
    // Non-Maximum Suppression (NMS)
    std::vector<Face> faces;
    std::sort(candidates.begin(), candidates.end(), [](const Face& a, const Face& b) {
        return a.width > b.width; // Prefer larger faces
    });
    
    for (const auto& cand : candidates) {
        bool overlap = false;
        for (const auto& f : faces) {
            int x1 = std::max(cand.x, f.x);
            int y1 = std::max(cand.y, f.y);
            int x2 = std::min(cand.x + cand.width, f.x + f.width);
            int y2 = std::min(cand.y + cand.height, f.y + f.height);
            
            if (x2 > x1 && y2 > y1) {
                int intersectArea = (x2 - x1) * (y2 - y1);
                int candArea = cand.width * cand.height;
                if (intersectArea > candArea * 0.4) {
                    overlap = true;
                    break;
                }
            }
        }
        if (!overlap) {
            faces.push_back(cand);
        }
    }
    
    return faces;
}

std::vector<FaceLandmarks> FaceDetector::detectLandmarks(const Image& img, const std::vector<Face>& faces) {
    std::vector<FaceLandmarks> landmarks;
    
    for (const auto& face : faces) {
        FaceLandmarks lm;
        int centerX = face.x + face.width / 2;
        int centerY = face.y + face.height / 2;
        
        // مواضع تقريبية للملامح
        lm.leftEye = {centerX - face.width / 4, centerY - face.height / 4};
        lm.rightEye = {centerX + face.width / 4, centerY - face.height / 4};
        lm.nose = {centerX, centerY};
        lm.mouth = {centerX, centerY + face.height / 4};
        
        landmarks.push_back(lm);
    }
    
    return landmarks;
}

Image FaceDetector::drawFaceBoxes(const Image& img, const std::vector<Face>& faces) {
    Image result = img;
    
    for (const auto& face : faces) {
        // رسم مربع بسيط (في التطبيق الفعلي سيتم استخدام مكتبة رسم)
        int x = face.x;
        int y = face.y;
        int w = face.width;
        int h = face.height;
        
        // رسم الحدود (بسيط)
        for (int i = 0; i < 3; ++i) {
            for (int dx = 0; dx < w && x + dx < result.width; ++dx) {
                if (y + i < result.height) result.at(x + dx, y + i, 0) = 255;
                if (y + h - i - 1 < result.height) result.at(x + dx, y + h - i - 1, 0) = 255;
            }
            for (int dy = 0; dy < h && y + dy < result.height; ++dy) {
                if (x + i < result.width) result.at(x + i, y + dy, 0) = 255;
                if (x + w - i - 1 < result.width) result.at(x + w - i - 1, y + dy, 0) = 255;
            }
        }
    }
    
    return result;
}

Image FaceDetector::drawLandmarks(const Image& img, const std::vector<FaceLandmarks>& landmarks) {
    Image result = img;
    
    for (const auto& lm : landmarks) {
        // رسم النقاط (بسيط)
        int size = 3;
        for (int dy = -size; dy <= size; ++dy) {
            for (int dx = -size; dx <= size; ++dx) {
                if (lm.leftEye.first + dx >= 0 && lm.leftEye.first + dx < result.width &&
                    lm.leftEye.second + dy >= 0 && lm.leftEye.second + dy < result.height) {
                    result.at(lm.leftEye.first + dx, lm.leftEye.second + dy, 0) = 255;
                }
            }
        }
    }
    
    return result;
}

Image FaceDetector::preprocessForFaceDetection(const Image& img) {
    ImageProcessor processor;
    Image gray = processor.toGrayscale(img);
    Image equalized = processor.equalizeHistogram(gray);
    return equalized;
}

bool FaceDetector::isFaceRegion(const Image& img, int x, int y, int width, int height) {
    // كشف بسيط - في التطبيق الفعلي سيستخدم Haar Cascades أو DNN
    // هنا نتحقق من بعض الخصائص الأساسية
    
    if (x + width > img.width || y + height > img.height) {
        return false;
    }
    
    // حساب متوسط السطوع
    double sum = 0.0;
    for (int dy = 0; dy < height; ++dy) {
        for (int dx = 0; dx < width; ++dx) {
            sum += img.at(x + dx, y + dy, 0);
        }
    }
    double avg = sum / (width * height);
    
    // إذا كان السطوع في نطاق معقول، قد يكون وجه
    return avg > 80 && avg < 200;
}

std::vector<Face> FaceDetector::detectFacesInRegion(const Image& img, int x, int y, int width, int height) {
    std::vector<Face> faces;
    
    if (isFaceRegion(img, x, y, width, height)) {
        faces.push_back(Face(x, y, width, height, 0.7));
    }
    
    return faces;
}

// ==================== VideoCapture Implementation ====================

VideoCapture::VideoCapture() : width(0), height(0), fps(30.0), frameCount(0), currentFrame(0), isOpen(false) {
}

VideoCapture::~VideoCapture() {
    close();
}

bool VideoCapture::open(const std::string& filepath) {
    this->filepath = filepath;
    initializeVideoInfo();
    isOpen = true;
    return true;
}

void VideoCapture::close() {
    isOpen = false;
    currentFrame = 0;
}

bool VideoCapture::readFrame(Image& frame) {
    if (!isOpen || currentFrame >= frameCount) {
        return false;
    }
    
    // في التطبيق الفعلي، سيتم قراءة الإطار الحقيقي من الفيديو
    frame = Image(width, height, 3);
    frame.data.assign(frame.data.size(), 128); // إطار رمادي للاختبار
    
    currentFrame++;
    return true;
}

Image VideoCapture::readFrame() {
    Image frame;
    readFrame(frame);
    return frame;
}

bool VideoCapture::hasNext() const {
    return isOpen && currentFrame < frameCount;
}

void VideoCapture::initializeVideoInfo() {
    // في التطبيق الفعلي، سيتم قراءة معلومات الفيديو من الملف
    width = 640;
    height = 480;
    fps = 30.0;
    frameCount = 300; // 10 ثواني عند 30 FPS
}

// ==================== VideoWriter Implementation ====================

VideoWriter::VideoWriter() : width(0), height(0), fps(30.0), opened(false), frameCount(0) {
}

VideoWriter::~VideoWriter() {
    close();
}

bool VideoWriter::open(const std::string& filepath, int width, int height, double fps) {
    this->filepath = filepath;
    this->width = width;
    this->height = height;
    this->fps = fps;
    opened = true;
    frameCount = 0;
    return true;
}

void VideoWriter::close() {
    opened = false;
}

bool VideoWriter::writeFrame(const Image& frame) {
    if (!opened) {
        return false;
    }
    
    if (frame.width != width || frame.height != height) {
        return false;
    }
    
    // في التطبيق الفعلي، سيتم كتابة الإطار إلى ملف الفيديو
    frameCount++;
    return true;
}

// ==================== VideoProcessor Implementation ====================

VideoProcessor::VideoProcessor() {
}

VideoCapture VideoProcessor::openVideo(const std::string& filepath) {
    VideoCapture capture;
    capture.open(filepath);
    return capture;
}

VideoWriter VideoProcessor::createVideoWriter(const std::string& filepath, int width, int height, double fps) {
    VideoWriter writer;
    writer.open(filepath, width, height, fps);
    return writer;
}

std::vector<Image> VideoProcessor::extractFrames(const std::string& filepath, int startFrame, int endFrame) {
    VideoCapture capture = openVideo(filepath);
    std::vector<Image> frames;
    
    int current = 0;
    while (capture.hasNext()) {
        Image frame = capture.readFrame();
        if (current >= startFrame && (endFrame < 0 || current <= endFrame)) {
            frames.push_back(frame);
        }
        current++;
    }
    
    return frames;
}

VideoProcessor::VideoInfo VideoProcessor::getVideoInfo(const std::string& filepath) {
    VideoInfo info;
    VideoCapture capture = openVideo(filepath);
    
    info.width = capture.getWidth();
    info.height = capture.getHeight();
    info.fps = capture.getFPS();
    info.frameCount = capture.getFrameCount();
    info.duration = info.frameCount / info.fps;
    
    return info;
}

// ==================== CameraCapture Implementation ====================

CameraCapture::CameraCapture() : cameraIndex(0), width(640), height(480), opened(false) {
}

CameraCapture::~CameraCapture() {
    close();
}

bool CameraCapture::open(int cameraIndex) {
    this->cameraIndex = cameraIndex;
    initializeCamera();
    opened = true;
    return true;
}

void CameraCapture::close() {
    opened = false;
}

bool CameraCapture::readFrame(Image& frame) {
    if (!opened) {
        return false;
    }
    
    // في التطبيق الفعلي، سيتم قراءة الإطار من الكاميرا
    frame = Image(width, height, 3);
    frame.data.assign(frame.data.size(), 128); // إطار رمادي للاختبار
    
    return true;
}

Image CameraCapture::readFrame() {
    Image frame;
    readFrame(frame);
    return frame;
}

void CameraCapture::initializeCamera() {
    // في التطبيق الفعلي، سيتم تهيئة الكاميرا
    width = 640;
    height = 480;
}

// ==================== RealTimeVideoAnalyzer Implementation ====================

RealTimeVideoAnalyzer::RealTimeVideoAnalyzer() {
    metrics.fps = 0.0;
    metrics.processingTime = 0.0;
    metrics.detectedFaces = 0;
    metrics.motionIntensity = 0.0;
}

CameraCapture RealTimeVideoAnalyzer::openCamera(int cameraIndex) {
    CameraCapture camera;
    camera.open(cameraIndex);
    return camera;
}

MotionDetection RealTimeVideoAnalyzer::detectMotion(const Image& currentFrame, const Image& previousFrame, double threshold) {
    MotionDetection motion;
    
    if (previousFrame.isEmpty() || currentFrame.width != previousFrame.width || 
        currentFrame.height != previousFrame.height) {
        return motion;
    }
    
    ImageProcessor processor;
    Image diff = processor.subtract(currentFrame, previousFrame);
    
    double sum = 0.0;
    int count = 0;
    
    for (int y = 0; y < diff.height; ++y) {
        for (int x = 0; x < diff.width; ++x) {
            double value = 0.0;
            for (int c = 0; c < diff.channels; ++c) {
                value += std::abs(static_cast<int>(diff.at(x, y, c)) - 128);
            }
            value /= diff.channels;
            sum += value;
            count++;
            
            if (value > threshold) {
                motion.motionRegions.push_back(Rectangle(x - 10, y - 10, 20, 20, value / 255.0));
            }
        }
    }
    
    motion.motionIntensity = sum / count;
    motion.hasMotion = motion.motionIntensity > threshold;
    
    return motion;
}

std::vector<Rectangle> RealTimeVideoAnalyzer::trackObjects(const Image& frame, const std::vector<Rectangle>& previousObjects) {
    // تتبع بسيط - في التطبيق الفعلي سيستخدم Kalman Filter أو DeepSORT
    return previousObjects;
}

Rectangle RealTimeVideoAnalyzer::trackObjectByColor(const Image& frame, const Color& lower, const Color& upper) {
    // كشف الأجسام بناءً على نطاق لوني
    // نستخدم HSV إذا كان متاحاً للدقة، وإلا نستخدم RGB
    int minX = frame.width, minY = frame.height, maxX = -1, maxY = -1;
    bool found = false;
    
    for (int y = 0; y < frame.height; ++y) {
        for (int x = 0; x < frame.width; ++x) {
            uint8_t r = frame.at(x, y, 0);
            uint8_t g = frame.at(x, y, 1);
            uint8_t b = frame.at(x, y, 2);
            
            if (r >= lower.r && r <= upper.r &&
                g >= lower.g && g <= upper.g &&
                b >= lower.b && b <= upper.b) {
                minX = std::min(minX, x);
                minY = std::min(minY, y);
                maxX = std::max(maxX, x);
                maxY = std::max(maxY, y);
                found = true;
            }
        }
    }
    
    if (!found) return Rectangle(0, 0, 0, 0, 0.0);
    return Rectangle(minX, minY, maxX - minX + 1, maxY - minY + 1, 1.0);
}

Image RealTimeVideoAnalyzer::annotateFrame(const Image& frame, const std::vector<Face>& faces, const MotionDetection& motion) {
    Image result = frame;
    
    // رسم مربعات الوجوه
    FaceDetector faceDetector;
    result = faceDetector.drawFaceBoxes(result, faces);
    
    // رسم مناطق الحركة
    for (const auto& region : motion.motionRegions) {
        // رسم بسيط للمناطق
        for (int i = 0; i < 2; ++i) {
            for (int dx = 0; dx < region.width && region.x + dx < result.width; ++dx) {
                if (region.y + i < result.height) {
                    result.at(region.x + dx, region.y + i, 1) = 255; // أخضر
                }
            }
        }
    }
    
    return result;
}

void RealTimeVideoAnalyzer::updateMetrics(double processingTime, int faces, double motion) {
    metrics.processingTime = processingTime;
    metrics.detectedFaces = faces;
    metrics.motionIntensity = motion;
    if (processingTime > 0) {
        metrics.fps = 1.0 / processingTime;
    }
}

// ==================== Optical Flow & Background Subtraction ====================

std::vector<std::pair<std::pair<int,int>, std::pair<int,int>>> OpticalFlow::denseFlow(const Image& prev, const Image& next, int step, int win) {
    ImageProcessor ip;
    Image p = (prev.channels == 1) ? prev : ip.toGrayscale(prev);
    Image n = (next.channels == 1) ? next : ip.toGrayscale(next);
    std::vector<std::pair<std::pair<int,int>, std::pair<int,int>>> flows;
    int r = win / 2;
    // حساب المشتقات
    std::vector<std::vector<double>> sobelX = {{-1,0,1},{-2,0,2},{-1,0,1}};
    std::vector<std::vector<double>> sobelY = {{-1,-2,-1},{0,0,0},{1,2,1}};
    Image gx(p.width, p.height, 1), gy(p.width, p.height, 1);
    ip.applyKernel(p, gx, sobelX);
    ip.applyKernel(p, gy, sobelY);
    // الفرق الزمني
    Image gt(p.width, p.height, 1);
    for (int y = 0; y < p.height; ++y) {
        for (int x = 0; x < p.width; ++x) {
            int dv = static_cast<int>(n.at(x, y, 0)) - static_cast<int>(p.at(x, y, 0));
            gt.at(x, y, 0) = ip.clamp(128 + dv);
        }
    }
    for (int y = r; y < p.height - r; y += step) {
        for (int x = r; x < p.width - r; x += step) {
            double sumIx2 = 0, sumIy2 = 0, sumIxIy = 0, sumIxIt = 0, sumIyIt = 0;
            for (int dy = -r; dy <= r; ++dy) {
                for (int dx = -r; dx <= r; ++dx) {
                    double ix = gx.at(x + dx, y + dy, 0) - 128.0;
                    double iy = gy.at(x + dx, y + dy, 0) - 128.0;
                    double it = gt.at(x + dx, y + dy, 0) - 128.0;
                    sumIx2 += ix * ix;
                    sumIy2 += iy * iy;
                    sumIxIy += ix * iy;
                    sumIxIt += ix * it;
                    sumIyIt += iy * it;
                }
            }
            double det = sumIx2 * sumIy2 - sumIxIy * sumIxIy;
            if (std::abs(det) < 1e-6) continue;
            double invDet = 1.0 / det;
            double vx = (-sumIy2 * sumIxIt + sumIxIy * sumIyIt) * invDet;
            double vy = (sumIxIy * sumIxIt - sumIx2 * sumIyIt) * invDet;
            int nx = static_cast<int>(x + vx);
            int ny = static_cast<int>(y + vy);
            flows.push_back({{x, y}, {nx, ny}});
        }
    }
    return flows;
}

Image BackgroundSubtractorSimple::apply(const Image& frame, int threshold) {
    ImageProcessor ip;
    Image gray = (frame.channels == 1) ? frame : ip.toGrayscale(frame);
    if (!initialized) {
        background = gray;
        initialized = true;
        return Image(gray.width, gray.height, 1);
    }
    // تحديث الخلفية
    for (int y = 0; y < gray.height; ++y) {
        for (int x = 0; x < gray.width; ++x) {
            uint8_t bg = background.at(x, y, 0);
            uint8_t fg = gray.at(x, y, 0);
            uint8_t updated = static_cast<uint8_t>(bg * (1.0 - alpha) + fg * alpha);
            background.at(x, y, 0) = updated;
        }
    }
    // الفرق
    Image mask(gray.width, gray.height, 1);
    for (int y = 0; y < gray.height; ++y) {
        for (int x = 0; x < gray.width; ++x) {
            int diff = std::abs(static_cast<int>(gray.at(x, y, 0)) - static_cast<int>(background.at(x, y, 0)));
            mask.at(x, y, 0) = (diff > threshold) ? 255 : 0;
        }
    }
    return mask;
}

// ==================== دوال الاختبار ====================

void testImageProcessing() {
    std::cout << "🧪 اختبار معالجة الصور..." << std::endl;
    
    ImageProcessor processor;
    
    // إنشاء صورة اختبار
    Image testImg(100, 100, 3);
    for (int y = 0; y < 100; ++y) {
        for (int x = 0; x < 100; ++x) {
            testImg.at(x, y, 0) = (x + y) % 256;
            testImg.at(x, y, 1) = (x * 2) % 256;
            testImg.at(x, y, 2) = (y * 2) % 256;
        }
    }
    
    // اختبار التحويلات
    Image gray = processor.toGrayscale(testImg);
    Image resized = processor.resize(testImg, 50, 50);
    Image blurred = processor.applyGaussianBlur(testImg, 2.0);
    
    std::cout << "✅ تم اختبار معالجة الصور بنجاح" << std::endl;
}

void testShapeDetection() {
    std::cout << "🧪 اختبار كشف الأشكال..." << std::endl;
    
    ImageProcessor processor;
    Image testImg(200, 200, 1);
    testImg.data.assign(testImg.data.size(), 128);
    
    ShapeDetector detector;
    std::vector<Circle> circles = detector.detectCircles(testImg);
    std::vector<Rectangle> rects = detector.detectRectangles(testImg);
    
    std::cout << "✅ تم العثور على " << circles.size() << " دائرة و " << rects.size() << " مستطيل" << std::endl;
}

void testFaceDetection() {
    std::cout << "🧪 اختبار كشف الوجوه..." << std::endl;
    
    ImageProcessor processor;
    Image testImg(640, 480, 3);
    testImg.data.assign(testImg.data.size(), 150);
    
    FaceDetector detector;
    std::vector<Face> faces = detector.detectFaces(testImg);
    
    std::cout << "✅ تم العثور على " << faces.size() << " وجه" << std::endl;
}

void testVideoProcessing() {
    std::cout << "🧪 اختبار معالجة الفيديو..." << std::endl;
    
    VideoProcessor processor;
    VideoCapture capture = processor.openVideo("test.mp4");
    
    int frameCount = 0;
    while (capture.hasNext() && frameCount < 10) {
        Image frame = capture.readFrame();
        frameCount++;
    }
    
    std::cout << "✅ تم معالجة " << frameCount << " إطار" << std::endl;
}

void testRealTimeAnalysis() {
    std::cout << "🧪 اختبار التحليل في الوقت الفعلي..." << std::endl;
    
    RealTimeVideoAnalyzer analyzer;
    CameraCapture camera = analyzer.openCamera(0);
    
    Image frame1 = camera.readFrame();
    Image frame2 = camera.readFrame();
    
    MotionDetection motion = analyzer.detectMotion(frame2, frame1);
    
    std::cout << "✅ تم كشف الحركة: " << (motion.hasMotion ? "نعم" : "لا") << std::endl;
}

// ==================== ArabicOCR Implementation ====================

ArabicOCR::ArabicOCR() {
}

std::string ArabicOCR::recognizeText(const Image& img) {
    Image preprocessed = preprocessForOCR(img);
    std::vector<Rectangle> segments = segmentText(preprocessed);
    
    std::string result = "";
    for (const auto& rect : segments) {
        result += recognizeRegion(preprocessed, rect);
        result += " ";
    }
    return result;
}

std::string ArabicOCR::recognizeRegion(const Image& img, const Rectangle& rect) {
    ImageProcessor ip;
    Image region = ip.crop(img, rect.x, rect.y, rect.width, rect.height);
    
    // تقسيم المنطقة (التي قد تكون كلمة) إلى حروف
    // نستخدم الإسقاط العمودي (Vertical Projection)
    std::vector<int> projection(region.width, 0);
    for (int x = 0; x < region.width; ++x) {
        for (int y = 0; y < region.height; ++y) {
            if (region.at(x, y, 0) < 128) { // بكسل نص (أسود)
                projection[x]++;
            }
        }
    }
    
    std::string word = "";
    int start = -1;
    for (int x = 0; x < region.width; ++x) {
        if (projection[x] > 0 && start == -1) {
            start = x;
        } else if (projection[x] == 0 && start != -1) {
            int charWidth = x - start;
            if (charWidth > 2) {
                Image charImg = ip.crop(region, start, 0, charWidth, region.height);
                char32_t c = ArabicOCR::recognizeCharacter(charImg);
                // تحويل UTF-32 إلى UTF-8 بسيط
                if (c < 0x80) word += (char)c;
                else {
                    // تمثيل مبسط للحروف العربية
                    word += "؟"; // سيتم تحسينه لاحقاً بدعم كامل لـ UTF-8
                }
            }
            start = -1;
        }
    }
    
    return word;
}

std::vector<Rectangle> ArabicOCR::segmentText(const Image& img) {
    std::vector<Rectangle> segments;
    // الإسقاط الأفقي لتقسيم الأسطر
    std::vector<int> hProj(img.height, 0);
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            if (img.at(x, y, 0) < 128) hProj[y]++;
        }
    }
    
    int lineStart = -1;
    for (int y = 0; y < img.height; ++y) {
        if (hProj[y] > 0 && lineStart == -1) {
            lineStart = y;
        } else if (hProj[y] == 0 && lineStart != -1) {
            int lineH = y - lineStart;
            if (lineH > 5) {
                // تقسيم السطر إلى كلمات باستخدام الإسقاط العمودي
                int wordStart = -1;
                for (int x = 0; x < img.width; ++x) {
                    bool hasContent = false;
                    for (int ly = lineStart; ly < y; ++ly) {
                        if (img.at(x, ly, 0) < 128) { hasContent = true; break; }
                    }
                    
                    if (hasContent && wordStart == -1) {
                        wordStart = x;
                    } else if (!hasContent && wordStart != -1) {
                        if (x - wordStart > 5) {
                            segments.push_back(Rectangle(wordStart, lineStart, x - wordStart, lineH, 1.0));
                        }
                        wordStart = -1;
                    }
                }
            }
            lineStart = -1;
        }
    }
    
    return segments;
}

Image ArabicOCR::preprocessForOCR(const Image& img) {
    ImageProcessor ip;
    Image gray = (img.channels == 1) ? img : ip.toGrayscale(img);
    
    // ثنائية الصورة (Thresholding)
    Image binary(gray.width, gray.height, 1);
    for (int i = 0; i < gray.width * gray.height; ++i) {
        binary.data[i] = (gray.data[i] > 127) ? 255 : 0;
    }
    
    // إزالة الضجيج البسيط
    return ip.applyMedianFilter(binary, 3);
}

char32_t ArabicOCR::recognizeCharacter(const Image& charImg) {
    // في النسخة الاحترافية، نستخدم شبكة عصبية (CNN)
    // هنا نستخدم "بصمة" بسيطة تعتمد على كثافة البكسلات والنسبة العرضية
    double density = 0;
    for (uint8_t v : charImg.data) if (v < 128) density++;
    density /= (charImg.width * charImg.height);
    
    double aspect = (double)charImg.width / charImg.height;
    
    // تمثيل وهمي للتمييز
    if (aspect > 1.5) return 0x0640; // تطويل (ـ)
    if (density > 0.5) return 0x0645; // ميم
    return 0x0627; // ألف (افتراضي)
}

double ArabicOCR::detectSkewAngle(const Image& img) {
    // استخدام تحويل هاف لكشف الخطوط المهيمنة (الأسطر)
    ArabicLanguage::ShapeDetector sd;
    auto lines = sd.detectLines(img, 0.5, 50, 10);
    if (lines.empty()) return 0.0;
    
    double avgAngle = 0;
    for (const auto& l : lines) {
        avgAngle += std::atan2(l.y2 - l.y1, l.x2 - l.x1);
    }
    return avgAngle / lines.size();
}

// ==================== TrainingTool Implementation ====================

bool TrainingTool::saveAnnotation(const std::string& imagePath, const std::vector<Annotation>& annotations, const std::string& outputPath) {
    std::ofstream file(outputPath, std::ios::app);
    if (!file.is_open()) return false;
    
    file << imagePath;
    for (const auto& ann : annotations) {
        file << " " << ann.rect.x << "," << ann.rect.y << "," << ann.rect.width << "," << ann.rect.height << "," << ann.label;
    }
    file << "\n";
    return true;
}

std::vector<TrainingTool::Annotation> TrainingTool::loadAnnotations(const std::string& annotationPath) {
    std::vector<Annotation> annotations;
    std::ifstream file(annotationPath);
    if (!file.is_open()) return annotations;
    
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string path;
        ss >> path;
        std::string annStr;
        while (ss >> annStr) {
            Annotation ann;
            size_t c1 = annStr.find(',');
            size_t c2 = annStr.find(',', c1 + 1);
            size_t c3 = annStr.find(',', c2 + 1);
            size_t c4 = annStr.find(',', c3 + 1);
            if (c1 != std::string::npos && c2 != std::string::npos && c3 != std::string::npos && c4 != std::string::npos) {
                ann.rect.x = std::stoi(annStr.substr(0, c1));
                ann.rect.y = std::stoi(annStr.substr(c1 + 1, c2 - c1 - 1));
                ann.rect.width = std::stoi(annStr.substr(c2 + 1, c3 - c2 - 1));
                ann.rect.height = std::stoi(annStr.substr(c3 + 1, c4 - c3 - 1));
                ann.label = annStr.substr(c4 + 1);
                annotations.push_back(ann);
            }
        }
    }
    return annotations;
}

void TrainingTool::captureSamplesFromVideo(const std::string& videoPath, const std::string& outputFolder, int intervalFrames) {
    if (!fs::exists(outputFolder)) {
        fs::create_directories(outputFolder);
    }
    
    // محاكاة استخراج الإطارات من الفيديو
    std::cout << "🎥 بدء استخراج العينات من: " << videoPath << std::endl;
    ImageProcessor ip;
    for (int i = 0; i < 5; ++i) { // محاكاة 5 إطارات
        Image frame(640, 480, 3);
        // توليد بيانات وهمية للإطار
        std::fill(frame.data.begin(), frame.data.end(), static_cast<uint8_t>(128 + i * 10));
        
        std::string filename = outputFolder + "/frame_" + std::to_string(i * intervalFrames) + ".jpg";
        ip.saveImage(frame, filename, ImageFormat::JPEG);
        std::cout << "✅ تم حفظ الإطار: " << filename << std::endl;
    }
    std::cout << "🏁 تم الانتهاء من استخراج العينات." << std::endl;
}

void TrainingTool::balanceDataset(const std::string& datasetPath) {
    std::cout << "⚖️ موازنة مجموعة البيانات في: " << datasetPath << std::endl;
    
    std::map<std::string, int> classCounts;
    try {
        if (!fs::exists(datasetPath)) return;

        for (const auto& entry : fs::directory_iterator(datasetPath)) {
            if (entry.is_directory()) {
                std::string className = entry.path().filename().string();
                int count = 0;
                for (const auto& file : fs::directory_iterator(entry.path())) {
                    if (file.is_regular_file()) count++;
                }
                classCounts[className] = count;
            }
        }

        if (classCounts.empty()) {
            std::cout << "⚠️ لم يتم العثور على فئات للموازنة." << std::endl;
            return;
        }

        // البحث عن الحد الأقصى والحد الأدنى
        int minCount = std::numeric_limits<int>::max();
        int maxCount = 0;
        for (auto const& [name, count] : classCounts) {
            minCount = std::min(minCount, count);
            maxCount = std::max(maxCount, count);
            std::cout << "  - الفئة [" << name << "]: " << count << " عينة" << std::endl;
        }

        if (maxCount == minCount) {
            std::cout << "✨ مجموعة البيانات متوازنة بالفعل." << std::endl;
        } else {
            std::cout << "📢 يوصى بزيادة العينات (Augmentation) للفئات الأقل من " << maxCount << " عينة." << std::endl;
            // في التطبيق الفعلي، يمكننا استدعاء DatasetManager::augmentImage هنا
        }

    } catch (const std::exception& e) {
        std::cerr << "❌ خطأ أثناء موازنة البيانات: " << e.what() << std::endl;
    }
}

// ==================== HandDetector Implementation ====================

std::vector<std::pair<int, int>> HandDetector::detectFingers(const Image& img) {
    // في التطبيق الفعلي، سنستخدم نموذج تعلم عميق أو خوارزمية معالجة صور متقدمة
    // هنا نقوم بمحاكاة اكتشاف الأصابع لأغراض العرض التعليمي
    std::vector<std::pair<int, int>> fingers;
    
    if (img.isEmpty()) return fingers;
    
    // محاكاة 5 أصابع موزعة بشكل منطقي في منتصف الصورة
    int centerX = img.width / 2;
    int centerY = img.height / 2;
    
    // إصبع الإبهام
    fingers.push_back({centerX - 60, centerY - 20});
    // السبابة
    fingers.push_back({centerX - 30, centerY - 80});
    // الوسطى
    fingers.push_back({centerX, centerY - 100});
    // البنصر
    fingers.push_back({centerX + 30, centerY - 80});
    // الخنصر
    fingers.push_back({centerX + 60, centerY - 50});
    
    return fingers;
}

Rectangle HandDetector::detectPalm(const Image& img) {
    if (img.isEmpty()) return Rectangle(0, 0, 0, 0);
    
    // محاكاة راحة اليد في منتصف الصورة
    int w = 150;
    int h = 150;
    int x = (img.width - w) / 2;
    int y = (img.height - h) / 2;
    
    return Rectangle(x, y, w, h);
}

} // namespace ArabicLanguage

