// ArabicComputerVision.h - مكتبة الرؤية الحاسوبية العربية
// الأسبوع الثالث من الشهر السادس: الرؤية الحاسوبية
#ifndef ARABIC_COMPUTER_VISION_H
#define ARABIC_COMPUTER_VISION_H

#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <thread>
#include <map>

namespace ArabicLanguage {

// ==================== أنواع البيانات الأساسية ====================

/**
 * @brief تنسيق الصورة
 */
enum class ImageFormat {
    JPEG,
    PNG,
    BMP,
    UNKNOWN
};

/**
 * @brief طريقة تغيير الحجم
 */
enum class ResizeMethod {
    NEAREST_NEIGHBOR,
    BILINEAR,
    BICUBIC
};

/**
 * @brief طريقة الاستيفاء
 */
enum class InterpolationMethod {
    NEAREST,
    BILINEAR,
    BICUBIC
};

/**
 * @brief صورة
 */
struct Image {
    std::vector<uint8_t> data;
    int width;
    int height;
    int channels; // 1 = Grayscale, 3 = RGB, 4 = RGBA
    
    Image() : width(0), height(0), channels(0) {}
    Image(int w, int h, int c) : width(w), height(h), channels(c) {
        data.resize(w * h * c);
    }
    
    size_t getSize() const { return width * height * channels; }
    bool isEmpty() const { return data.empty(); }
    
    uint8_t& at(int x, int y, int c = 0) {
        return data[(y * width + x) * channels + c];
    }
    
    const uint8_t& at(int x, int y, int c = 0) const {
        return data[(y * width + x) * channels + c];
    }
};

/**
 * @brief لون بسيط (RGB أو RGBA)
 */
struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}
};

/**
 * @brief دائرة
 */
struct Circle {
    int centerX;
    int centerY;
    int radius;
    double confidence;
    
    Circle() : centerX(0), centerY(0), radius(0), confidence(0.0) {}
    Circle(int x, int y, int r, double conf = 1.0) 
        : centerX(x), centerY(y), radius(r), confidence(conf) {}
};

/**
 * @brief مستطيل
 */
struct Rectangle {
    int x;
    int y;
    int width;
    int height;
    double confidence;
    
    Rectangle() : x(0), y(0), width(0), height(0), confidence(0.0) {}
    Rectangle(int x, int y, int w, int h, double conf = 1.0)
        : x(x), y(y), width(w), height(h), confidence(conf) {}
};

/**
 * @brief خط
 */
struct Line {
    int x1;
    int y1;
    int x2;
    int y2;
    double confidence;
    
    Line() : x1(0), y1(0), x2(0), y2(0), confidence(0.0) {}
    Line(int x1, int y1, int x2, int y2, double conf = 1.0)
        : x1(x1), y1(y1), x2(x2), y2(y2), confidence(conf) {}
};

/**
 * @brief مضلع
 */
struct Polygon {
    std::vector<std::pair<int, int>> points;
    double confidence;
    
    Polygon() : confidence(0.0) {}
};

/**
 * @brief وجه
 */
struct Face {
    int x;
    int y;
    int width;
    int height;
    double confidence;
    
    Face() : x(0), y(0), width(0), height(0), confidence(0.0) {}
    Face(int x, int y, int w, int h, double conf = 1.0)
        : x(x), y(y), width(w), height(h), confidence(conf) {}
};

/**
 * @brief معالم الوجه
 */
struct FaceLandmarks {
    std::pair<int, int> leftEye;
    std::pair<int, int> rightEye;
    std::pair<int, int> nose;
    std::pair<int, int> mouth;
    std::vector<std::pair<int, int>> additionalPoints;
};

/**
 * @brief كشف الحركة
 */
struct MotionDetection {
    std::vector<Rectangle> motionRegions;
    double motionIntensity;
    bool hasMotion;
    
    MotionDetection() : motionIntensity(0.0), hasMotion(false) {}
};

/**
 * @brief نقطة ميزة
 */
