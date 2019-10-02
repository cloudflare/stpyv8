#include "Engine.h"
#include "Exception.h"
#include "Wrapper.h"

#include <iostream>

#include <boost/preprocessor.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

#ifdef SUPPORT_SERIALIZE
  CEngine::CounterTable CEngine::m_counters;
#endif

void CEngine::Expose(void)
{
#ifndef SUPPORT_SERIALIZE
// TODO port me
//  v8::V8::SetFatalErrorHandler(ReportFatalError);
//  v8::V8::AddMessageListener(ReportMessage);
#endif
  /*
  py::enum_<v8::ObjectSpace>("JSObjectSpace")
    .value("New", v8::kObjectSpaceNewSpace)

    .value("OldPointer", v8::kObjectSpaceOldPointerSpace)
    .value("OldData", v8::kObjectSpaceOldDataSpace)
    .value("Code", v8::kObjectSpaceCodeSpace)
    .value("Map", v8::kObjectSpaceMapSpace)
    .value("Lo", v8::kObjectSpaceLoSpace)

    .value("All", v8::kObjectSpaceAll);

  py::enum_<v8::AllocationAction>("JSAllocationAction")
    .value("alloc", v8::kAllocationActionAllocate)
    .value("free", v8::kAllocationActionFree)
    .value("all", v8::kAllocationActionAll);

  py::enum_<v8i::LanguageMode>("JSLanguageMode")
    .value("CLASSIC", v8i::CLASSIC_MODE)
    .value("STRICT", v8i::STRICT_MODE)
    .value("EXTENDED", v8i::EXTENDED_MODE);
  */

  py::class_<CEngine, boost::noncopyable>("JSEngine", "JSEngine is a backend Javascript engine.")
    .def(py::init<>("Create a new script engine instance."))
    .add_static_property("version", &CEngine::GetVersion,
                         "Get the V8 engine version.")

    //.add_static_property("dead", &v8::V8::IsDead,
    //                     "Check if V8 is dead and therefore unusable.")

    .def("setFlags", &CEngine::SetFlags, "Sets V8 flags from a string.")
    .staticmethod("setFlags")

    .def("collect", &CEngine::CollectAllGarbage, (py::arg("force")=true),
         "Performs a full garbage collection. Force compaction if the parameter is true.")
    .staticmethod("collect")

    /*
  #ifdef SUPPORT_SERIALIZE
    .add_static_property("serializeEnabled", &CEngine::IsSerializeEnabled, &CEngine::SetSerializeEnable)

    .def("serialize", &CEngine::Serialize)
    .staticmethod("serialize")

    .def("deserialize", &CEngine::Deserialize)
    .staticmethod("deserialize")
  #endif
    */

    .def("terminateAllThreads", &CEngine::TerminateAllThreads,
         "Forcefully terminate the current thread of JavaScript execution.")
    .staticmethod("terminateAllThreads")

    .def("dispose", &v8::V8::Dispose,
         "Releases any resources used by v8 and stops any utility threads "
         "that may be running. Note that disposing v8 is permanent, "
         "it cannot be reinitialized.")
    .staticmethod("dispose")

    .def("lowMemory", &v8::Isolate::LowMemoryNotification,
         "Optional notification that the system is running low on memory.")
    .staticmethod("lowMemory")

	/*
    .def("setMemoryLimit", &CEngine::SetMemoryLimit, (py::arg("max_young_space_size") = 0,
                                                      py::arg("max_old_space_size") = 0,
                                                      py::arg("max_executable_size") = 0),
         "Specifies the limits of the runtime's memory use."
         "You must set the heap size before initializing the VM"
         "the size cannot be adjusted after the VM is initialized.")
    .staticmethod("setMemoryLimit")

	*/
    .def("setStackLimit", &CEngine::SetStackLimit, (py::arg("stack_limit_size") = 0),
         "Uses the address of a local variable to determine the stack top now."
         "Given a size, returns an address that is that far from the current top of stack.")
    .staticmethod("setStackLimit")

	/*
    .def("setMemoryAllocationCallback", &MemoryAllocationManager::SetCallback,
                                        (py::arg("callback"),
                                         py::arg("space") = v8::kObjectSpaceAll,
                                         py::arg("action") = v8::kAllocationActionAll),
                                        "Enables the host application to provide a mechanism to be notified "
                                        "and perform custom logging when V8 Allocates Executable Memory.")
    .staticmethod("setMemoryAllocationCallback")

    .def("precompile", &CEngine::PreCompile, (py::arg("source")))
    .def("precompile", &CEngine::PreCompileW, (py::arg("source")))
    */

    .def("compile", &CEngine::Compile, (py::arg("source"),
                                        py::arg("name") = std::string(),
                                        py::arg("line") = -1,
                                        py::arg("col") = -1,
                                        py::arg("precompiled") = py::object()))
    .def("compile", &CEngine::CompileW, (py::arg("source"),
                                         py::arg("name") = std::wstring(),
                                         py::arg("line") = -1,
                                         py::arg("col") = -1,
                                         py::arg("precompiled") = py::object()))
    ;

  py::class_<CScript, boost::noncopyable>("JSScript", "JSScript is a compiled JavaScript script.", py::no_init)
    .add_property("source", &CScript::GetSource, "the source code")

    .def("run", &CScript::Run, "Execute the compiled code.")

    /*
  */
    ;

  py::objects::class_value_wrapper<boost::shared_ptr<CScript>,
    py::objects::make_ptr_instance<CScript,
    py::objects::pointer_holder<boost::shared_ptr<CScript>, CScript> > >();

  /*
#ifdef SUPPORT_EXTENSION

  py::class_<CExtension, boost::noncopyable>("JSExtension", "JSExtension is a reusable script module.", py::no_init)
    .def(py::init<const std::string&, const std::string&, py::object, py::list, bool>((py::arg("name"),
                                                                                       py::arg("source"),
                                                                                       py::arg("callback") = py::object(),
                                                                                       py::arg("dependencies") = py::list(),
                                                                                       py::arg("register") = true)))
    .add_static_property("extensions", &CExtension::GetExtensions)

    .add_property("name", &CExtension::GetName, "The name of extension")
    .add_property("source", &CExtension::GetSource, "The source code of extension")
    .add_property("dependencies", &CExtension::GetDependencies, "The extension dependencies which will be load before this extension")

    .add_property("autoEnable", &CExtension::IsAutoEnable, &CExtension::SetAutoEnable, "Enable the extension by default.")

    .add_property("registered", &CExtension::IsRegistered, "The extension has been registerd")
    .def("register", &CExtension::Register, "Register the extension")
    ;

#endif
*/
}

