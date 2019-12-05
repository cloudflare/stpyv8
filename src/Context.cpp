#include "Context.h"
#include "Wrapper.h"
#include "Engine.h"

#include "libplatform/libplatform.h"

void CContext::Expose(void)
{
  py::class_<CPlatform, boost::noncopyable>("JSPlatform", "JSPlatform allows the V8 platform to be initialized", py::no_init)
    .def(py::init<std::string>((py::arg("argv") = std::string())))
    .def("init", &CPlatform::Init, "Initializes the platform")
    ;

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
    .def(py::init<const CContext&>("Create a new context based on a existing context"))
    .def(py::init<py::object>((py::arg("global") = py::object(),
                               "Create a new context based on global object")))

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

    .def("eval", &CContext::Evaluate, (py::arg("source"),
                                       py::arg("name") = std::string(),
                                       py::arg("line") = -1,
                                       py::arg("col") = -1))
    .def("eval", &CContext::EvaluateW, (py::arg("source"),
                                        py::arg("name") = std::wstring(),
                                        py::arg("line") = -1,
                                        py::arg("col") = -1))

    .def("enter", &CContext::Enter, "Enter this context. "
         "After entering a context, all code compiled and "
         "run is compiled and run in this context.")
    .def("leave", &CContext::Leave, "Exit this context. "
         "Exiting the current context restores the context "
         "that was in place when entering the current context.")

    .def("__bool__", &CContext::IsEntered, "the context has been entered.")
    ;

  py::objects::class_value_wrapper<std::shared_ptr<CPlatform>,
    py::objects::make_ptr_instance<CPlatform,
    py::objects::pointer_holder<std::shared_ptr<CPlatform>,CPlatform> > >();

  py::objects::class_value_wrapper<std::shared_ptr<CIsolate>,
    py::objects::make_ptr_instance<CIsolate,
    py::objects::pointer_holder<std::shared_ptr<CIsolate>,CIsolate> > >();

  py::objects::class_value_wrapper<std::shared_ptr<CContext>,
    py::objects::make_ptr_instance<CContext,
    py::objects::pointer_holder<std::shared_ptr<CContext>,CContext> > >();
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

CContext::CContext(py::object global)
  : m_global(global)
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  v8::Handle<v8::Context> context = v8::Context::New(isolate);

  m_context.Reset(isolate, context);

  v8::Context::Scope context_scope(Handle());

  if (!global.is_none())
  {
    v8::Maybe<bool> retcode =
    Handle()->Global()->Set(context,
        v8::String::NewFromUtf8(isolate, "__proto__").ToLocalChecked(),
        CPythonObject::Wrap(global));
    if(retcode.IsNothing()) {
      //TODO we need to do something if the set call failed
    }

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
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  v8::Handle<v8::Value> token = Handle()->GetSecurityToken();

  if (token.IsEmpty()) return py::str();

  v8::String::Utf8Value str(isolate, token->ToString(m_context.Get(isolate)).ToLocalChecked());

  return py::str(*str, str.length());
}

void CContext::SetSecurityToken(py::str token)
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  if (token.is_none())
  {
    Handle()->UseDefaultSecurityToken();
  }
  else
  {
    Handle()->SetSecurityToken(v8::String::NewFromUtf8(isolate, 
          py::extract<const char *>(token)()).ToLocalChecked());
  }
}

py::object CContext::GetEntered(void)
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  v8::Handle<v8::Context> entered = isolate->GetEnteredContext();

  return (!isolate->InContext() || entered.IsEmpty()) ? py::object() :
    py::object(py::handle<>(boost::python::converter::shared_ptr_to_python<CContext>(CContextPtr(new CContext(entered)))));
}

py::object CContext::GetCurrent(void)
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  v8::Handle<v8::Context> current = isolate->GetCurrentContext();

  return (current.IsEmpty()) ? py::object() :
    py::object(py::handle<>(boost::python::converter::shared_ptr_to_python<CContext>(CContextPtr(new CContext(current)))));
}

py::object CContext::GetCalling(void)
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  v8::Handle<v8::Context> calling = isolate->GetCurrentContext();

  return (!isolate->InContext() || calling.IsEmpty()) ? py::object() :
    py::object(py::handle<>(boost::python::converter::shared_ptr_to_python<CContext>(CContextPtr(new CContext(calling)))));
}

py::object CContext::Evaluate(const std::string& src,
                              const std::string name,
                              int line, int col)
{
  CEngine engine(v8::Isolate::GetCurrent());

  CScriptPtr script = engine.Compile(src, name, line, col);

  return script->Run();
}

py::object CContext::EvaluateW(const std::wstring& src,
                               const std::wstring name,
                               int line, int col)
{
  CEngine engine(v8::Isolate::GetCurrent());

  CScriptPtr script = engine.CompileW(src, name, line, col);

  return script->Run();
}