struct Keypoint {
    int x;
    int y;
    float response;
    float angle;
    Keypoint() : x(0), y(0), response(0.0f), angle(0.0f) {}
    Keypoint(int x, int y, float response, float angle = 0.0f)
        : x(x), y(y), response(response), angle(angle) {}
};

/**
 * @brief واصف ميزة بسيط
 */
struct Descriptor {
    std::vector<float> data;
};

// ==================== معالجة الصور ====================

/**
 * @brief معالج الصور
 */
class ImageProcessor {
public:
    ImageProcessor();
    ~ImageProcessor() = default;
    
    // تحميل وحفظ
    Image loadImage(const std::string& filepath);
    bool saveImage(const Image& img, const std::string& filepath, ImageFormat format);
    
    // التحويلات الأساسية
    Image resize(const Image& img, int newWidth, int newHeight, ResizeMethod method = ResizeMethod::BILINEAR);
    Image crop(const Image& img, int x, int y, int width, int height);
    Image rotate(const Image& img, double angle, InterpolationMethod method = InterpolationMethod::BILINEAR);
    Image flip(const Image& img, bool horizontal, bool vertical);
    
    // تحويلات الألوان
    Image toGrayscale(const Image& img);
    Image toRGB(const Image& img);
    Image toRGBA(const Image& img);
    Image toHSV(const Image& img);
    Image fromHSV(const Image& img);
    Image toLAB(const Image& img);
    Image fromLAB(const Image& img);
    Image toYUV(const Image& img);
    Image fromYUV(const Image& img);
    
    // الفلاتر
    Image applyGaussianBlur(const Image& img, double sigma);
    Image applyMedianFilter(const Image& img, int kernelSize);
    Image sharpen(const Image& img, double strength);
    Image detectEdges(const Image& img, int lowThreshold, int highThreshold);
    Image detectEdgesCanny(const Image& img, int lowThreshold, int highThreshold);
    Image boxBlur(const Image& img, int kernelSize);
    
    // تحسينات
    Image adjustBrightnessContrast(const Image& img, double brightness, double contrast);
    Image adjustSaturation(const Image& img, double saturation);
    Image equalizeHistogram(const Image& img);
    Image applyCLAHE(const Image& img, double clipLimit = 40.0, int tileGridSize = 8);
    Image applyLUT(const Image& img, const std::vector<uint8_t>& lut);
    Image alphaBlend(const Image& img1, const Image& img2, double alpha);
    
    // التجزئة المتقدمة (Advanced Segmentation)
    void watershed(const Image& img, Image& markers);
    Image grabCut(const Image& img, const Rectangle& rect, int iterCount = 5);
    
    // المخططات البيانية (Histograms)
    std::vector<float> calcHist(const Image& img, int channel = 0, const Image& mask = Image(), int bins = 256);
    std::vector<std::vector<float>> calc2DHist(const Image& img, int channel1, int channel2, int bins1 = 32, int bins2 = 32);
    std::vector<std::vector<std::vector<float>>> calc3DHist(const Image& img, int bins1 = 16, int bins2 = 16, int bins3 = 16);
    Image drawHistogram(const std::vector<float>& hist, int width = 512, int height = 400, const Color& color = Color(255, 255, 255));
    Image draw2DHistogram(const std::vector<std::vector<float>>& hist, int width = 512, int height = 512);
    Image calcBackProject(const Image& img, const std::vector<float>& hist, int channel = 0);
    
    // الأهرامات والتحويلات المتقدمة
    Image pyramidUp(const Image& img);
    Image pyramidDown(const Image& img);
    Image blendPyramids(const Image& img1, const Image& img2, const Image& mask);
    Image warpAffine(const Image& img, const std::vector<double>& matrix, int width, int height);
    Image warpPerspective(const Image& img, const std::vector<double>& matrix, int width, int height);
    