//TODO - port serialization code


void CEngine::CollectAllGarbage(bool force_compaction)
{
  //NOT PORTED ON PURPOSE
  /*
  v8i::HandleScope handle_scope(v8i::Isolate::Current());

  if (force_compaction) {
    v8i::Isolate::Current()->heap()->CollectAllAvailableGarbage();
  } else {
    v8i::Isolate::Current()->heap()->CollectAllGarbage(v8i::Heap::kMakeHeapIterableMask);
  }
  */
}

void CEngine::TerminateAllThreads(void)
{
  v8::Isolate::GetCurrent()->TerminateExecution();
}

void CEngine::ReportFatalError(const char* location, const char* message)
{
  std::cerr << "<" << location << "> " << message << std::endl;
}

void CEngine::ReportMessage(v8::Handle<v8::Message> message, v8::Handle<v8::Value> data)
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::String::Utf8Value filename(isolate, message->GetScriptResourceName());
  int lineno = message->GetLineNumber(context).ToChecked();
  v8::String::Utf8Value sourceline(isolate, message->GetSourceLine(context).ToLocalChecked());

  std::cerr << *filename << ":" << lineno << " -> " << *sourceline << std::endl;
}

bool CEngine::SetMemoryLimit(int max_young_space_size, int max_old_space_size, int max_executable_size)
{
  v8::ResourceConstraints limit;

  if (max_young_space_size) limit.set_max_young_generation_size_in_bytes(max_young_space_size);
  if (max_old_space_size) limit.set_max_old_generation_size_in_bytes(max_old_space_size);
  //TODO should this be code range size instead?
  //if (max_executable_size) limit.set_max_executable_size(max_executable_size);

  //TODO - memory limits are now only settable on isolate creation
  //return v8::SetResourceConstraints(v8::Isolate::GetCurrent(), &limit);
  return false;
}

uintptr_t CEngine::CalcStackLimitSize(uintptr_t size)
{
  uintptr_t frame = reinterpret_cast<uintptr_t>(&size);
  uintptr_t answer = frame  - size;

  // If the size is very large and the stack is very near the bottom of
  // memory then the calculation above may wrap around and give an address
  // that is above the (downwards-growing) stack.  In that case we return
  // 0 meaning the new stack limit is not going to be set.
  if (answer > frame)
    return 0;

  return answer;
}

void CEngine::SetStackLimit(uintptr_t stack_limit_size)
{
  /*
  v8::ResourceConstraints limit;

  limit.set_stack_limit(CalcStackLimitSize(stack_limit_size));

  return v8::SetResourceConstraints(v8::Isolate::GetCurrent(), &limit);
  */
  if (stack_limit_size < 1024)
    return;

  uintptr_t stack_limit = CalcStackLimitSize(stack_limit_size);
  if (!stack_limit)
    std::cerr << "[ERROR] Attempted to set a stack limit greater than available memory" << std::endl;
    return;

  v8::Isolate::GetCurrent()->SetStackLimit(stack_limit);
}

