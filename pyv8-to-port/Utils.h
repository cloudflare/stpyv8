#pragma once

#include <string>

#ifdef _WIN32

#ifdef DEBUG
# pragma warning( push )
#endif

#pragma warning( disable : 4100 ) // 'identifier' : unreferenced formal parameter
#pragma warning( disable : 4121 ) // 'symbol' : alignment of a member was sensitive to packing
#pragma warning( disable : 4127 ) // conditional expression is constant
#pragma warning( disable : 4189 ) // 'identifier' : local variable is initialized but not referenced
#pragma warning( disable : 4244 ) // 'argument' : conversion from 'type1' to 'type2', possible loss of data
#pragma warning( disable : 4505 ) // 'function' : unreferenced local function has been removed
#pragma warning( disable : 4512 ) // 'class' : assignment operator could not be generated
#pragma warning( disable : 4800 ) // 'type' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning( disable : 4996 ) // 'function': was declared deprecated

#ifndef _WIN64
# ifndef _USE_32BIT_TIME_T
#  define _USE_32BIT_TIME_T
# endif
#endif

#else

# if !defined(__GNUC__) || (__GNUC__ <= 4 && __GNUC_MINOR__ < 7)

#include <cmath>
using std::isnan;

#ifndef isfinite
# include <limits>
namespace std {
  inline bool isfinite(double val) { return val <= std::numeric_limits<double>::max(); }
}

# endif

#endif

#include <strings.h>
#define strnicmp strncasecmp

#define _countof(array) (sizeof(array)/sizeof(array[0]))

#endif

#if defined(__APPLE__) && !defined(isalnum)
#include <pyport.h>
#undef isalnum
#undef isalpha
#undef islower
#undef isspace
#undef isupper
#undef tolower
#undef toupper
#endif

# if BOOST_VERSION / 100 % 1000 < 50
#  undef TIME_UTC
# endif

#include <boost/python.hpp>
namespace py = boost::python;

#ifdef _WIN32
  #undef FP_NAN
  #undef FP_INFINITE
  #undef FP_ZERO
  #undef FP_SUBNORMAL
  #undef FP_NORMAL
#endif

#include <v8.h>

#ifdef _WIN32

#ifdef DEBUG
# pragma warning( pop )
#endif

#endif

#if defined(__GNUC__)
  #define UNUSED_VAR(x) x __attribute__((unused))
#elif defined(__APPLE__)
  #define UNUSED_VAR(x) x __unused
#else
  #define UNUSED_VAR(x) x
#endif

#if PY_MAJOR_VERSION >= 3

#define PyInt_Check               PyLong_Check
#define PyInt_AsUnsignedLongMask  PyLong_AsUnsignedLong

#define PySlice_Cast(obj) obj

#ifdef Py_DEBUG
#define PyErr_OCCURRED() PyErr_Occurred()
#else
#define PyErr_OCCURRED() (PyThreadState_GET()->curexc_type)
#endif

#else

#define PySlice_Cast(obj) ((PySliceObject *) obj)

#define PyErr_OCCURRED() _PyErr_OCCURRED()

#endif

v8::Handle<v8::String> ToString(const std::string& str);
v8::Handle<v8::String> ToString(const std::wstring& str);
v8::Handle<v8::String> ToString(py::object str);

v8::Handle<v8::String> DecodeUtf8(const std::string& str);
const std::string EncodeUtf8(const std::wstring& str);

struct CPythonGIL
{
  PyGILState_STATE m_state;

  CPythonGIL();
  ~CPythonGIL();
};

#ifdef SUPPORT_PROBES

#define PyObject_t PyObject

typedef v8::Persistent<v8::Object> V8Object_t;
typedef v8::Persistent<v8::Object> V8Array_t;
typedef v8::Handle<v8::Script> V8Script_t;
typedef const char * string_t;

#include "probes.h"

#endif
