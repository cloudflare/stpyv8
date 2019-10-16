#include <stdlib.h>
#include <iostream>
#include <vector>

#include <boost/python/raw_function.hpp>
#include <descrobject.h>
#include <datetime.h>

#include "libplatform/libplatform.h"

#include "Wrapper.h"
#include "Context.h"
#include "Utils.h"


#define TERMINATE_EXECUTION_CHECK(returnValue) \
  if(v8::Isolate::GetCurrent()->IsExecutionTerminating()) { \
    ::PyErr_Clear(); \
    ::PyErr_SetString(PyExc_RuntimeError, "execution is terminating"); \
    return returnValue; \
  }

#define CHECK_V8_CONTEXT() \
  if (v8::Isolate::GetCurrent()->GetCurrentContext().IsEmpty()) { \
    throw CJavascriptException("Javascript object out of context", PyExc_UnboundLocalError); \
  }

std::ostream& operator <<(std::ostream& os, const CJavascriptObject& obj)
{
  obj.Dump(os);

  return os;
}


void CWrapper::Expose(void)
{
  PyDateTime_IMPORT;

  py::class_<CJavascriptObject, boost::noncopyable>("JSObject", py::no_init)
    .def("__getattr__", &CJavascriptObject::GetAttr)
    .def("__setattr__", &CJavascriptObject::SetAttr)
    .def("__delattr__", &CJavascriptObject::DelAttr)

    .def("__hash__", &CJavascriptObject::GetIdentityHash)
    .def("clone", &CJavascriptObject::Clone, "Clone the object.")

  #if PY_MAJOR_VERSION < 3
    .add_property("__members__", &CJavascriptObject::GetAttrList)
  #else
    .def("__dir__", &CJavascriptObject::GetAttrList)
  #endif

    // Emulating dict object
    .def("keys", &CJavascriptObject::GetAttrList, "Get a list of an object's attributes.")

    .def("__getitem__", &CJavascriptObject::GetAttr)
    .def("__setitem__", &CJavascriptObject::SetAttr)
    .def("__delitem__", &CJavascriptObject::DelAttr)

    .def("__contains__", &CJavascriptObject::Contains)

    .def(int_(py::self))
    .def(float_(py::self))
    .def(str(py::self))

    .def("__bool__", &CJavascriptObject::operator bool)
    .def("__eq__", &CJavascriptObject::Equals)
    .def("__ne__", &CJavascriptObject::Unequals)

    .def("create", &CJavascriptFunction::CreateWithArgs,
         (py::arg("constructor"),
          py::arg("arguments") = py::tuple(),
          py::arg("propertiesObject") = py::dict()),
         "Creates a new object with the specified prototype object and properties.")
          .staticmethod("create")
    ;

  py::class_<CJavascriptNull, py::bases<CJavascriptObject>, boost::noncopyable>("JSNull")
    .def("__bool__", &CJavascriptNull::nonzero)
    .def("__str__", &CJavascriptNull::str)
    ;

  py::class_<CJavascriptUndefined, py::bases<CJavascriptObject>, boost::noncopyable>("JSUndefined")
    .def("__bool__", &CJavascriptUndefined::nonzero)
    .def("__str__", &CJavascriptUndefined::str)
    ;

  py::class_<CJavascriptArray, py::bases<CJavascriptObject>, boost::noncopyable>("JSArray", py::no_init)
    .def(py::init<py::object>())

    .def("__len__", &CJavascriptArray::Length)

    .def("__getitem__", &CJavascriptArray::GetItem)
    .def("__setitem__", &CJavascriptArray::SetItem)
    .def("__delitem__", &CJavascriptArray::DelItem)

    .def("__iter__", py::range(&CJavascriptArray::begin, &CJavascriptArray::end))

    .def("__contains__", &CJavascriptArray::Contains)
    ;

  py::class_<CJavascriptFunction, py::bases<CJavascriptObject>, boost::noncopyable>("JSFunction", py::no_init)
    .def("__call__", py::raw_function(&CJavascriptFunction::CallWithArgs))

    .def("apply", &CJavascriptFunction::ApplyJavascript,
         (py::arg("self"),
          py::arg("args") = py::list(),
          py::arg("kwds") = py::dict()),
          "Performs a function call using the parameters.")
    .def("apply", &CJavascriptFunction::ApplyPython,
         (py::arg("self"),
          py::arg("args") = py::list(),
          py::arg("kwds") = py::dict()),
          "Performs a function call using the parameters.")
    .def("invoke", &CJavascriptFunction::Invoke,
          (py::arg("args") = py::list(),
           py::arg("kwds") = py::dict()),
          "Performs a binding method call using the parameters.")

    .def("setName", &CJavascriptFunction::SetName)

    .add_property("name", &CJavascriptFunction::GetName, &CJavascriptFunction::SetName, "The name of function")
    .add_property("owner", &CJavascriptFunction::GetOwner)

    .add_property("linenum", &CJavascriptFunction::GetLineNumber, "The line number of function in the script")
    .add_property("colnum", &CJavascriptFunction::GetColumnNumber, "The column number of function in the script")
    .add_property("resname", &CJavascriptFunction::GetResourceName, "The resource name of script")
    .add_property("inferredname", &CJavascriptFunction::GetInferredName, "Name inferred from variable or property assignment of this function")
    .add_property("lineoff", &CJavascriptFunction::GetLineOffset, "The line offset of function in the script")
    .add_property("coloff", &CJavascriptFunction::GetColumnOffset, "The column offset of function in the script")
    ;
    py::objects::class_value_wrapper<boost::shared_ptr<CJavascriptObject>,
    py::objects::make_ptr_instance<CJavascriptObject,
    py::objects::pointer_holder<boost::shared_ptr<CJavascriptObject>,CJavascriptObject> > >();
}


// =====================================================================
// CPythonObject Start
// =====================================================================