    // تحويل فورييه (DFT)
    struct DFTResult {
        std::vector<double> realData;
        std::vector<double> imagData;
        int width;
        int height;
        Image magnitude;
        Image phase;
    };
    DFTResult applyDFT(const Image& img);
    Image applyIDFT(const DFTResult& dft);
    Image fftShift(const Image& img);
    
    // العمليات الأساسية
    Image add(const Image& img1, const Image& img2);
    Image subtract(const Image& img1, const Image& img2);
    Image multiply(const Image& img1, const Image& img2, double factor = 1.0);

    // تدرجات الصورة
    Image applySobel(const Image& img, bool horizontal = true);
    Image applyScharr(const Image& img, bool horizontal = true);
    Image applyLaplacian(const Image& img);
    Image bilateralFilter(const Image& img, int d, double sigmaColor, double sigmaSpace);

    // معايرة الكاميرا والرؤية المجسمة
    struct CameraMatrix {
        double fx, fy, cx, cy;
        std::vector<double> distCoeffs; // k1, k2, p1, p2, k3
    };
    Image undistort(const Image& img, const CameraMatrix& camera);
    Image computeStereoDisparity(const Image& left, const Image& right, int numDisparities = 16, int blockSize = 15);
    
    struct Pose {
        std::vector<double> rotation;    // 3x3 matrix
        std::vector<double> translation; // 3x1 vector
    };
    Pose solvePnP(const std::vector<std::vector<double>>& objectPoints, const std::vector<std::pair<int, int>>& imagePoints, const CameraMatrix& camera);

    // إحصائيات الصورة
    Color meanColor(const Image& img, const Image& mask = Image());
    struct ImageProps {
        int width;
        int height;
        int channels;
        double minVal;
        double maxVal;
        double avgVal;
        double stdDev;
    };
    ImageProps getImageProps(const Image& img);

    // توابع الرسم الأساسية
    Image drawLine(const Image& img, int x1, int y1, int x2, int y2, const Color& color, int thickness = 1);
    Image drawRectangle(const Image& img, int x, int y, int width, int height, const Color& color, int thickness = 1, bool filled = false);
    Image drawCircle(const Image& img, int centerX, int centerY, int radius, const Color& color, int thickness = 1, bool filled = false);
    Image drawEllipse(const Image& img, int centerX, int centerY, int radiusX, int radiusY, double angleDeg, const Color& color, int thickness = 1, bool filled = false);
    Image drawPolygon(const Image& img, const std::vector<std::pair<int, int>>& points, const Color& color, int thickness = 1, bool filled = false);
    Image putText(const Image& img, int x, int y, const std::string& text, const Color& color);

    // العمليات على مستوى البت
    Image bitwiseAnd(const Image& img1, const Image& img2);
    Image bitwiseOr(const Image& img1, const Image& img2);
    Image bitwiseXor(const Image& img1, const Image& img2);
    Image bitwiseNot(const Image& img);

    // قنوات الصورة
    std::vector<Image> splitChannels(const Image& img);
    Image mergeChannels(const std::vector<Image>& channels);

    // المنطقة المهمة وحدود الصورة
    Image extractROI(const Image& img, int x, int y, int width, int height);
    Image addBorder(const Image& img, int top, int bottom, int left, int right, const Color& color);

    // التعتيب (Thresholding)
    Image simpleThreshold(const Image& img, uint8_t thresh, uint8_t maxVal = 255);
    Image adaptiveThresholdMean(const Image& img, int blockSize = 11, int c = 2, uint8_t maxVal = 255);
    Image otsuThreshold(const Image& img, uint8_t maxVal = 255);

    // التحويلات الشكلية (Morphology)
    Image erode(const Image& img, int kernelSize = 3, int iterations = 1);
    Image dilate(const Image& img, int kernelSize = 3, int iterations = 1);
    Image openMorph(const Image& img, int kernelSize = 3);
    Image closeMorph(const Image& img, int kernelSize = 3);
    Image gradientMorph(const Image& img, int kernelSize = 3);
    Image topHat(const Image& img, int kernelSize = 3);
    Image blackHat(const Image& img, int kernelSize = 3);
    
