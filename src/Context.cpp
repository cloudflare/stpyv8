#include "Context.h"

#include "Wrapper.h"
#include "Engine.h"



CIsolate::CIsolate(bool owner=false) : m_owner(owner) 
{
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
    v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  m_isolate = v8::Isolate::New(create_params); 
}
CIsolate::CIsolate(v8::Isolate *isolate) : m_isolate(isolate), m_owner(false) 
{
}

CIsolate::~CIsolate(void) 
{ 
  if (m_owner) m_isolate->Dispose(); 
}

v8::Isolate *CIsolate::GetIsolate(void) 
{ 
  return m_isolate; 
}

CJavascriptStackTracePtr CIsolate::GetCurrentStackTrace(int frame_limit,
    v8::StackTrace::StackTraceOptions options = v8::StackTrace::kOverview) 
{
  return CJavascriptStackTrace::GetCurrentStackTrace(m_isolate, frame_limit, options);
}


void CContext::Expose(void)
{
//TODO port me
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

  std::shared_ptr<v8::ExtensionConfiguration> cfg;
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
    v8::Maybe<bool> retcode =
    Handle()->Global()->Set(context,
        v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), "__proto__").ToLocalChecked(), 
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

  //v8::Handle<v8::Context> calling = v8::Isolate::GetCurrent()->GetCallingContext();
  v8::Handle<v8::Context> calling = v8::Isolate::GetCurrent()->GetCurrentContext();

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

bool CContext::HasOutOfMemoryException(void) 
{ 
  //TODO port me.  No trace of HasOutOfMemoryException in the new V8 api
  //v8::HandleScope handle_scope(v8::Isolate::GetCurrent()); return Handle()->HasOutOfMemoryException(); 
  return false;
}