void CPythonObject::ThrowIf(v8::Isolate* isolate)
{
  CPythonGIL python_gil;

  assert(PyErr_OCCURRED());

  v8::HandleScope handle_scope(isolate);

  PyObject *exc, *val, *trb;

  ::PyErr_Fetch(&exc, &val, &trb);
  ::PyErr_NormalizeException(&exc, &val, &trb);

  py::object type(py::handle<>(py::allow_null(exc))),
             value(py::handle<>(py::allow_null(val)));

  if (trb) py::decref(trb);

  std::string msg;

  if (::PyObject_HasAttrString(value.ptr(), "args"))
  {
    py::object args = value.attr("args");

    if (PyTuple_Check(args.ptr()))
    {
      for (Py_ssize_t i=0; i<PyTuple_GET_SIZE(args.ptr()); i++)
      {
        py::extract<const std::string> extractor(args[i]);

        if (extractor.check()) msg += extractor();
      }
    }
  }
  else if (::PyObject_HasAttrString(value.ptr(), "message"))
  {
    py::extract<const std::string> extractor(value.attr("message"));

    if (extractor.check()) msg = extractor();
  }
  else if (val)
  {
    if (PyBytes_CheckExact(val))
    {
      msg = PyBytes_AS_STRING(val);
    }
    else if (PyTuple_CheckExact(val))
    {
      for (int i=0; i<PyTuple_GET_SIZE(val); i++)
      {
        PyObject *item = PyTuple_GET_ITEM(val, i);

        if (item && PyBytes_CheckExact(item))
        {
          msg = PyBytes_AS_STRING(item);
          break;
        }
      }
    }
  }

  v8::Handle<v8::Value> error;

  if (::PyErr_GivenExceptionMatches(type.ptr(), ::PyExc_IndexError))
  {
    error = v8::Exception::RangeError(v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
  }
  else if (::PyErr_GivenExceptionMatches(type.ptr(), ::PyExc_AttributeError))
  {
    error = v8::Exception::ReferenceError(v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
  }
  else if (::PyErr_GivenExceptionMatches(type.ptr(), ::PyExc_SyntaxError))
  {
    error = v8::Exception::SyntaxError(v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
  }
  else if (::PyErr_GivenExceptionMatches(type.ptr(), ::PyExc_TypeError))
  {
    error = v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
  }
  else
  {
    error = v8::Exception::Error(v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
  }

  if (error->IsObject())
  {
    // FIXME How to trace the lifecycle of exception? and when to delete those object in the hidden value?

  /* TODO port me
  #ifdef SUPPORT_TRACE_EXCEPTION_LIFECYCLE
    error->ToObject()->SetHiddenValue(v8::String::NewFromUtf8(isolate, "exc_type"),
                                      v8::External::New(isolate, ObjectTracer::Trace(error, new py::object(type)).Object()));
    error->ToObject()->SetHiddenValue(v8::String::NewFromUtf8(isolate, "exc_value"),
                                      v8::External::New(isolate, ObjectTracer::Trace(error, new py::object(value)).Object()));
  #else
    error->ToObject()->SetHiddenValue(v8::String::NewFromUtf8(isolate, "exc_type"),
                                      v8::External::New(isolate, new py::object(type)));
    error->ToObject()->SetHiddenValue(v8::String::NewFromUtf8(isolate, "exc_value"),
                                      v8::External::New(isolate, new py::object(value)));
  #endif
  */
  }

  isolate->ThrowException(error);
}

#define _TERMINATE_CALLBACK_EXECUTION_CHECK(returnValue) \
  if(v8::Isolate::GetCurrent()->IsExecutionTerminating()) { \
    ::PyErr_Clear(); \
    ::PyErr_SetString(PyExc_RuntimeError, "execution is terminating"); \
    info.GetReturnValue().Set(returnValue); \
    return; \
  }

#define TRY_HANDLE_EXCEPTION(value) _TERMINATE_CALLBACK_EXECUTION_CHECK(value) \
                                    BEGIN_HANDLE_PYTHON_EXCEPTION \
                                    {
#define END_HANDLE_EXCEPTION(value) } \
                                    END_HANDLE_PYTHON_EXCEPTION \
                                    info.GetReturnValue().Set(value);

#define CALLBACK_RETURN(value) do { info.GetReturnValue().Set(value); return; } while(0);

CPythonObject::CPythonObject()
{
}

CPythonObject::~CPythonObject()
{
}

void CPythonObject::NamedGetter(v8::Local<v8::Name> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()))

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  v8::String::Utf8Value name(info.GetIsolate(), prop);

  if (PyGen_Check(obj.ptr())) CALLBACK_RETURN(v8::Undefined(info.GetIsolate()));

  PyObject *value = ::PyObject_GetAttrString(obj.ptr(), *name);

  if (!value)
  {
    if (_PyErr_OCCURRED())
    {
      if (::PyErr_ExceptionMatches(::PyExc_AttributeError))
      {
        ::PyErr_Clear();
      }
      else
      {
        py::throw_error_already_set();
      }
    }

    if (::PyMapping_Check(obj.ptr()) &&
        ::PyMapping_HasKeyString(obj.ptr(), *name))
    {
      py::object result(py::handle<>(::PyMapping_GetItemString(obj.ptr(), *name)));

      if (!result.is_none()) CALLBACK_RETURN(Wrap(result));
    }

    CALLBACK_RETURN(v8::Handle<v8::Value>());
  }

  py::object attr = py::object(py::handle<>(value));

/* TODO port me
#ifdef SUPPORT_PROPERTY
  if (PyObject_TypeCheck(attr.ptr(), &::PyProperty_Type))
  {
    py::object getter = attr.attr("fget");

    if (getter.is_none())
      throw CJavascriptException("unreadable attribute", ::PyExc_AttributeError);

    attr = getter();
  }
#endif
*/

  CALLBACK_RETURN(Wrap(attr));

  END_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()))

}

void CPythonObject::NamedSetter(v8::Local<v8::Name> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()))

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  v8::String::Utf8Value name(info.GetIsolate(), prop);
  py::object newval = CJavascriptObject::Wrap(value);

  bool found = 1 == ::PyObject_HasAttrString(obj.ptr(), *name);

  if (::PyObject_HasAttrString(obj.ptr(), "__watchpoints__"))
  {
    py::dict watchpoints(obj.attr("__watchpoints__"));
    py::str propname(*name, name.length());

    if (watchpoints.has_key(propname))
    {
      py::object watchhandler = watchpoints.get(propname);

      newval = watchhandler(propname, found ? obj.attr(propname) : py::object(), newval);
    }
  }

  if (!found && ::PyMapping_Check(obj.ptr()))
  {
    ::PyMapping_SetItemString(obj.ptr(), *name, newval.ptr());
  }
  else
  {
    /* TODO port me
  #ifdef SUPPORT_PROPERTY
    if (found)
    {
      py::object attr = obj.attr(*name);

      if (PyObject_TypeCheck(attr.ptr(), &::PyProperty_Type))
      {
        py::object setter = attr.attr("fset");

        if (setter.is_none())
          throw CJavascriptException("can't set attribute", ::PyExc_AttributeError);

        setter(newval);

        CALLBACK_RETURN(value);
      }
    }
  #endif
  */
    obj.attr(*name) = newval;
  }

  CALLBACK_RETURN(value);

  END_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()));
}

void CPythonObject::NamedQuery(v8::Local<v8::Name> prop, const v8::PropertyCallbackInfo<v8::Integer>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Handle<v8::Integer>())

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  v8::String::Utf8Value name(info.GetIsolate(), prop);

  bool exists = PyGen_Check(obj.ptr()) || ::PyObject_HasAttrString(obj.ptr(), *name) ||
                (::PyMapping_Check(obj.ptr()) && ::PyMapping_HasKeyString(obj.ptr(), *name));

  if (exists) CALLBACK_RETURN(v8::Integer::New(info.GetIsolate(), v8::None));

  END_HANDLE_EXCEPTION(v8::Handle<v8::Integer>())
}

void CPythonObject::NamedDeleter(v8::Local<v8::Name> prop, const v8::PropertyCallbackInfo<v8::Boolean>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Handle<v8::Boolean>())

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  v8::String::Utf8Value name(info.GetIsolate(), prop);

  if (!::PyObject_HasAttrString(obj.ptr(), *name) &&
      ::PyMapping_Check(obj.ptr()) &&
      ::PyMapping_HasKeyString(obj.ptr(), *name))
  {
    CALLBACK_RETURN(-1 != ::PyMapping_DelItemString(obj.ptr(), *name));
  }
  else
  {
    /* TODO port me
  #ifdef SUPPORT_PROPERTY
    py::object attr = obj.attr(*name);

    if (::PyObject_HasAttrString(obj.ptr(), *name) &&
        PyObject_TypeCheck(attr.ptr(), &::PyProperty_Type))
    {
      py::object deleter = attr.attr("fdel");

      if (deleter.is_none())
        throw CJavascriptException("can't delete attribute", ::PyExc_AttributeError);

      CALLBACK_RETURN(py::extract<bool>(deleter()));
    }
    else
    {
      CALLBACK_RETURN(-1 != ::PyObject_DelAttrString(obj.ptr(), *name));
    }
  #else
  */
    CALLBACK_RETURN(-1 != ::PyObject_DelAttrString(obj.ptr(), *name));
    /* TODO port me
  #endif
  */
  }

  END_HANDLE_EXCEPTION(v8::Handle<v8::Boolean>())
}