    // مطابقة القوالب
    struct TemplateMatch {
        int x;
        int y;
        double score;
    };
    std::vector<TemplateMatch> matchTemplate(const Image& img, const Image& templ, int maxMatches = 1, double threshold = 0.8);

    // قياس الأداء وتحسينه
    static int64_t getTickCount();
    static double getTickFrequency();
    static void setUseOptimized(bool on);
    static bool useOptimized();

    // تسريع العتاد (Hardware Acceleration)
    enum class AccelerationMode {
        CPU,
        SIMD,
        OPENCL,
        CUDA
    };
    static void setAccelerationMode(AccelerationMode mode);
    static AccelerationMode getAccelerationMode();
    static bool isHardwareAccelerationAvailable(AccelerationMode mode);

    void applyKernel(const Image& img, Image& result, const std::vector<std::vector<double>>& kernel);
    void applyKernel1D(const Image& img, Image& result, const std::vector<double>& kernel, bool horizontal);
    uint8_t clamp(int value);
    void setPixelColor(Image& img, int x, int y, const Color& color);

private:
    static bool _useOptimized;
    std::vector<int32_t> computeIntegralImage(const Image& img);
    double gaussian(double x, double y, double sigma);
};

/**
 * @brief كاشف الأشكال والمعالم (Shape and Feature Detector)
 */
class ShapeDetector {
public:
    ShapeDetector();
    ~ShapeDetector() = default;

    // كشف الأشكال الأساسية
    std::vector<Circle> detectCircles(const Image& img, int minRadius = 10, int maxRadius = 100, double threshold = 0.8);
    std::vector<Rectangle> detectRectangles(const Image& img, double threshold = 0.5);
    std::vector<Line> detectLines(const Image& img, double threshold = 0.5, int minLineLength = 50, int maxLineGap = 10);
    std::vector<Line> detectLinesProbabilistic(const Image& img, double threshold = 0.5, int minLineLength = 50, int maxLineGap = 10);
    std::vector<Polygon> detectPolygons(const Image& img, int minVertices = 3, int maxVertices = 10);

    // معالجة الكنتور (Contour Processing)
    std::vector<std::vector<std::pair<int, int>>> detectContours(const Image& img);
    std::vector<std::pair<int, int>> approxPolyDP(const std::vector<std::pair<int, int>>& contour, double epsilon);
    double contourArea(const std::vector<std::pair<int, int>>& contour) const;
    double contourPerimeter(const std::vector<std::pair<int, int>>& contour) const;
    
    // خصائص الشكل
    double aspectRatio(const std::vector<std::pair<int, int>>& contour) const;
    double extent(const std::vector<std::pair<int, int>>& contour) const;
    double solidity(const std::vector<std::pair<int, int>>& contour) const;
    double equivalentDiameter(const std::vector<std::pair<int, int>>& contour) const;
    double orientation(const std::vector<std::pair<int, int>>& contour) const;
    Rectangle boundingBox(const std::vector<std::pair<int, int>>& contour) const;
    Circle minEnclosingCircle(const std::vector<std::pair<int, int>>& contour) const;
    void fitEllipse(const std::vector<std::pair<int, int>>& contour, int& centerX, int& centerY, int& radiusX, int& radiusY, double& angle);
    void fitLine(const std::vector<std::pair<int, int>>& contour, double& vx, double& vy, double& x0, double& y0);

    // نقاط المعالم (Keypoints)
    std::vector<Keypoint> detectHarrisCorners(const Image& img, int blockSize = 2, int ksize = 3, double k = 0.04, double threshold = 0.01);
    std::vector<Keypoint> detectShiTomasi(const Image& img, int maxCorners = 100, double qualityLevel = 0.01, int minDistance = 10);
    std::vector<Keypoint> detectFAST(const Image& img, int threshold = 10, int contiguous = 9);
    std::vector<Keypoint> detectORB(const Image& img, int nfeatures = 500);
    
