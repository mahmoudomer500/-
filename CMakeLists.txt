cmake_minimum_required(VERSION 3.10)
project(ArabicProgrammingLanguage VERSION 2.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# اكتشاف الـ toolset تلقائياً — يدعم VS 2022 (v143) وVS 2025 (v180)
if(MSVC)
    if(MSVC_VERSION GREATER_EQUAL 1940)
        # Visual Studio 2025 (v18x)
        message(STATUS "Detected Visual Studio 2025 toolset")
    elseif(MSVC_VERSION GREATER_EQUAL 1930)
        # Visual Studio 2022 (v14x)
        message(STATUS "Detected Visual Studio 2022 toolset")
    endif()
endif()

# Compiler-specific flags to handle warnings
if(MSVC)
    # Disable specific MSVC warnings
    add_compile_options(
        /FS                           # Force synchronous PDB writes (fixes C1041)
        /W4                           # Warning level 4
        /wd4267                       # conversion from 'size_t' to 'uint32_t'
        /wd4244                       # conversion from 'double' to 'float'
        /wd4996                       # strcpy unsafe function warning
        /wd4028                       # formal parameter different from declaration
        /wd4505                       # unreferenced local function has been removed
        /wd4100                       # unreferenced parameter
        /wd4189                       # local variable is initialized but not referenced
        /wd4456                       # declaration of 'x' hides class member
        /wd4457                       # declaration of 'x' hides parameter
        /wd4555                       # result of expression not used
        /wd4245                       # conversion from 'int' to 'DWORD', signed/unsigned mismatch
        /wd4305                       # truncation from 'double' to 'float'
    )
    # Enable security features but allow unsafe functions (for compatibility)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    # GCC/Clang flags
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
endif()

# Include Directories
include_directories(
    src/core
    src/include
    src/runtime
    src/modules
    src/modules/ui
    src/modules/graphics
    src/modules/ai
    src/modules/web
    src/modules/mobile
    src/modules/edu
    src/modules/enterprise
    src/modules/core
    src/utils/core
    src/utils/debug
    src/stdlib
    src/stdlib/io
    src/stdlib/math
    src/stdlib/collections
    src/api
)

# Source Groups (for IDE organization)
file(GLOB_RECURSE CORE_SOURCES 
    "src/core/*.cpp"
)

file(GLOB_RECURSE RUNTIME_SOURCES "src/runtime/*.cpp")
file(GLOB_RECURSE UI_SOURCES "src/modules/ui/*.cpp")
file(GLOB_RECURSE GRAPHICS_SOURCES "src/modules/graphics/*.cpp")
file(GLOB_RECURSE AI_SOURCES "src/modules/ai/*.cpp")
file(GLOB_RECURSE UTILS_SOURCES "src/utils/core/*.cpp" "src/utils/debug/*.cpp")
file(GLOB_RECURSE STDLIB_SOURCES "src/stdlib/*.cpp")

# --- Target: Arabic Compiler Core (Static Library) ---
add_library(ArabicCompilerCore STATIC 
    ${CORE_SOURCES}
    ${RUNTIME_SOURCES}
    ${UTILS_SOURCES}
    ${STDLIB_SOURCES}
    ${AI_SOURCES}
    ${UI_SOURCES}
    ${GRAPHICS_SOURCES}
)

# Add preprocessor definitions
target_compile_definitions(ArabicCompilerCore PRIVATE 
    ARABIC_ONLY_MODE=1
    GREAT_DETACHMENT_COMPLETED=1
    BUILD_TIMESTAMP="2026-04-01-FORCE-FULL-REBUILD-V3"
)

# --- Target: Arabic Compiler Executable ---
add_executable(arabic_compiler src/main.cpp)
if(WIN32)
    target_link_libraries(arabic_compiler 
        ArabicCompilerCore
        gdiplus
        ole32
        oleaut32
        strmiids
        uuid
        user32
        gdi32
        shell32
        comdlg32
    )
else()
    target_link_libraries(arabic_compiler ArabicCompilerCore)
endif()

# --- Target: Al-Biruni IDE ---
if(WIN32)
    set(IDE_SOURCES 
        "src/ide/ArabicIDE.cpp" 
        "src/ide/ide_main.cpp"
        "src/modules/ui/ArabicUI.cpp"
        "src/modules/ui/ArabicCodeEditor.cpp"
        "src/modules/graphics/ArabicGraphics.cpp"
        "src/modules/graphics/ArabicGameEngine.cpp"
        "src/modules/ai/ArabicVisionBridge.cpp"
        "src/modules/ai/ArabicComputerVision.cpp"
    )
    add_executable(AlBiruniIDE WIN32 ${IDE_SOURCES})
    target_link_libraries(AlBiruniIDE 
        ArabicCompilerCore
        comctl32 
        comdlg32
        opengl32
        user32
        gdi32
        shell32
        gdiplus
        ole32
        oleaut32
        strmiids
        uuid
        xinput
        dsound
    )
    
    # Enable Unicode for Windows
    target_compile_definitions(AlBiruniIDE PRIVATE UNICODE _UNICODE)
endif()

# Installation
install(TARGETS arabic_compiler AlBiruniIDE RUNTIME DESTINATION bin)

# Build Status
message(STATUS "=================================================")
message(STATUS "Arabic Programming Language v2.0.0 - Build Config")
message(STATUS "C++ Standard: C++17")
message(STATUS "Compiler: MSVC (with precompiled headers)")
message(STATUS "=================================================")
