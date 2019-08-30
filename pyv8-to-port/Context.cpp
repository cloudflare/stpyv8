#include "Context.h"

#include "Wrapper.h"
#include "Engine.h"

void CContext::Expose(void)
{
  py::class_<CIsolate, boost::noncopyable>("JSIsolate", "JSIsolate is an isolated instance of the V8 engine.", py::no_init)
    .def(py::init<bool>((py::arg("owner") = false)))

    .add_static_property("current", &CIsolate::GetCurrent,
                         "Returns the entered isolate for the current thread or NULL in case there is no current isolate.")

    .add_property("locked", &CIsolate::IsLocked)

    .def("GetCurrentStackTrace", &CIsolate::GetCurrentStackTrace)

    .def("enter", &CIsolate::Enter,
         "Sets this isolate as the entered one for the current thread. "
         "Saves the previously entered one (if any), so that it can be "
         "restored when exiting.  Re-entering an isolate is allowed.")

    .def("leave", &CIsolate::Leave,
         "Exits this isolate by restoring the previously entered one in the current thread. "
         "The isolate may still stay the same, if it was entered more than once.")
    ;

  py::class_<CContext, boost::noncopyable>("JSContext", "JSContext is an execution context.", py::no_init)
    .def(py::init<const CContext&>("create a new context base on a exists context"))
    .def(py::init<py::object, py::list>((py::arg("global") = py::object(),
                                         py::arg("extensions") = py::list()),
                                        "create a new context base on global object"))

    .add_property("securityToken", &CContext::GetSecurityToken, &CContext::SetSecurityToken)

    .add_property("locals", &CContext::GetGlobal, "Local variables within context")

    .add_static_property("entered", &CContext::GetEntered,
                         "The last entered context.")
    .add_static_property("current", &CContext::GetCurrent,
                         "The context that is on the top of the stack.")
    .add_static_property("calling", &CContext::GetCalling,
                         "The context of the calling JavaScript code.")
    .add_static_property("inContext", &CContext::InContext,
                         "Returns true if V8 has a current context.")

    .add_property("hasOutOfMemoryException", &CContext::HasOutOfMemoryException)

    .def("eval", &CContext::Evaluate, (py::arg("source"),
                                       py::arg("name") = std::string(),
                                       py::arg("line") = -1,
                                       py::arg("col") = -1,
                                       py::arg("precompiled") = py::object()))
    .def("eval", &CContext::EvaluateW, (py::arg("source"),
                                        py::arg("name") = std::wstring(),
                                        py::arg("line") = -1,
                                        py::arg("col") = -1,
                                        py::arg("precompiled") = py::object()))

    .def("enter", &CContext::Enter, "Enter this context. "
         "After entering a context, all code compiled and "
         "run is compiled and run in this context.")
    .def("leave", &CContext::Leave, "Exit this context. "
         "Exiting the current context restores the context "
         "that was in place when entering the current context.")

    .def("__nonzero__", &CContext::IsEntered, "the context has been entered.")
    ;

  py::objects::class_value_wrapper<boost::shared_ptr<CIsolate>,
    py::objects::make_ptr_instance<CIsolate,
    py::objects::pointer_holder<boost::shared_ptr<CIsolate>,CIsolate> > >();

  py::objects::class_value_wrapper<boost::shared_ptr<CContext>,
    py::objects::make_ptr_instance<CContext,
    py::objects::pointer_holder<boost::shared_ptr<CContext>,CContext> > >();
}

py::object CIsolate::GetCurrent(void)
{
  v8::Isolate *isolate = v8::Isolate::GetCurrent();

  v8::HandleScope handle_scope(isolate);

  return !isolate ? py::object() :
    py::object(py::handle<>(boost::python::converter::shared_ptr_to_python<CIsolate>(
    CIsolatePtr(new CIsolate(isolate)))));
}

CContext::CContext(v8::Handle<v8::Context> context)
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  m_context.Reset(context->GetIsolate(), context);
}

CContext::CContext(const CContext& context)
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  m_context.Reset(context.Handle()->GetIsolate(), context.Handle());
}

CContext::CContext(py::object global, py::list extensions)
  : m_global(global)
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  std::auto_ptr<v8::ExtensionConfiguration> cfg;
  std::vector<std::string> ext_names;
  std::vector<const char *> ext_ptrs;

  for (Py_ssize_t i=0; i<PyList_Size(extensions.ptr()); i++)
  {
    py::extract<const std::string> extractor(::PyList_GetItem(extensions.ptr(), i));

    if (extractor.check())
    {
      ext_names.push_back(extractor());
    }
  }

  for (size_t i=0; i<ext_names.size(); i++)
  {
    ext_ptrs.push_back(ext_names[i].c_str());
  }

  if (!ext_ptrs.empty()) cfg.reset(new v8::ExtensionConfiguration(ext_ptrs.size(), &ext_ptrs[0]));

  v8::Handle<v8::Context> context = v8::Context::New(v8::Isolate::GetCurrent(), cfg.get());

  m_context.Reset(v8::Isolate::GetCurrent(), context);

  v8::Context::Scope context_scope(Handle());

  if (!global.is_none())
  {
    Handle()->Global()->Set(v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), "__proto__"), CPythonObject::Wrap(global));

    Py_DECREF(global.ptr());
  }
}

py::object CContext::GetGlobal(void)
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  return CJavascriptObject::Wrap(Handle()->Global());
}

py::str CContext::GetSecurityToken(void)
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  v8::Handle<v8::Value> token = Handle()->GetSecurityToken();

  if (token.IsEmpty()) return py::str();

  v8::String::Utf8Value str(token->ToString());

  return py::str(*str, str.length());
}

void CContext::SetSecurityToken(py::str token)
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  if (token.is_none())
  {
    Handle()->UseDefaultSecurityToken();
  }
  else
  {
    Handle()->SetSecurityToken(v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), py::extract<const char *>(token)()));
  }
}

py::object CContext::GetEntered(void)
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  v8::Handle<v8::Context> entered = v8::Isolate::GetCurrent()->GetEnteredContext();

  return (!v8::Isolate::GetCurrent()->InContext() || entered.IsEmpty()) ? py::object() :
    py::object(py::handle<>(boost::python::converter::shared_ptr_to_python<CContext>(CContextPtr(new CContext(entered)))));
}

py::object CContext::GetCurrent(void)
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  v8::Handle<v8::Context> current = v8::Isolate::GetCurrent()->GetCurrentContext();

  return (current.IsEmpty()) ? py::object() :
    py::object(py::handle<>(boost::python::converter::shared_ptr_to_python<CContext>(CContextPtr(new CContext(current)))));
}

py::object CContext::GetCalling(void)
{
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  v8::Handle<v8::Context> calling = v8::Isolate::GetCurrent()->GetCallingContext();

  return (!v8::Isolate::GetCurrent()->InContext() || calling.IsEmpty()) ? py::object() :
    py::object(py::handle<>(boost::python::converter::shared_ptr_to_python<CContext>(CContextPtr(new CContext(calling)))));
}

py::object CContext::Evaluate(const std::string& src,
                              const std::string name,
                              int line, int col,
                              py::object precompiled)
{
  CEngine engine(v8::Isolate::GetCurrent());

  CScriptPtr script = engine.Compile(src, name, line, col, precompiled);

  return script->Run();
}

py::object CContext::EvaluateW(const std::wstring& src,
                               const std::wstring name,
                               int line, int col,
                               py::object precompiled)
{
  CEngine engine(v8::Isolate::GetCurrent());

  CScriptPtr script = engine.CompileW(src, name, line, col, precompiled);

  return script->Run();
}