    // الواصفات والمطابقة (Descriptors and Matching)
    std::vector<Descriptor> computeORBDescriptors(const Image& img, const std::vector<Keypoint>& kps);
    Descriptor computeDescriptor(const Image& img, const Keypoint& kp, int patch = 31);
    std::vector<Descriptor> computeDescriptors(const Image& img, const std::vector<Keypoint>& kps, int patch = 31);
    
    struct Match {
        int queryIdx;
        int trainIdx;
        double distance;
    };
    std::vector<Match> matchBruteForce(const std::vector<Descriptor>& q, const std::vector<Descriptor>& t);
    
    // التدفق البصري وتتبع الكائنات
    std::vector<std::pair<int, int>> calcOpticalFlowPyrLK(const Image& prevImg, const Image& nextImg, 
                                                         const std::vector<std::pair<int, int>>& prevPts,
                                                         std::vector<uint8_t>& status,
                                                         int winSize = 15, int maxLevel = 3);
    Image calcOpticalFlowFarneback(const Image& prevImg, const Image& nextImg);
    Rectangle meanShift(const Image& probImage, const Rectangle& window, int iterations = 10);
    Rectangle camShift(const Image& probImage, const Rectangle& window, int iterations = 10);

    // تحليل الحركة والخلفية
    Image backgroundSubtractionMOG2(const Image& img, Image& background, double learningRate = 0.05);

    // دوال مساعدة
    double pointPolygonTest(const std::vector<std::pair<int, int>>& contour, int x, int y) const;
    std::vector<std::pair<int, int>> convexHull(const std::vector<std::pair<int, int>>& contour) const;
    std::vector<std::pair<int, int>> convexityDefects(const std::vector<std::pair<int, int>>& contour) const;
    
    struct Moments {
        double m00, m10, m01, m20, m11, m02, m30, m21, m12, m03;
        double mu20, mu11, mu02, mu30, mu21, mu12, mu03;
        double nu20, nu11, nu02, nu30, nu21, nu12, nu03;
        double hu[7];
    };
    Moments calculateMoments(const std::vector<std::pair<int, int>>& contour);
    double matchShapes(const std::vector<std::pair<int, int>>& c1, const std::vector<std::pair<int, int>>& c2);

    void cornerSubPix(const Image& img, std::vector<Keypoint>& corners, int winSize = 5);
    std::vector<double> findHomography(const std::vector<std::pair<int, int>>& srcPoints, const std::vector<std::pair<int, int>>& dstPoints);

private:
    Image preprocessForShapeDetection(const Image& img);
    std::vector<Circle> houghCircles(const Image& img, int minRadius, int maxRadius, double threshold);
    std::vector<Line> houghLines(const Image& img, double threshold, int minLineLength, int maxLineGap);
    double distance(int x1, int y1, int x2, int y2) const;
    
    std::pair<int, int> extremePointMinX(const std::vector<std::pair<int, int>>& contour) const;
    std::pair<int, int> extremePointMaxX(const std::vector<std::pair<int, int>>& contour) const;
    std::pair<int, int> extremePointMinY(const std::vector<std::pair<int, int>>& contour) const;
    std::pair<int, int> extremePointMaxY(const std::vector<std::pair<int, int>>& contour) const;
};

/**
 * @brief كاميرا الجهاز (Device Camera)
 */
class Camera {
public:
    Camera();
    ~Camera();

    bool open(int deviceIndex = 0);
    void close();
    bool isOpened() const;
    Image capture();

private:
    bool _isOpened;
    int _deviceIndex;
    void* _internalData = nullptr; // For DirectShow handles
};

/**
 * @brief كاشف اليد والأصابع (Hand and Finger Detector)
 */
class HandDetector {
public:
    HandDetector() = default;
    ~HandDetector() = default;

    /**
     * @brief اكتشاف أصابع اليد في الصورة
     * @return ناقل يحتوي على نقاط رؤوس الأصابع المكتشفة
     */
    std::vector<std::pair<int, int>> detectFingers(const Image& img);
    