#pragma GCC diagnostic ignored "-Wunused-result"

void CPythonObject::NamedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Handle<v8::Array>())

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  py::list keys;
  bool filter_name = false;

  if (::PySequence_Check(obj.ptr()))
  {
    CALLBACK_RETURN(v8::Handle<v8::Array>());
  }
  else if (::PyMapping_Check(obj.ptr()))
  {
    keys = py::list(py::handle<>(PyMapping_Keys(obj.ptr())));
  }
  else if (PyGen_CheckExact(obj.ptr()))
  {
    py::object iter(py::handle<>(::PyObject_GetIter(obj.ptr())));

    PyObject *item = NULL;

    while (NULL != (item = ::PyIter_Next(iter.ptr())))
    {
      keys.append(py::object(py::handle<>(item)));
    }
  }
  else
  {
    keys = py::list(py::handle<>(::PyObject_Dir(obj.ptr())));
    filter_name = true;
  }

  Py_ssize_t len = PyList_GET_SIZE(keys.ptr());
  v8::Handle<v8::Array> result = v8::Array::New(info.GetIsolate(), len);

  if (len > 0)
  {
    for (Py_ssize_t i=0; i<len; i++)
    {
      PyObject *item = PyList_GET_ITEM(keys.ptr(), i);

      if (filter_name && PyBytes_CheckExact(item))
      {
        py::str name(py::handle<>(py::borrowed(item)));

        // FIXME Are there any methods to avoid such a dirty work?

        if (name.startswith("__") && name.endswith("__"))
          continue;
      }

      result->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), v8::Uint32::New(info.GetIsolate(), i), Wrap(py::object(py::handle<>(py::borrowed(item)))));
    }

    CALLBACK_RETURN(result);
  }

  END_HANDLE_EXCEPTION(v8::Handle<v8::Array>())
}

void CPythonObject::IndexedGetter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()));

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  if (PyGen_Check(obj.ptr())) CALLBACK_RETURN(v8::Undefined(info.GetIsolate()));

  if (::PySequence_Check(obj.ptr()))
  {
    if ((Py_ssize_t) index < ::PySequence_Size(obj.ptr()))
    {
      py::object ret(py::handle<>(::PySequence_GetItem(obj.ptr(), index)));

      CALLBACK_RETURN(Wrap(ret));
    }
  }
  else if (::PyMapping_Check(obj.ptr()))
  {
    char buf[65];

    snprintf(buf, sizeof(buf), "%d", index);

    PyObject *value = ::PyMapping_GetItemString(obj.ptr(), buf);

    if (!value)
    {
      py::long_ key(index);

      value = ::PyObject_GetItem(obj.ptr(), key.ptr());
    }

    if (value)
    {
      CALLBACK_RETURN(Wrap(py::object(py::handle<>(value))));
    }
  }

  END_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()))
}

void CPythonObject::IndexedSetter(uint32_t index, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()));

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  if (::PySequence_Check(obj.ptr()))
  {
    if (::PySequence_SetItem(obj.ptr(), index, CJavascriptObject::Wrap(value).ptr()) < 0)
      info.GetIsolate()->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(info.GetIsolate(), "fail to set indexed value").ToLocalChecked()));
  }
  else if (::PyMapping_Check(obj.ptr()))
  {
    char buf[65];

    snprintf(buf, sizeof(buf), "%d", index);

    if (::PyMapping_SetItemString(obj.ptr(), buf, CJavascriptObject::Wrap(value).ptr()) < 0)
      info.GetIsolate()->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(info.GetIsolate(), "fail to set named value").ToLocalChecked()));
  }

  CALLBACK_RETURN(value);

  END_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()))
}

void CPythonObject::IndexedQuery(uint32_t index, const v8::PropertyCallbackInfo<v8::Integer>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Handle<v8::Integer>());

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  if (PyGen_Check(obj.ptr())) CALLBACK_RETURN(v8::Integer::New(info.GetIsolate(), v8::ReadOnly));

  if (::PySequence_Check(obj.ptr()))
  {
    if ((Py_ssize_t) index < ::PySequence_Size(obj.ptr()))
    {
      CALLBACK_RETURN(v8::Integer::New(info.GetIsolate(), v8::None));
    }
  }
  else if (::PyMapping_Check(obj.ptr()))
  {
    char buf[65];

    snprintf(buf, sizeof(buf), "%d", index);

    if (::PyMapping_HasKeyString(obj.ptr(), buf) ||
        ::PyMapping_HasKey(obj.ptr(), py::long_(index).ptr()))
    {
      CALLBACK_RETURN(v8::Integer::New(info.GetIsolate(), v8::None));
    }
  }

  END_HANDLE_EXCEPTION(v8::Handle<v8::Integer>())
}

void CPythonObject::IndexedDeleter(uint32_t index, const v8::PropertyCallbackInfo<v8::Boolean>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Handle<v8::Boolean>());

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  if (::PySequence_Check(obj.ptr()) && (Py_ssize_t) index < ::PySequence_Size(obj.ptr()))
  {
    CALLBACK_RETURN(0 <= ::PySequence_DelItem(obj.ptr(), index));
  }
  else if (::PyMapping_Check(obj.ptr()))
  {
    char buf[65];

    snprintf(buf, sizeof(buf), "%d", index);

    CALLBACK_RETURN(PyMapping_DelItemString(obj.ptr(), buf) == 0);
  }

  END_HANDLE_EXCEPTION(v8::Handle<v8::Boolean>())
}

void CPythonObject::IndexedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Handle<v8::Array>());

  CPythonGIL python_gil;

  py::object obj = CJavascriptObject::Wrap(info.Holder());

  Py_ssize_t len = ::PySequence_Check(obj.ptr()) ? ::PySequence_Size(obj.ptr()) : 0;

  v8::Handle<v8::Array> result = v8::Array::New(info.GetIsolate(), len);

  for (Py_ssize_t i=0; i<len; i++)
  {
    result->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), v8::Uint32::New(info.GetIsolate(), i), v8::Int32::New(info.GetIsolate(), i)->ToString(v8::Isolate::GetCurrent()->GetCurrentContext()).ToLocalChecked());
  }

  CALLBACK_RETURN(result);

  END_HANDLE_EXCEPTION(v8::Handle<v8::Array>())
}

#define GEN_ARG(z, n, data) CJavascriptObject::Wrap(info[n])
#define GEN_ARGS(count) BOOST_PP_ENUM(count, GEN_ARG, NULL)

#define GEN_CASE_PRED(r, state) \
  BOOST_PP_NOT_EQUAL( \
    BOOST_PP_TUPLE_ELEM(2, 0, state), \
    BOOST_PP_INC(BOOST_PP_TUPLE_ELEM(2, 1, state)) \
  ) \
  /**/

