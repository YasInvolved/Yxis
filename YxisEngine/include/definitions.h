#pragma once

#ifdef YX_WINDOWS
   #ifdef YX_EXPORT_SYMBOLS
      #define YX_API __declspec(dllexport)
   #else
      #define YX_API __declspec(dllimport)
   #endif
#else
   #define YX_API
#endif