    /**
     * @brief اكتشاف راحة اليد
     */
    Rectangle detectPalm(const Image& img);
};

/**
 * @brief أدوات التدريب (Training Tools)
 */
class TrainingTool {
public:
    TrainingTool() = default;
    
    // تسجيل بيانات التدريب (Annotation)
    struct Annotation {
        std::string label;
        Rectangle rect;
    };
    
    bool saveAnnotation(const std::string& imagePath, const std::vector<Annotation>& annotations, const std::string& outputPath);
    std::vector<Annotation> loadAnnotations(const std::string& annotationPath);
    
    // جمع البيانات من الفيديو
    void captureSamplesFromVideo(const std::string& videoPath, const std::string& outputFolder, int intervalFrames = 10);
    
    // موازنة البيانات
    void balanceDataset(const std::string& datasetPath);
};

// ==================== أحداث التفاعل وإدارة النوافذ ====================

/**
 * @brief أحداث الفأرة
 */
enum class MouseEvent {
    MOVE,
    LEFT_DOWN,
    RIGHT_DOWN,
    MIDDLE_DOWN,
    LEFT_UP,
    RIGHT_UP,
    MIDDLE_UP,
    LEFT_DBLCLK,
    RIGHT_DBLCLK,
    MIDDLE_DBLCLK
};

/**
 * @brief نوع الدالة المستدعاة لأحداث الفأرة
 */
typedef void (*MouseCallback)(MouseEvent event, int x, int y, int flags, void* userdata);

/**
 * @brief نوع الدالة المستدعاة لشريط التمرير
 */
typedef void (*TrackbarCallback)(int pos, void* userdata);

/**
 * @brief مدير النوافذ والتفاعل
 */
class WindowManager {
public:
    struct WindowInfo {
        std::string name;
        Image currentImage;
        void* hwnd = nullptr;
        MouseCallback mouseCallback = nullptr;
        void* mouseUserdata = nullptr;
        std::map<std::string, int> trackbars;
        std::map<std::string, TrackbarCallback> trackbarCallbacks;
        std::map<std::string, void*> trackbarUserdata;
        std::map<std::string, int*> trackbarValues;
        bool isClosed = false;
    };

    static void createWindow(const std::string& name);
    static void destroyWindow(const std::string& name);
    static void destroyAllWindows();
    static void showImage(const std::string& name, const Image& img);
    static int waitKey(int delay = 0);
    
    static void setMouseCallback(const std::string& name, MouseCallback onMouse, void* userdata = nullptr);
    static int createTrackbar(const std::string& trackbarName, const std::string& windowName, 
                              int* value, int count, TrackbarCallback onChange = nullptr, void* userdata = nullptr);
    static int getTrackbarPos(const std::string& trackbarName, const std::string& windowName);
    static void setTrackbarPos(const std::string& trackbarName, const std::string& windowName, int pos);

    // ضخ أحداث الواجهة (Event Pumping) — تُستدعى من حلقات ArabicExecutor
    static bool hasActiveWindows();
    static void pumpMessages();

private:
    static std::map<std::string, WindowInfo> windows;
};

// ==================== كشف الوجوه ====================

/**
 * @brief كاشف الوجوه
 */
class FaceDetector {
public:
    FaceDetector();
    ~FaceDetector() = default;
    
    // كشف الوجوه
    std::vector<Face> detectFaces(const Image& img, double scaleFactor = 1.1, 
                                   int minNeighbors = 3, int minSize = 30);
    
    // كشف الملامح
    std::vector<FaceLandmarks> detectLandmarks(const Image& img, const std::vector<Face>& faces);
    
    // رسم مربعات حول الوجوه
    Image drawFaceBoxes(const Image& img, const std::vector<Face>& faces);
    
