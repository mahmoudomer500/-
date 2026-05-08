#!/bin/bash
# build_and_test.sh - بناء واختبار لغة البرمجة العربية (Linux)
# Arabic Programming Language - Build and Test Script for Linux

set -e

echo "═══════════════════════════════════════════════════════"
echo "  لغة البرمجة العربية - بناء واختبار"
echo "  Arabic Programming Language - Build & Test"
echo "═══════════════════════════════════════════════════════"
echo ""

# التحقق من الأدوات
echo "[1/4] التحقق من الأدوات..."
if ! command -v g++ &> /dev/null; then
    echo "❌ g++ غير مثبت. قم بتثبيته: sudo apt install g++"
    exit 1
fi
echo "✅ g++ $(g++ --version | head -1)"
echo ""

# إنشاء مجلد البناء
echo "[2/4] إعداد مجلد البناء..."
mkdir -p build_release/Release
echo ""

# بناء المترجم
echo "[3/4] بناء المترجم..."
g++ -std=c++17 -O2 -D_CRT_SECURE_NO_WARNINGS -DARABIC_ONLY_MODE=1 \
    -finput-charset=UTF-8 -fexec-charset=UTF-8 \
    -Isrc/core -Isrc/include -Isrc/runtime -Isrc/modules \
    -Isrc/modules/core -Isrc/modules/ai \
    -Isrc/utils/core -Isrc/utils/debug \
    -Isrc/stdlib -Isrc/stdlib/io -Isrc/stdlib/math -Isrc/stdlib/collections \
    -Isrc/api \
    src/main.cpp \
    src/core/*.cpp \
    src/runtime/*.cpp \
    src/stdlib/*.cpp \
    src/modules/core/*.cpp \
    src/utils/core/*.cpp \
    src/utils/debug/*.cpp \
    -o build_release/Release/arabic_compiler \
    -lpthread -ldl 2>&1

if [ $? -ne 0 ]; then
    echo "❌ فشل البناء!"
    exit 1
fi
echo "✅ تم بناء المترجم بنجاح"
echo ""

# اختبار
echo "[4/4] اختبار المترجم..."
if [ -f "examples/basic/hello.عربي" ]; then
    ./build_release/Release/arabic_compiler --mode run examples/basic/hello.عربي
    echo ""
    echo "✅ اختبار Hello World نجح"
else
    echo "⚠️ ملف الاختبار غير موجود"
fi

echo ""
echo "═══════════════════════════════════════════════════════"
echo "  ✅ اكتمل البناء والاختبار!"
echo "  المترجم: build_release/Release/arabic_compiler"
echo "═══════════════════════════════════════════════════════"
