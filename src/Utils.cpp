#ifndef _WIN32
#include "Python.h"
#endif

#include "Utils.h"

#include <vector>
#include <iterator>

#include "utf8.h"
//#include "Locker.h" //TODO port me

v8::Handle<v8::String> ToString(const std::string& str)
{
  v8::EscapableHandleScope scope(v8::Isolate::GetCurrent());

  return scope.Escape(
      v8::String::NewFromUtf8(
        v8::Isolate::GetCurrent(), 
        str.c_str(), 
        v8::NewStringType::kNormal, 
        str.size())
      .ToLocalChecked());
}

v8::Handle<v8::String> ToString(const std::wstring& str)
{
  v8::EscapableHandleScope scope(v8::Isolate::GetCurrent());

  if (sizeof(wchar_t) == sizeof(uint16_t))
  {
    return scope.Escape(
        v8::String::NewFromTwoByte(
          v8::Isolate::GetCurrent(), 
          reinterpret_cast<const uint16_t *>(str.c_str()), 
          v8::NewStringType::kNormal, 
          str.size())
        .ToLocalChecked());
    //return scope.Escape(v8::String::NewFromTwoByte(v8::Isolate::GetCurrent(), (const uint16_t *) str.c_str(), v8::String::kNormalString, str.size()));
  }

  std::vector<uint16_t> data(str.size()+1);

  for (size_t i=0; i<str.size(); i++)
  {
    data[i] = (uint16_t) str[i];
  }

  data[str.size()] = 0;

  return scope.Escape(
      v8::String::NewFromTwoByte(
        v8::Isolate::GetCurrent(), 
        &data[0], 
        v8::NewStringType::kNormal, 
        str.size())
      .ToLocalChecked());
  //orig: return scope.Escape(v8::String::NewFromTwoByte(v8::Isolate::GetCurrent(), &data[0], v8::String::kNormalString, str.size()));
}

v8::Handle<v8::String> ToString(py::object str)
{
  v8::EscapableHandleScope scope(v8::Isolate::GetCurrent());

  if (PyBytes_CheckExact(str.ptr()))
  {
    return scope.Escape(
        v8::String::NewFromUtf8(
          v8::Isolate::GetCurrent(), 
          PyBytes_AS_STRING(str.ptr()), 
          v8::NewStringType::kNormal, 
          PyBytes_GET_SIZE(str.ptr()))
        .ToLocalChecked());
    //orig: return scope.Escape(v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), PyBytes_AS_STRING(str.ptr()), v8::String::kNormalString, PyBytes_GET_SIZE(str.ptr())));
  }

  if (PyUnicode_CheckExact(str.ptr()))
  {
#ifndef Py_UNICODE_WIDE
    return scope.Escape(
        v8::String::NewFromTwoByte(
          v8::Isolate::GetCurrent(), 
          reinterpret_cast<const uint16_t *>(PyUnicode_AS_UNICODE(str.ptr())))
        .ToLocalChecked());
    //orig: return scope.Escape(v8::String::NewFromTwoByte(v8::Isolate::GetCurrent(),
    //  reinterpret_cast<const uint16_t *>(PyUnicode_AS_UNICODE(str.ptr()))));

#else
    Py_ssize_t len = PyUnicode_GET_SIZE(str.ptr());
    const uint32_t *p = reinterpret_cast<const uint32_t *>(PyUnicode_AS_UNICODE(str.ptr()));

    std::vector<uint16_t> data(len+1);

    for(Py_ssize_t i=0; i<len; i++)
    {
      data[i] = (uint16_t) (p[i]);
    }

    data[len] = 0;

    return scope.Escape(
        v8::String::NewFromTwoByte(
          v8::Isolate::GetCurrent(), 
          &data[0], 
          v8::NewStringType::kNormal, 
          len)
        .ToLocalChecked());
    //orig: return scope.Escape(v8::String::NewFromTwoByte(v8::Isolate::GetCurrent(), &data[0], v8::String::kNormalString, len));
#endif
  }

  return ToString(py::object(py::handle<>(::PyObject_Str(str.ptr()))));
}

v8::Handle<v8::String> DecodeUtf8(const std::string& str)
{
  v8::EscapableHandleScope scope(v8::Isolate::GetCurrent());

  std::vector<uint16_t> data;

  try
  {
    utf8::utf8to16(str.begin(), str.end(), std::back_inserter(data));

    return scope.Escape(v8::String::NewFromTwoByte(v8::Isolate::GetCurrent(), &data[0], v8::NewStringType::kNormal, data.size()).ToLocalChecked());
  }
  catch (const std::exception&)
  {
  	return scope.Escape(v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), str.c_str(), v8::NewStringType::kNormal, str.size()).ToLocalChecked());
  }
}

const std::string EncodeUtf8(const std::wstring& str)
{
  std::vector<uint8_t> data;

  if (sizeof(wchar_t) == sizeof(uint16_t))
  {
    utf8::utf16to8(str.begin(), str.end(), std::back_inserter(data));
  }
  else
  {
    utf8::utf32to8(str.begin(), str.end(), std::back_inserter(data));
  }

  return std::string((const char *) &data[0], data.size());
}


CPythonGIL::CPythonGIL()
{
  m_state = ::PyGILState_Ensure();
}

CPythonGIL::~CPythonGIL()
{
  ::PyGILState_Release(m_state);
}


