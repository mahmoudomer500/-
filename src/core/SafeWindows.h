#ifndef SAFE_WINDOWS_H
#define SAFE_WINDOWS_H

#ifdef _WIN32
  // Prevent strict min/max macros if desired
  #ifndef NOMINMAX
  #define NOMINMAX
  #endif

  // Rename byte to win_byte to avoid conflict with std::byte
  #define byte win_byte
  #include <windows.h>
  #undef byte
#endif

#endif // SAFE_WINDOWS_H