    // رسم الملامح
    Image drawLandmarks(const Image& img, const std::vector<FaceLandmarks>& landmarks);
    
private:
    Image preprocessForFaceDetection(const Image& img);
    bool isFaceRegion(const Image& img, int x, int y, int width, int height);
    std::vector<Face> detectFacesInRegion(const Image& img, int x, int y, int width, int height);
};

// ==================== معالجة الفيديو ====================

/**
 * @brief قارئ الفيديو
 */
class VideoCapture {
public:
    VideoCapture();
    ~VideoCapture();
    
    bool open(const std::string& filepath);
    void close();
    
    bool readFrame(Image& frame);
    Image readFrame();
    bool hasNext() const;
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    double getFPS() const { return fps; }
    int getFrameCount() const { return frameCount; }
    int getCurrentFrame() const { return currentFrame; }
    
private:
    std::string filepath;
    int width;
    int height;
    double fps;
    int frameCount;
    int currentFrame;
    bool isOpen;
    
    // في التطبيق الفعلي، سيكون هناك معالج فيديو حقيقي
    void initializeVideoInfo();
};

/**
 * @brief كاتب الفيديو
 */
class VideoWriter {
public:
    VideoWriter();
    ~VideoWriter();
    
    bool open(const std::string& filepath, int width, int height, double fps);
    void close();
    
    bool writeFrame(const Image& frame);
    
    bool isOpen() const { return opened; }
    
private:
    std::string filepath;
    int width;
    int height;
    double fps;
    bool opened;
    int frameCount;
};

/**
 * @brief معالج الفيديو
 */
class VideoProcessor {
public:
    VideoProcessor();
    ~VideoProcessor() = default;
    
    VideoCapture openVideo(const std::string& filepath);
    VideoWriter createVideoWriter(const std::string& filepath, int width, int height, double fps);
    
    // استخراج الإطارات
    std::vector<Image> extractFrames(const std::string& filepath, int startFrame = 0, int endFrame = -1);
    
    // معلومات الفيديو
    struct VideoInfo {
        int width;
        int height;
        double fps;
        int frameCount;
        double duration;
    };
    
    VideoInfo getVideoInfo(const std::string& filepath);
};

// ==================== التحليل في الوقت الفعلي ====================

/**
 * @brief كاميرا
 */
class CameraCapture {
public:
    CameraCapture();
    ~CameraCapture();
    
    bool open(int cameraIndex = 0);
    void close();
    
    bool readFrame(Image& frame);
    Image readFrame();
    bool isOpen() const { return opened; }
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
private:
    int cameraIndex;
    int width;
    int height;
    bool opened;
    
    // في التطبيق الفعلي، سيكون هناك معالج كاميرا حقيقي
    void initializeCamera();
};

/**
 * @brief محلل الفيديو في الوقت الفعلي
 */
class RealTimeVideoAnalyzer {
public:
    RealTimeVideoAnalyzer();
    ~RealTimeVideoAnalyzer() = default;
    
    // فتح الكاميرا
    CameraCapture openCamera(int cameraIndex = 0);
    
    // كشف الحركة
    MotionDetection detectMotion(const Image& currentFrame, const Image& previousFrame, 
                                  double threshold = 30.0);
    
    // تتبع الكائنات
    std::vector<Rectangle> trackObjects(const Image& frame, const std::vector<Rectangle>& previousObjects);
    Rectangle trackObjectByColor(const Image& frame, const Color& lower, const Color& upper);
    
    // رسم التعليقات التوضيحية
    Image annotateFrame(const Image& frame, const std::vector<Face>& faces, 
                       const MotionDetection& motion);
    
    // تحليل الأداء
    struct PerformanceMetrics {
        double fps;
        double processingTime;
        int detectedFaces;
        double motionIntensity;
    };
    
    PerformanceMetrics getMetrics() const { return metrics; }
    
private:
    PerformanceMetrics metrics;
    Image previousFrame;
    std::vector<Rectangle> trackedObjects;
    
    void updateMetrics(double processingTime, int faces, double motion);
};

// ==================== Optical Flow & Background Subtraction ====================