#define GEN_CASE_OP(r, state) \
  ( \
    BOOST_PP_INC(BOOST_PP_TUPLE_ELEM(2, 0, state)), \
    BOOST_PP_TUPLE_ELEM(2, 1, state) \
  ) \
  /**/

#define GEN_CASE_MACRO(r, state) \
  case BOOST_PP_TUPLE_ELEM(2, 0, state): { \
    result = self(GEN_ARGS(BOOST_PP_TUPLE_ELEM(2, 0, state))); \
    break; \
  } \
  /**/

void CPythonObject::Caller(const v8::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope handle_scope(info.GetIsolate());

  TRY_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()));

  CPythonGIL python_gil;

  py::object self;

  if (!info.Data().IsEmpty() && info.Data()->IsExternal())
  {
    v8::Handle<v8::External> field = v8::Handle<v8::External>::Cast(info.Data());

    self = *static_cast<py::object *>(field->Value());
  }
  else
  {
    self = CJavascriptObject::Wrap(info.This());
  }

  py::object result;

  switch (info.Length())
  {
    BOOST_PP_FOR((0, 10), GEN_CASE_PRED, GEN_CASE_OP, GEN_CASE_MACRO)
  default:
    info.GetIsolate()->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(info.GetIsolate(), "too many arguments").ToLocalChecked()));

    CALLBACK_RETURN(v8::Undefined(info.GetIsolate()));
  }

  CALLBACK_RETURN(Wrap(result));

  END_HANDLE_EXCEPTION(v8::Undefined(info.GetIsolate()))
}

void CPythonObject::SetupObjectTemplate(v8::Isolate *isolate, v8::Handle<v8::ObjectTemplate> clazz)
{
  v8::HandleScope handle_scope(isolate);

  clazz->SetInternalFieldCount(1);
  clazz->SetHandler(v8::NamedPropertyHandlerConfiguration(NamedGetter, NamedSetter, NamedQuery, NamedDeleter, NamedEnumerator));
  clazz->SetIndexedPropertyHandler(IndexedGetter, IndexedSetter, IndexedQuery, IndexedDeleter, IndexedEnumerator);
  clazz->SetCallAsFunctionHandler(Caller);
}

v8::Handle<v8::ObjectTemplate> CPythonObject::CreateObjectTemplate(v8::Isolate *isolate)
{
  v8::EscapableHandleScope handle_scope(isolate);

  v8::Local<v8::ObjectTemplate> clazz = v8::ObjectTemplate::New(isolate);

  SetupObjectTemplate(isolate, clazz);

  return handle_scope.Escape(clazz);
}

bool CPythonObject::IsWrapped(v8::Handle<v8::Object> obj)
{
  return obj->InternalFieldCount() == 1;
}

py::object CPythonObject::Unwrap(v8::Handle<v8::Object> obj)
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  v8::Handle<v8::External> payload = v8::Handle<v8::External>::Cast(obj->GetInternalField(0));

  return *static_cast<py::object *>(payload->Value());
}

void CPythonObject::Dispose(v8::Handle<v8::Value> value)
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  if (value->IsObject())
  {
    v8::MaybeLocal<v8::Object> objMaybe = value->ToObject(v8::Isolate::GetCurrent()->GetCurrentContext());

    if (objMaybe.IsEmpty()) 
    {
      return;
    }

    v8::Handle<v8::Object> obj = objMaybe.ToLocalChecked();

    if (IsWrapped(obj))
    {
      Py_DECREF(CPythonObject::Unwrap(obj).ptr());
    }
  }
}

v8::Handle<v8::Value> CPythonObject::Wrap(py::object obj)
{
  v8::EscapableHandleScope handle_scope(v8::Isolate::GetCurrent());

  v8::Local<v8::Value> value;

  //TODO port me
  //#ifdef SUPPORT_TRACE_LIFECYCLE
  //value = ObjectTracer::FindCache(obj);
  //
  //if (value.IsEmpty())
  //#endif

  value = WrapInternal(obj);

  return handle_scope.Escape(value);
}