py::object CEngine::InternalPreCompile(v8::Handle<v8::String> src)
{
  /*
   * Precompiling is no longer supported
  v8::TryCatch try_catch;

  std::auto_ptr<v8::ScriptData> precompiled;

  Py_BEGIN_ALLOW_THREADS

  precompiled.reset(v8::ScriptData::PreCompile(src));

  Py_END_ALLOW_THREADS

  if (!precompiled.get()) CJavascriptException::ThrowIf(m_isolate, try_catch);
  if (precompiled->HasError()) throw CJavascriptException("fail to compile", ::PyExc_SyntaxError);

  py::object obj(py::handle<>(::PyByteArray_FromStringAndSize(precompiled->Data(), precompiled->Length())));

  return obj;
  */
  py::object obj;
  return obj;
}

boost::shared_ptr<CScript> CEngine::InternalCompile(v8::Handle<v8::String> src,
                                                    v8::Handle<v8::Value> name,
                                                    int line, int col,
                                                    py::object precompiled)
{
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  v8::Persistent<v8::String> script_source(m_isolate, src);

  //v8::Handle<v8::Script> script;
  v8::MaybeLocal<v8::Script> script;
  v8::Handle<v8::String> source = v8::Local<v8::String>::New(m_isolate, script_source);
  /*
   * TODO -- looks like ScriptData as a class or concept no longer
   * exists
  std::shared_ptr<v8::ScriptData> script_data;

  if (!precompiled.is_none())
  {
    if (PyObject_CheckBuffer(precompiled.ptr()))
    {
      Py_buffer buf;

      if (-1 == ::PyObject_GetBuffer(precompiled.ptr(), &buf, PyBUF_WRITABLE))
      {
        throw CJavascriptException("fail to get data from the precompiled buffer");
      }

      script_data.reset(v8::ScriptData::New((const char *)buf.buf, (int) buf.len));

      ::PyBuffer_Release(&buf);
    }
    else
    {
      throw CJavascriptException("need a precompiled buffer object");
    }
  }
  */

  Py_BEGIN_ALLOW_THREADS

  if (line >= 0 && col >= 0)
  {
    v8::ScriptOrigin script_origin(name, v8::Integer::New(m_isolate, line), v8::Integer::New(m_isolate, col));

    //script = v8::Script::Compile(source, &script_origin, script_data.get());
    script = v8::Script::Compile(context, source, &script_origin);
  }
  else
  {
    v8::ScriptOrigin script_origin(name);

    //script = v8::Script::Compile(source, &script_origin, script_data.get());
    script = v8::Script::Compile(context, source, &script_origin);
  }

  Py_END_ALLOW_THREADS

#ifdef SUPPORT_PROBES
  if (ENGINE_SCRIPT_COMPILE_ENABLED()) {
    v8::String::Utf8Value s(source);
    v8::String::Utf8Value n(v8::Handle<v8::String>::Cast(name));

    ENGINE_SCRIPT_COMPILE(&script, *s, *n, line, col);
  }
#endif

  if (script.IsEmpty()) CJavascriptException::ThrowIf(m_isolate, try_catch);

  return boost::shared_ptr<CScript>(new CScript(m_isolate, *this, script_source, script.ToLocalChecked()));
}

py::object CEngine::ExecuteScript(v8::Handle<v8::Script> script)
{
#ifdef SUPPORT_PROBES
  if (ENGINE_SCRIPT_RUN_ENABLED()) {
    ENGINE_SCRIPT_RUN(&script);
  }
#endif

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  v8::MaybeLocal<v8::Value> result;

  Py_BEGIN_ALLOW_THREADS

  result = script->Run(context);

  Py_END_ALLOW_THREADS

  if (result.IsEmpty())
  {
    if (try_catch.HasCaught())
    {
      if(!try_catch.CanContinue() && PyErr_OCCURRED())
      {
        throw py::error_already_set();
      }

      CJavascriptException::ThrowIf(m_isolate, try_catch);
    }

    result = v8::Null(m_isolate);
  }

  return CJavascriptObject::Wrap(result.ToLocalChecked());
}

const std::string CScript::GetSource(void) const
{
  v8::HandleScope handle_scope(m_isolate);

  v8::String::Utf8Value source(m_isolate, Source());

  return std::string(*source, source.length());
}

py::object CScript::Run(void)
{
  v8::HandleScope handle_scope(m_isolate);

  return m_engine.ExecuteScript(Script());
}

#ifdef SUPPORT_EXTENSION
/*
class CPythonExtension : public v8::Extension
{
  py::object m_callback;
*/
#endif


