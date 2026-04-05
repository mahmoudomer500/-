#!/bin/bash
# build_direct.sh - بناء مباشر لـ IDE البيروني (Linux)
# Direct Build Script - Builds the Full IDE for Linux

set -e

echo "═══════════════════════════════════════════════════════"
echo "  بناء IDE البيروني الكامل - Linux"
echo "  Al-Biruni IDE Full Build - Linux"
echo "═══════════════════════════════════════════════════════"
echo ""

# التحقق من الأدوات
echo "[1/3] التحقق من الأدوات..."
if ! command -v g++ &> /dev/null; then
    echo "❌ g++ غير مثبت. قم بتثبيته: sudo apt install g++"
    exit 1
fi
echo "✅ g++ $(g++ --version | head -1)"
echo ""

# إنشاء مجلد البناء
echo "[2/3] إعداد مجلد البناء..."
mkdir -p build_release/Release
if [ -f "build_release/Release/AlBiruniIDE" ]; then
    echo "🗑️ حذف الملف القديم..."
    rm -f build_release/Release/AlBiruniIDE
fi
echo ""

# بناء IDE
echo "[3/3] بناء IDE (61 ملف)..."
g++ -std=c++17 -O2 -D_CRT_SECURE_NO_WARNINGS -DARABIC_ONLY_MODE=1 \
    -finput-charset=UTF-8 -fexec-charset=UTF-8 \
    -Isrc/core -Isrc/include -Isrc/runtime -Isrc/modules \
    -Isrc/modules/ui -Isrc/modules/graphics -Isrc/modules/ai \
    -Isrc/modules/core \
    -Isrc/utils/core -Isrc/utils/debug \
    -Isrc/stdlib -Isrc/stdlib/io -Isrc/stdlib/math -Isrc/stdlib/collections \
    -Isrc/ide \
    src/ide/ide_main.cpp \
    src/ide/ArabicIDE.cpp \
    src/modules/ui/ArabicUI.cpp \
    src/modules/ui/ArabicCodeEditor.cpp \
    src/modules/graphics/ArabicGraphics.cpp \
    src/modules/graphics/ArabicGameEngine.cpp \
    src/modules/ai/ArabicVisionBridge.cpp \
    src/modules/ai/ArabicComputerVision.cpp \
    src/core/*.cpp \
    src/runtime/*.cpp \
    src/stdlib/*.cpp \
    src/modules/core/*.cpp \
    src/utils/core/*.cpp \
    src/utils/debug/*.cpp \
    -o build_release/Release/AlBiruniIDE \
    -lpthread -ldl -lGL -lGLU -lglut -lopenal -lalut 2>&1

if [ $? -ne 0 ]; then
    echo ""
    echo "❌ فشل البناء!"
    exit 1
fi

SIZE=$(du -h build_release/Release/AlBiruniIDE | cut -f1)

echo ""
echo "═══════════════════════════════════════════════════════"
echo "  ✅ تم بناء IDE بنجاح!"
echo "  الملف: build_release/Release/AlBiruniIDE"
echo "  الحجم: $SIZE"
echo "═══════════════════════════════════════════════════════"