v8::Handle<v8::Value> CPythonObject::WrapInternal(py::object obj)
{
  assert(v8::Isolate::GetCurrent()->InContext());

  v8::EscapableHandleScope handle_scope(v8::Isolate::GetCurrent());

  v8::TryCatch try_catch(v8::Isolate::GetCurrent());

  CPythonGIL python_gil;

  TERMINATE_EXECUTION_CHECK(v8::Undefined(v8::Isolate::GetCurrent()))

  if (obj.is_none()) return v8::Null(v8::Isolate::GetCurrent());
  if (obj.ptr() == Py_True) return v8::True(v8::Isolate::GetCurrent());
  if (obj.ptr() == Py_False) return v8::False(v8::Isolate::GetCurrent());

  py::extract<CJavascriptObject&> extractor(obj);

  if (extractor.check())
  {
    CJavascriptObject& jsobj = extractor();
    
    if (dynamic_cast<CJavascriptNull *>(&jsobj)) return v8::Null(v8::Isolate::GetCurrent());
    if (dynamic_cast<CJavascriptUndefined *>(&jsobj)) return v8::Undefined(v8::Isolate::GetCurrent());
    
    if (jsobj.Object().IsEmpty())
    {
      ILazyObject *pLazyObject = dynamic_cast<ILazyObject *>(&jsobj);

      if (pLazyObject) pLazyObject->LazyConstructor();
    }

    if (jsobj.Object().IsEmpty())
    {
      throw CJavascriptException("Refer to a null object", ::PyExc_AttributeError);
    }

    //TODO port me
    //#ifdef SUPPORT_TRACE_LIFECYCLE
    //py::object *object = new py::object(obj);
    //
    //ObjectTracer::Trace(jsobj.Object(), object);
    //#endif 

    return handle_scope.Escape(jsobj.Object());
  }

  v8::Local<v8::Value> result;

#if PY_MAJOR_VERSION < 3
  if (PyInt_CheckExact(obj.ptr()))
  {
    result = v8::Integer::New(v8::Isolate::GetCurrent(), ::PyInt_AsLong(obj.ptr()));
  }
  else
#endif
    if (PyLong_CheckExact(obj.ptr()))
    {
      result = v8::Integer::New(v8::Isolate::GetCurrent(), ::PyLong_AsLong(obj.ptr()));
    }
    else if (PyBool_Check(obj.ptr()))
    {
      result = v8::Boolean::New(v8::Isolate::GetCurrent(), py::extract<bool>(obj));
    }
    else if (PyBytes_CheckExact(obj.ptr()) ||
        PyUnicode_CheckExact(obj.ptr()))
    {
      result = ToString(obj);
    }
    else if (PyFloat_CheckExact(obj.ptr()))
    {
      result = v8::Number::New(v8::Isolate::GetCurrent(), py::extract<double>(obj));
    }
    else if (PyDateTime_CheckExact(obj.ptr()) || PyDate_CheckExact(obj.ptr()))
    {
      tm ts = { 0 };

      ts.tm_year = PyDateTime_GET_YEAR(obj.ptr()) - 1900;
      ts.tm_mon = PyDateTime_GET_MONTH(obj.ptr()) - 1;
      ts.tm_mday = PyDateTime_GET_DAY(obj.ptr());
      ts.tm_hour = PyDateTime_DATE_GET_HOUR(obj.ptr());
      ts.tm_min = PyDateTime_DATE_GET_MINUTE(obj.ptr());
      ts.tm_sec = PyDateTime_DATE_GET_SECOND(obj.ptr());
      ts.tm_isdst = -1;

      int ms = PyDateTime_DATE_GET_MICROSECOND(obj.ptr());

      result = v8::Date::New(v8::Isolate::GetCurrent()->GetCurrentContext(), ((double) mktime(&ts)) * 1000 + ms / 1000).ToLocalChecked();
    }
    else if (PyTime_CheckExact(obj.ptr()))
    {
      tm ts = { 0 };

      ts.tm_hour = PyDateTime_TIME_GET_HOUR(obj.ptr()) - 1;
      ts.tm_min = PyDateTime_TIME_GET_MINUTE(obj.ptr());
      ts.tm_sec = PyDateTime_TIME_GET_SECOND(obj.ptr());

      int ms = PyDateTime_TIME_GET_MICROSECOND(obj.ptr());

      result = v8::Date::New(v8::Isolate::GetCurrent()->GetCurrentContext(), ((double) mktime(&ts)) * 1000 + ms / 1000).ToLocalChecked();
    }
    else if (PyCFunction_Check(obj.ptr()) || PyFunction_Check(obj.ptr()) ||
        PyMethod_Check(obj.ptr()) || PyType_Check(obj.ptr()))
    {
      v8::Handle<v8::FunctionTemplate> func_tmpl = v8::FunctionTemplate::New(v8::Isolate::GetCurrent());
      py::object *object = new py::object(obj);

      func_tmpl->SetCallHandler(Caller, v8::External::New(v8::Isolate::GetCurrent(), object));

      if (PyType_Check(obj.ptr()))
      {
        v8::Handle<v8::String> cls_name = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), py::extract<const char *>(obj.attr("__name__"))()).ToLocalChecked();

        func_tmpl->SetClassName(cls_name);
      }

      result = func_tmpl->GetFunction(v8::Isolate::GetCurrent()->GetCurrentContext()).ToLocalChecked();
      
      /* TODO port me
      #ifdef SUPPORT_TRACE_LIFECYCLE
      if (!result.IsEmpty()) ObjectTracer::Trace(result, object);
      #endif
      */
    }
    else
    {
      static v8::Persistent<v8::ObjectTemplate> s_template(v8::Isolate::GetCurrent(), CreateObjectTemplate(v8::Isolate::GetCurrent()));

      v8::MaybeLocal<v8::Object> instance = v8::Local<v8::ObjectTemplate>::New(
          v8::Isolate::GetCurrent(), s_template)->NewInstance(
          v8::Isolate::GetCurrent()->GetCurrentContext());

      if (!instance.IsEmpty())
      {
        py::object *object = new py::object(obj);

        v8::Handle<v8::Object> realInstance = instance.ToLocalChecked();
        realInstance->SetInternalField(0, v8::External::New(v8::Isolate::GetCurrent(), object));

        //TODO port me
        //#ifdef SUPPORT_TRACE_LIFECYCLE
        //  ObjectTracer::Trace(instance, object);
        //#endif
        result = realInstance;
//TODO add test
//std::cout << "Is is wrapped? " << CPythonObject::IsWrapped(realInstance) << std::endl;
//py::object un = CPythonObject::Unwrap(realInstance);
//std::cout << "Unwrapped python object is this many bytes: " << sizeof(un) << std::endl;
      }

    }

  if (result.IsEmpty()) CJavascriptException::ThrowIf(v8::Isolate::GetCurrent(), try_catch);

  return handle_scope.Escape(result);
}

// =====================================================================
// CPythonObject End 
// =====================================================================

// =====================================================================
// CJavaScriptObject Start - UNTESTED 
// =====================================================================

void CJavascriptObject::CheckAttr(v8::Handle<v8::String> name) const
{
  assert(v8::Isolate::GetCurrent()->InContext());

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  if (!Object()->Has(context, name).ToChecked())
  {
    std::ostringstream msg;

    msg << "'" << *v8::String::Utf8Value(isolate, Object()->ObjectProtoToString(context).ToLocalChecked())
        << "' object has no attribute '" 
        << *v8::String::Utf8Value(isolate, name) << "'";

    throw CJavascriptException(msg.str(), ::PyExc_AttributeError);
  }
}

py::object CJavascriptObject::GetAttr(const std::string& name)
{
/* TODO port me
#ifdef SUPPORT_PROBES
  if (WRAPPER_JS_OBJECT_GETATTR_ENABLED()) {
    WRAPPER_JS_OBJECT_GETATTR(&m_obj, name.c_str());
  }
#endif
*/

  // CHECK_V8_CONTEXT();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  v8::Handle<v8::String> attr_name = DecodeUtf8(name);

  CheckAttr(attr_name);

  v8::Handle<v8::Value> attr_value = Object()->Get(context, attr_name).ToLocalChecked();

  if (attr_value.IsEmpty())
    CJavascriptException::ThrowIf(v8::Isolate::GetCurrent(), try_catch);

  return CJavascriptObject::Wrap(attr_value, Object());
}

void CJavascriptObject::SetAttr(const std::string& name, py::object value)
{
/* TODO port me
#ifdef SUPPORT_PROBES
  if (WRAPPER_JS_OBJECT_SETATTR_ENABLED()) {
    WRAPPER_JS_OBJECT_SETATTR(&m_obj, name.c_str(), value.ptr());
  }
#endif
*/

  // CHECK_V8_CONTEXT();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  v8::Handle<v8::String> attr_name = DecodeUtf8(name);
  v8::Handle<v8::Value> attr_obj = CPythonObject::Wrap(value);

  if (Object()->Has(context, attr_name).ToChecked())
  {
    v8::Handle<v8::Value> UNUSED_VAR(attr_value) = Object()->Get(context, attr_name).ToLocalChecked();
  }

  if (!Object()->Set(context, attr_name, attr_obj).ToChecked())
    CJavascriptException::ThrowIf(v8::Isolate::GetCurrent(), try_catch);
}

void CJavascriptObject::DelAttr(const std::string& name)
{
/* TODO port me
#ifdef SUPPORT_PROBES
  if (WRAPPER_JS_OBJECT_DELATTR_ENABLED()) {
    WRAPPER_JS_OBJECT_DELATTR(&m_obj, name.c_str());
  }
#endif
*/

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  v8::Handle<v8::String> attr_name = DecodeUtf8(name);

  CheckAttr(attr_name);

  if (!Object()->Delete(context, attr_name).ToChecked())
    CJavascriptException::ThrowIf(v8::Isolate::GetCurrent(), try_catch);
}

py::list CJavascriptObject::GetAttrList(void)
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  CPythonGIL python_gil;

  py::list attrs;

  TERMINATE_EXECUTION_CHECK(attrs);

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  v8::Handle<v8::Array> props = Object()->GetPropertyNames(context).ToLocalChecked();

  for (size_t i=0; i<props->Length(); i++)
  {
    attrs.append(CJavascriptObject::Wrap(props->Get(context, v8::Integer::New(v8::Isolate::GetCurrent(), i)).ToLocalChecked()));
  }

  if (try_catch.HasCaught()) CJavascriptException::ThrowIf(v8::Isolate::GetCurrent(), try_catch);

  return attrs;
}

int CJavascriptObject::GetIdentityHash(void)
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  return Object()->GetIdentityHash();
}

