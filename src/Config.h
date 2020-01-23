#pragma once

// Enable the built-in Python property support
#define SUPPORT_PROPERTY 1

// Enable the object lifecycle tracing support
#define SUPPORT_TRACE_LIFECYCLE 1

// ICU data file
#define ICU_DATA_UNIX "/usr/share/stpyv8/icudtl.dat"

#if defined(__linux) || defined(__APPLE)
#  define ICU_DATA ICU_DATA_UNIX;
#else
#  define ICU_DATA nullptr;
#endif
