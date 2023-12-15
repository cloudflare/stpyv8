#pragma once

// Enable the built-in Python property support
#define SUPPORT_PROPERTY 1

// Enable the object lifecycle tracing support
#define SUPPORT_TRACE_LIFECYCLE 1

// ICU data file

#if defined(__linux)
#  define ICU_DATA_SYSTEM "/usr/share/stpyv8/icudtl.dat"
#  define ICU_DATA_USER   ".local/share/stpyv8/icudtl.dat"
#elif defined(__APPLE__)
#  define ICU_DATA_SYSTEM "/Library/Application Support/STPyV8/icudtl.dat"
#  define ICU_DATA_USER   "Library/Application Support/STPyV8/icudtl.dat"
#elif defined(_WIN32)
#  define ICU_DATA_SYSTEM nullptr
#  define ICU_DATA_USER   "\\STPyV8\\icudtl.dat"
#elif defined (_WIN64)
#  define ICU_DATA_SYSTEM nullptr
#  define ICU_DATA_USER   "\\STPyV8\\icudtl.dat"
#else
#  define ICU_DATA_SYSTEM nullptr
#  define ICU_DATA_USER   nullptr
#endif