CJavascriptObjectPtr CJavascriptObject::Clone(void)
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  return CJavascriptObjectPtr(new CJavascriptObject(Object()->Clone()));
}

bool CJavascriptObject::Contains(const std::string& name)
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  bool found = Object()->Has(context, DecodeUtf8(name)).ToChecked();

  if (try_catch.HasCaught()) CJavascriptException::ThrowIf(v8::Isolate::GetCurrent(), try_catch);

  return found;
}

bool CJavascriptObject::Equals(CJavascriptObjectPtr other) const
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  return other.get() && Object()->Equals(context, other->Object()).ToChecked();
}

void CJavascriptObject::Dump(std::ostream& os) const
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  if (m_obj.IsEmpty())
    os << "None";
  else if (Object()->IsInt32())
    os << Object()->Int32Value(context).ToChecked();
  else if (Object()->IsNumber())
    os << Object()->NumberValue(context).ToChecked();
  else if (Object()->IsBoolean())
    os << Object()->BooleanValue(isolate);
  else if (Object()->IsNull())
    os << "None";
  else if (Object()->IsUndefined())
    os << "N/A";
  else if (Object()->IsString())
    os << *v8::String::Utf8Value(isolate, v8::Handle<v8::String>::Cast(Object()));
  else
  {
    /*
    v8::Handle<v8::String> s = Object()->ToString(context).ToLocalChecked();

    if (s.IsEmpty())
      s = Object()->ObjectProtoToString(context).ToLocalChecked();

    if (!s.IsEmpty())
      os << *v8::String::Utf8Value(isolate, s);
    */

	v8::MaybeLocal<v8::String> s = Object()->ToString(context);
	if(s.IsEmpty())
		s = Object()->ObjectProtoToString(context);

	if(!s.IsEmpty())
		os << *v8::String::Utf8Value(isolate, s.ToLocalChecked());
  }
}

CJavascriptObject::operator long() const
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  if (m_obj.IsEmpty())
    throw CJavascriptException("argument must be a string or a number, not 'NoneType'", ::PyExc_TypeError);

  return Object()->Int32Value(context).ToChecked();
}

CJavascriptObject::operator double() const
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  if (m_obj.IsEmpty())
    throw CJavascriptException("argument must be a string or a number, not 'NoneType'", ::PyExc_TypeError);

  return Object()->NumberValue(context).ToChecked();
}

CJavascriptObject::operator bool() const
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  if (m_obj.IsEmpty()) return false;

  return Object()->BooleanValue(isolate);
}

// =====================================================================
// CJavascriptObject -- TESTED FUNCTIONS begin
// =====================================================================

py::object CJavascriptObject::Wrap(v8::Handle<v8::Value> value, v8::Handle<v8::Object> self)
{
  assert(v8::Isolate::GetCurrent()->InContext());

  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  if (value.IsEmpty() || value->IsNull() || value->IsUndefined()) 
  {
    return py::object();
  }
  if (value->IsTrue())
  {
    return py::object(py::handle<>(py::borrowed(Py_True)));
  }
  if (value->IsFalse())
  {
    return py::object(py::handle<>(py::borrowed(Py_False)));
  }
  if (value->IsInt32())
  { 
    return py::object(value->Int32Value(v8::Isolate::GetCurrent()->GetCurrentContext()).ToChecked());
  }
  if (value->IsString())
  {
    v8::String::Utf8Value str(v8::Isolate::GetCurrent(), v8::Handle<v8::String>::Cast(value));

    return py::str(*str, str.length());
  }
  if (value->IsStringObject())
  {
    v8::String::Utf8Value str(v8::Isolate::GetCurrent(), value.As<v8::StringObject>()->ValueOf());

    return py::str(*str, str.length());
  }
  if (value->IsBoolean())
  {
    return py::object(py::handle<>(py::borrowed(value->BooleanValue(v8::Isolate::GetCurrent()) ? Py_True : Py_False)));
  }
  if (value->IsBooleanObject())
  {
    return py::object(py::handle<>(py::borrowed(value.As<v8::BooleanObject>()->BooleanValue(v8::Isolate::GetCurrent()) ? Py_True : Py_False)));
  }
  if (value->IsNumber())
  {
    return py::object(py::handle<>(::PyFloat_FromDouble(value->NumberValue(v8::Isolate::GetCurrent()->GetCurrentContext()).ToChecked())));
  }
  if (value->IsNumberObject())
  {
    return py::object(py::handle<>(::PyFloat_FromDouble(value.As<v8::NumberObject>()->NumberValue(v8::Isolate::GetCurrent()->GetCurrentContext()).ToChecked())));
  }
  if (value->IsDate())
  {
    double n = v8::Handle<v8::Date>::Cast(value)->NumberValue(v8::Isolate::GetCurrent()->GetCurrentContext()).ToChecked();

    time_t ts = (time_t) floor(n / 1000);

    tm *t = localtime(&ts);

    return py::object(py::handle<>(::PyDateTime_FromDateAndTime(
      t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec,
      ((long long) floor(n)) % 1000 * 1000)));
  }

  return Wrap(value->ToObject(v8::Isolate::GetCurrent()->GetCurrentContext()).ToLocalChecked(), self);
}

py::object CJavascriptObject::Wrap(v8::Handle<v8::Object> obj, v8::Handle<v8::Object> self)
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  if (obj.IsEmpty())
  {
    return py::object();
  }
  else if (obj->IsArray())
  {
    v8::Handle<v8::Array> array = v8::Handle<v8::Array>::Cast(obj);

    return Wrap(new CJavascriptArray(array));
  }
  else if (CPythonObject::IsWrapped(obj))
  {
    return CPythonObject::Unwrap(obj);
  }
  else if (obj->IsFunction())
  {
    return Wrap(new CJavascriptFunction(self, v8::Handle<v8::Function>::Cast(obj)));
  }

  return Wrap(new CJavascriptObject(obj));
}

py::object CJavascriptObject::Wrap(CJavascriptObject *obj)
{
  CPythonGIL python_gil;

  TERMINATE_EXECUTION_CHECK(py::object())

  return py::object(py::handle<>(boost::python::converter::shared_ptr_to_python<CJavascriptObject>(CJavascriptObjectPtr(obj))));
}

// =====================================================================
// CJavascriptObject Untested
// =====================================================================

