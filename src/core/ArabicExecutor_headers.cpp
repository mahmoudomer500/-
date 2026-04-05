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

#include "ArabicExecutor.h"
#include "ArabicParser.h"
#include "ArabicTextUtils.h"
#include "ArabicMemoryManager.h"
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <sstream>

// ... rest of file continues as before
