#include "Context.h"

#include "Wrapper.h"
#include "Engine.h"

#include "libplatform/libplatform.h"

void CIsolate::Init(bool owner)
{
  m_owner = owner;

  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  m_isolate = v8::Isolate::New(create_params);
}

CIsolate::CIsolate(bool owner)
{
  CIsolate::Init(owner);
}

CIsolate::CIsolate()
{
  CIsolate::Init(false);
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

py::object CIsolate::GetCurrent(void)
{
  v8::Isolate *isolate = v8::Isolate::GetCurrent();
  if(isolate == nullptr || (!isolate->IsInUse()))
  {
    return py::object();
  }

  v8::HandleScope handle_scope(isolate);

  return !isolate ? py::object() :
    py::object(py::handle<>(boost::python::converter::shared_ptr_to_python<CIsolate>(
    CIsolatePtr(new CIsolate(isolate)))));
}