void CJavascriptArray::LazyConstructor(void)
{
  if (!m_obj.IsEmpty()) return;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::Handle<v8::Array> array;

  if (m_items.is_none())
  {
    array = v8::Array::New(isolate, m_size);
  }
#if PY_MAJOR_VERSION < 3
  else if (PyInt_CheckExact(m_items.ptr()))
  {
    m_size = PyInt_AS_LONG(m_items.ptr());
    array = v8::Array::New(isolate, m_size);
  }
#endif
  else if (PyLong_CheckExact(m_items.ptr()))
  {
    m_size = PyLong_AsLong(m_items.ptr());
    array = v8::Array::New(isolate, m_size);
  }
  else if (PyList_Check(m_items.ptr()))
  {
    m_size = PyList_GET_SIZE(m_items.ptr());
    array = v8::Array::New(isolate, m_size);

    for (Py_ssize_t i=0; i< (Py_ssize_t) m_size; i++)
    {
      array->Set(context, v8::Uint32::New(isolate, i), CPythonObject::Wrap(py::object(py::handle<>(py::borrowed(PyList_GET_ITEM(m_items.ptr(), i))))));
    }
  }
  else if (PyTuple_Check(m_items.ptr()))
  {
    m_size = PyTuple_GET_SIZE(m_items.ptr());
    array = v8::Array::New(isolate, m_size);

    for (Py_ssize_t i=0; i< (Py_ssize_t) m_size; i++)
    {
      array->Set(context, v8::Uint32::New(isolate, i), CPythonObject::Wrap(py::object(py::handle<>(py::borrowed(PyTuple_GET_ITEM(m_items.ptr(), i))))));
    }
  }
  else if (PyGen_Check(m_items.ptr()))
  {
    array = v8::Array::New(isolate);

    py::object iter(py::handle<>(::PyObject_GetIter(m_items.ptr())));

    m_size = 0;
    PyObject *item = NULL;

    while (NULL != (item = ::PyIter_Next(iter.ptr())))
    {
      array->Set(context, v8::Uint32::New(isolate, m_size++), CPythonObject::Wrap(py::object(py::handle<>(py::borrowed(item)))));
    }
  }

  m_obj.Reset(isolate, array);
}
// =====================================================================
// CJavascriptObject End 
// =====================================================================


// =====================================================================
// CJavascriptArray Begin untested 
// =====================================================================

size_t CJavascriptArray::Length(void)
{
  LazyConstructor();

  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  return v8::Handle<v8::Array>::Cast(Object())->Length();
}

py::object CJavascriptArray::GetItem(py::object key)
{
#ifdef SUPPORT_PROBES
  if (WRAPPER_JS_ARRAY_GETITEM_ENABLED()) {
    WRAPPER_JS_ARRAY_GETITEM(&m_obj, key.ptr());
  }
#endif

  LazyConstructor();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  if (PySlice_Check(key.ptr()))
  {
    Py_ssize_t arrayLen = v8::Handle<v8::Array>::Cast(Object())->Length();
    Py_ssize_t start, stop, step, sliceLen;

    if (0 == PySlice_GetIndicesEx(PySlice_Cast(key.ptr()), arrayLen, &start, &stop, &step, &sliceLen))
    {
      py::list slice;

      for (Py_ssize_t idx=start; idx<stop; idx+=step)
      {
        v8::Handle<v8::Value> value = Object()->Get(context, v8::Integer::New(isolate, (uint32_t) idx)).ToLocalChecked();

        if (value.IsEmpty()) CJavascriptException::ThrowIf(isolate, try_catch);

        slice.append(CJavascriptObject::Wrap(value, Object()));
      }

      return slice;
    }
  }
  else if (PyInt_Check(key.ptr()) || PyLong_Check(key.ptr()))
  {
    uint32_t idx = PyInt_Check(key.ptr()) ? (uint32_t) ::PyInt_AsUnsignedLongMask(key.ptr()) : (uint32_t) ::PyLong_AsUnsignedLong(key.ptr());

    if (!Object()->Has(context,idx).ToChecked()) return py::object();

    v8::Handle<v8::Value> value = Object()->Get(context, v8::Integer::New(isolate, idx)).ToLocalChecked();

    if (value.IsEmpty()) CJavascriptException::ThrowIf(isolate, try_catch);

    return CJavascriptObject::Wrap(value, Object());
  }

  throw CJavascriptException("list indices must be integers", ::PyExc_TypeError);
}

py::object CJavascriptArray::SetItem(py::object key, py::object value)
{
/* TODO port me
#ifdef SUPPORT_PROBES
  if (WRAPPER_JS_ARRAY_SETITEM_ENABLED()) {
    WRAPPER_JS_ARRAY_SETITEM(&m_obj, key.ptr(), value.ptr());
  }
#endif
*/
  LazyConstructor();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  if (PySlice_Check(key.ptr()))
  {
    PyObject *values = ::PySequence_Fast(value.ptr(), "can only assign an iterable");

    if (values)
    {
      Py_ssize_t itemSize = PySequence_Fast_GET_SIZE(value.ptr());
      PyObject **items = PySequence_Fast_ITEMS(value.ptr());

      Py_ssize_t arrayLen = v8::Handle<v8::Array>::Cast(Object())->Length();
      Py_ssize_t start, stop, step, sliceLen;

      if (0 == PySlice_GetIndicesEx(PySlice_Cast(key.ptr()), arrayLen, &start, &stop, &step, &sliceLen))
      {
        /* TODO port me BUGBUG
         * This will require some work since we are trying to avoid the
         * internal interface.  But, it may be needed.
        if (itemSize != sliceLen)
        {
          v8i::Handle<v8i::JSArray> array = v8::Utils::OpenHandle(*Object());

          array->set_length(v8i::Smi::FromInt(arrayLen - sliceLen + itemSize));

          if (itemSize < sliceLen)
          {
            Py_ssize_t diff = sliceLen - itemSize;

            for (Py_ssize_t idx=start+itemSize; idx<arrayLen-diff; idx++)
            {
              Object()->Set(idx, Object()->Get((uint32_t) (idx + diff)));
            }
            for (Py_ssize_t idx=arrayLen-1; idx >arrayLen-diff-1; idx--)
            {
              Object()->Delete((uint32_t) idx);
            }
          }
          else if (itemSize > sliceLen)
          {
            Py_ssize_t diff = itemSize - sliceLen;

            for (Py_ssize_t idx=arrayLen+diff-1; idx>stop-1; idx--)
            {
              Object()->Set(idx, Object()->Get((uint32_t) (idx - diff)));
            }
          }
        }
        */

        for (Py_ssize_t idx=0; idx<itemSize; idx++)
        {
          Object()->Set(context, (uint32_t) (start + idx), CPythonObject::Wrap(py::object(py::handle<>(py::borrowed(items[idx])))));
        }
      }
    }
  }
  else if (PyInt_Check(key.ptr()) || PyLong_Check(key.ptr()))
  {
    uint32_t idx = PyInt_Check(key.ptr()) ? (uint32_t) ::PyInt_AsUnsignedLongMask(key.ptr()) : (uint32_t) ::PyLong_AsUnsignedLong(key.ptr());

    if (!Object()->Set(context, v8::Integer::New(v8::Isolate::GetCurrent(), idx), CPythonObject::Wrap(value)).ToChecked())
      CJavascriptException::ThrowIf(v8::Isolate::GetCurrent(), try_catch);
  }

  return value;
}