class OpticalFlow {
public:
    OpticalFlow() = default;
    // Lucas-Kanade مبسط على شبكة نقاط
    std::vector<std::pair<std::pair<int,int>, std::pair<int,int>>> denseFlow(const Image& prev, const Image& next, int step = 8, int win = 5);
};

class BackgroundSubtractorSimple {
public:
    BackgroundSubtractorSimple(double alpha = 0.05) : alpha(alpha), initialized(false) {}
    Image apply(const Image& frame, int threshold = 25);
private:
    double alpha;
    bool initialized;
    Image background;
};

// ==================== دوال الاختبار ====================

void testImageProcessing();
void testShapeDetection();
void testFaceDetection();
void testVideoProcessing();
void testRealTimeAnalysis();

// ==================== التعرف الضوئي على الحروف (OCR) ====================

/**
 * @brief وحدة التعرف الضوئي على الحروف العربية
 */
class ArabicOCR {
public:
    ArabicOCR();
    ~ArabicOCR() = default;

    /**
     * @brief التعرف على النص في الصورة بالكامل
     */
    std::string recognizeText(const Image& img);

    /**
     * @brief التعرف على الحروف في منطقة محددة
     */
    std::string recognizeRegion(const Image& img, const Rectangle& rect);

    /**
     * @brief تقسيم الصورة إلى أسطر وكلمات
     */
    std::vector<Rectangle> segmentText(const Image& img);

    /**
     * @brief تحسين الصورة لعملية الـ OCR (تعديل الميل، إزالة الضجيج)
     */
    Image preprocessForOCR(const Image& img);

private:
    /**
     * @brief التعرف على حرف واحد (محرك داخلي يعتمد على عزوم Hu أو واصفات الميزات)
     */
    char32_t recognizeCharacter(const Image& charImg);

    /**
     * @brief تصحيح ميل النص
     */
    double detectSkewAngle(const Image& img);
};

// ==================== أدوات التدريب (Training Tools) ====================

/**
 * @brief مدير مجموعات البيانات للتدريب
 */
class DatasetManager {
public:
    DatasetManager();
    ~DatasetManager() = default;

    /**
     * @brief حفظ عينة تدريب (صورة + تصنيف)
     */
    bool saveSample(const Image& img, const std::string& label, const std::string& datasetPath);

    /**
     * @brief توليد بيانات اصطناعية (Augmentation)
     */
    std::vector<Image> augmentImage(const Image& img);

    /**
     * @brief تحميل مجموعة بيانات للتدريب
     */
    std::vector<std::pair<Image, std::string>> loadDataset(const std::string& datasetPath);

private:
    std::string sanitizeFilename(const std::string& name);
};

// ==================== تسريع العتاد (Hardware Acceleration) ====================

/**
 * @brief معالج العمليات المتوازية
 */
class ParallelProcessor {
public:
    /**
     * @brief تنفيذ عملية على نطاق من البيانات بشكل متوازٍ
     */
    template<typename Func>
    static void parallelFor(int start, int end, Func func) {
        int numThreads = std::thread::hardware_concurrency();
        if (numThreads < 1) numThreads = 1;
        
        int range = end - start;
        if (range <= 0) return;

        if (numThreads < 2 || range < 100) {
            for (int i = start; i < end; ++i) func(i);
            return;
        }

        std::vector<std::thread> threads;
        int grainSize = range / numThreads;
        if (grainSize < 1) grainSize = 1;

        for (int i = 0; i < numThreads; ++i) {
            int tStart = start + i * grainSize;
            int tEnd = (i == numThreads - 1) ? end : tStart + grainSize;
            
            if (tStart >= end) break;

            threads.emplace_back([tStart, tEnd, func]() {
                for (int j = tStart; j < tEnd; ++j) func(j);
            });
        }
        for (auto& t : threads) {
            if (t.joinable()) t.join();
        }
    }
};

} // namespace ArabicLanguage

#endif // ARABIC_COMPUTER_VISION_H

