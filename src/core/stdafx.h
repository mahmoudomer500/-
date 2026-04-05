// stdafx.h - Precompiled Header for Arabic Compiler
// رؤوس مُجمّعة مسبقاً

#ifndef STDAFX_H
#define STDAFX_H

// ═══════════════════════════════════════════════════════════
// Standard Library Headers
// ═══════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cctype>
#include <cstring>
#include <mutex>
#include <thread>
#include <functional>
#include <codecvt>
#include <locale>
#include <cstdint>

// ═══════════════════════════════════════════════════════════
// Windows-specific headers
// ═══════════════════════════════════════════════════════════

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifdef Value
#undef Value
#endif
#ifdef Command
#undef Command
#endif
#endif

#endif // STDAFX_H