py::object CJavascriptArray::DelItem(py::object key)
{
/* TODO port me
#ifdef SUPPORT_PROBES
  if (WRAPPER_JS_ARRAY_DELITEM_ENABLED()) {
    WRAPPER_JS_ARRAY_DELITEM(&m_obj, key.ptr());
  }
#endif
*/
  LazyConstructor();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  if (PySlice_Check(key.ptr()))
  {
    Py_ssize_t arrayLen = v8::Handle<v8::Array>::Cast(Object())->Length();
    Py_ssize_t start, stop, step, sliceLen;

    if (0 == PySlice_GetIndicesEx(PySlice_Cast(key.ptr()), arrayLen, &start, &stop, &step, &sliceLen))
    {
      for (Py_ssize_t idx=stop; idx<arrayLen; idx++)
      {
        Object()->Set(context, (uint32_t) (idx - sliceLen), Object()->Get(context, (uint32_t) idx).ToLocalChecked());
      }

      for (Py_ssize_t idx=arrayLen-1; idx>arrayLen-sliceLen-1; idx--)
      {
        Object()->Delete(context, (uint32_t)idx);
      }

      /* TODO port me BUGBUG
       * Look into if we have to use the private api
      v8i::Handle<v8i::JSArray> array = v8::Utils::OpenHandle(*Object());

      array->set_length(v8i::Smi::FromInt(arrayLen - sliceLen));
      */
    }

    return py::object();
  }
  else if (PyInt_Check(key.ptr()) || PyLong_Check(key.ptr()))
  {
    uint32_t idx = PyInt_Check(key.ptr()) ? (uint32_t) ::PyInt_AsUnsignedLongMask(key.ptr()) : (uint32_t) ::PyLong_AsUnsignedLong(key.ptr());

    py::object value;

    if (Object()->Has(context, idx).ToChecked())
      value = CJavascriptObject::Wrap(Object()->Get(context, v8::Integer::New(v8::Isolate::GetCurrent(), idx)).ToLocalChecked(), Object());

    if (!Object()->Delete(context, idx).ToChecked())
      CJavascriptException::ThrowIf(v8::Isolate::GetCurrent(), try_catch);

    return value;
  }

  throw CJavascriptException("list indices must be integers", ::PyExc_TypeError);
}

bool CJavascriptArray::Contains(py::object item)
{
  LazyConstructor();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  for (size_t i=0; i<Length(); i++)
  {
    if (Object()->Has(context, i).ToChecked())
    {
      v8::Handle<v8::Value> value = Object()->Get(context, v8::Integer::New(isolate, i)).ToLocalChecked();

      if (try_catch.HasCaught()) CJavascriptException::ThrowIf(v8::Isolate::GetCurrent(), try_catch);

      if (item == CJavascriptObject::Wrap(value, Object()))
      {
        return true;
      }
    }
  }

  if (try_catch.HasCaught()) CJavascriptException::ThrowIf(v8::Isolate::GetCurrent(), try_catch);

  return false;
}

py::object CJavascriptFunction::CallWithArgs(py::tuple args, py::dict kwds)
{
  size_t argc = ::PyTuple_Size(args.ptr());

  if (argc == 0) throw CJavascriptException("missed self argument", ::PyExc_TypeError);

  py::object self = args[0];
  py::extract<CJavascriptFunction&> extractor(self);

  if (!extractor.check()) throw CJavascriptException("missed self argument", ::PyExc_TypeError);

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  CHECK_V8_CONTEXT();

  v8::TryCatch try_catch(isolate);

  CJavascriptFunction& func = extractor();
  py::list argv(args.slice(1, py::_));

  return func.Call(func.Self(), argv, kwds);
}

py::object CJavascriptFunction::Call(v8::Handle<v8::Object> self, py::list args, py::dict kwds)
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(Object());

  size_t args_count = ::PyList_Size(args.ptr()), kwds_count = ::PyMapping_Size(kwds.ptr());

  std::vector< v8::Handle<v8::Value> > params(args_count + kwds_count);

  for (size_t i=0; i<args_count; i++)
  {
    params[i] = CPythonObject::Wrap(args[i]);
  }

  py::list values = kwds.values();

  for (size_t i=0; i<kwds_count; i++)
  {
    params[args_count+i] = CPythonObject::Wrap(values[i]);
  }

  v8::Handle<v8::Value> result;

  Py_BEGIN_ALLOW_THREADS

  result = func->Call(context, 
    self.IsEmpty() ? v8::Isolate::GetCurrent()->GetCurrentContext()->Global() : self,
    params.size(), params.empty() ? NULL : &params[0]).ToLocalChecked();

  Py_END_ALLOW_THREADS

  if (result.IsEmpty()) CJavascriptException::ThrowIf(v8::Isolate::GetCurrent(), try_catch);

  return CJavascriptObject::Wrap(result);
}

py::object CJavascriptFunction::CreateWithArgs(CJavascriptFunctionPtr proto, py::tuple args, py::dict kwds)
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  if (proto->Object().IsEmpty())
    throw CJavascriptException("Object prototype may only be an Object", ::PyExc_TypeError);

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(proto->Object());

  size_t args_count = ::PyTuple_Size(args.ptr());

  std::vector< v8::Handle<v8::Value> > params(args_count);

  for (size_t i=0; i<args_count; i++)
  {
    params[i] = CPythonObject::Wrap(args[i]);
  }

  v8::Handle<v8::Object> result;

  Py_BEGIN_ALLOW_THREADS

  result = func->NewInstance(context, params.size(), params.empty() ? NULL : &params[0]).ToLocalChecked();

  Py_END_ALLOW_THREADS

  if (result.IsEmpty()) CJavascriptException::ThrowIf(v8::Isolate::GetCurrent(), try_catch);

  size_t kwds_count = ::PyMapping_Size(kwds.ptr());
  py::list items = kwds.items();

  for (size_t i=0; i<kwds_count; i++)
  {
    py::tuple item(items[i]);

    py::str key(item[0]);
    py::object value = item[1];

    result->Set(context, ToString(key), CPythonObject::Wrap(value));
  }

  return CJavascriptObject::Wrap(result);
}

py::object CJavascriptFunction::ApplyJavascript(CJavascriptObjectPtr self, py::list args, py::dict kwds)
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  return Call(self->Object(), args, kwds);
}

py::object CJavascriptFunction::ApplyPython(py::object self, py::list args, py::dict kwds)
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  return Call(CPythonObject::Wrap(self)->ToObject(context).ToLocalChecked(), args, kwds);
}

py::object CJavascriptFunction::Invoke(py::list args, py::dict kwds)
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  return Call(Self(), args, kwds);
}

const std::string CJavascriptFunction::GetName(void) const
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(Object());

  v8::String::Utf8Value name(isolate, v8::Handle<v8::String>::Cast(func->GetName()));

  return std::string(*name, name.length());
}

void CJavascriptFunction::SetName(const std::string& name)
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(Object());

  func->SetName(v8::String::NewFromUtf8(isolate, name.c_str(), v8::NewStringType::kNormal, name.size()).ToLocalChecked());
}

int CJavascriptFunction::GetLineNumber(void) const
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(Object());

  return func->GetScriptLineNumber();
}

int CJavascriptFunction::GetColumnNumber(void) const
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(Object());

  return func->GetScriptColumnNumber();
}

const std::string CJavascriptFunction::GetResourceName(void) const
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  CHECK_V8_CONTEXT();

  v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(Object());

  v8::String::Utf8Value name(isolate, v8::Handle<v8::String>::Cast(func->GetScriptOrigin().ResourceName()));

  return std::string(*name, name.length());
}

const std::string CJavascriptFunction::GetInferredName(void) const
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  CHECK_V8_CONTEXT();

  v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(Object());

  v8::String::Utf8Value name(isolate, v8::Handle<v8::String>::Cast(func->GetInferredName()));

  return std::string(*name, name.length());
}

int CJavascriptFunction::GetLineOffset(void) const
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(Object());

  return func->GetScriptOrigin().ResourceLineOffset()->Value();
}

int CJavascriptFunction::GetColumnOffset(void) const
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(Object());

  return func->GetScriptOrigin().ResourceColumnOffset()->Value();
}

py::object CJavascriptFunction::GetOwner(void) const
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  return CJavascriptObject::Wrap(Self());
}
