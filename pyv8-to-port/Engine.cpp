#include "Engine.h"

#include <iostream>

#include <boost/preprocessor.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

#ifdef SUPPORT_SERIALIZE
  CEngine::CounterTable CEngine::m_counters;
#endif

#ifdef SUPPORT_AST
  #include "AST.h"
#endif

struct MemoryAllocationCallbackBase
{
  virtual void Set(py::object callback) = 0;
};

template <v8::ObjectSpace SPACE, v8::AllocationAction ACTION>
struct MemoryAllocationCallbackStub : public MemoryAllocationCallbackBase
{
  static py::object s_callback;

  typedef boost::mutex lock_t;
  typedef boost::lock_guard<lock_t> lock_guard_t;

  static lock_t s_callbackLock;

  static void onMemoryAllocation(v8::ObjectSpace space, v8::AllocationAction action, int size)
  {
    lock_guard_t hold(s_callbackLock);

    if (!s_callback.is_none()) s_callback(space, action, size);
  }

  virtual void Set(py::object callback)
  {
    lock_guard_t hold(s_callbackLock);

    if (s_callback.is_none() && !callback.is_none())
    {
      v8::V8::AddMemoryAllocationCallback(&onMemoryAllocation, SPACE, ACTION);
    }
    else if (!s_callback.is_none() && callback.is_none())
    {
      v8::V8::RemoveMemoryAllocationCallback(&onMemoryAllocation);
    }

    s_callback = callback;
  }
};

template<v8::ObjectSpace space, v8::AllocationAction action>
py::object MemoryAllocationCallbackStub<space, action>::s_callback;

template<v8::ObjectSpace space, v8::AllocationAction action>
typename MemoryAllocationCallbackStub<space, action>::lock_t MemoryAllocationCallbackStub<space, action>::s_callbackLock;

class MemoryAllocationManager
{
  typedef std::map<std::pair<v8::ObjectSpace, v8::AllocationAction>, MemoryAllocationCallbackBase *> CallbackMap;

  static CallbackMap s_callbacks;
public:
  static void Init(void)
  {
#define ADD_CALLBACK_STUB(space, action) s_callbacks[std::make_pair(space, action)] = new MemoryAllocationCallbackStub<space, action>()

#define OBJECT_SPACES (kObjectSpaceNewSpace) (kObjectSpaceOldPointerSpace) (kObjectSpaceOldDataSpace) \
(kObjectSpaceCodeSpace) (kObjectSpaceMapSpace) (kObjectSpaceLoSpace) (kObjectSpaceAll)
#define ALLOCATION_ACTIONS (kAllocationActionAllocate) (kAllocationActionFree) (kAllocationActionAll)

#define ADD_CALLBACK_STUBS(r, action, space) ADD_CALLBACK_STUB(v8::space, v8::action);

    BOOST_PP_SEQ_FOR_EACH(ADD_CALLBACK_STUBS, kAllocationActionAllocate, OBJECT_SPACES);
    BOOST_PP_SEQ_FOR_EACH(ADD_CALLBACK_STUBS, kAllocationActionFree, OBJECT_SPACES);
    BOOST_PP_SEQ_FOR_EACH(ADD_CALLBACK_STUBS, kAllocationActionAll, OBJECT_SPACES);
  }

  static void SetCallback(py::object callback, v8::ObjectSpace space, v8::AllocationAction action)
  {
    s_callbacks[std::make_pair(space, action)]->Set(callback);
  }
};

MemoryAllocationManager::CallbackMap MemoryAllocationManager::s_callbacks;

void CEngine::Expose(void)
{
#ifndef SUPPORT_SERIALIZE
  v8::V8::SetFatalErrorHandler(ReportFatalError);
  v8::V8::AddMessageListener(ReportMessage);
#endif

  MemoryAllocationManager::Init();

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

  py::class_<CEngine, boost::noncopyable>("JSEngine", "JSEngine is a backend Javascript engine.")
    .def(py::init<>("Create a new script engine instance."))
    .add_static_property("version", &CEngine::GetVersion,
                         "Get the V8 engine version.")

    .add_static_property("dead", &v8::V8::IsDead,
                         "Check if V8 is dead and therefore unusable.")

    .def("setFlags", &CEngine::SetFlags, "Sets V8 flags from a string.")
    .staticmethod("setFlags")

    .def("collect", &CEngine::CollectAllGarbage, (py::arg("force")=true),
         "Performs a full garbage collection. Force compaction if the parameter is true.")
    .staticmethod("collect")

  #ifdef SUPPORT_SERIALIZE
    .add_static_property("serializeEnabled", &CEngine::IsSerializeEnabled, &CEngine::SetSerializeEnable)

    .def("serialize", &CEngine::Serialize)
    .staticmethod("serialize")

    .def("deserialize", &CEngine::Deserialize)
    .staticmethod("deserialize")
  #endif

    .def("terminateAllThreads", &CEngine::TerminateAllThreads,
         "Forcefully terminate the current thread of JavaScript execution.")
    .staticmethod("terminateAllThreads")

    .def("dispose", &v8::V8::Dispose,
         "Releases any resources used by v8 and stops any utility threads "
         "that may be running. Note that disposing v8 is permanent, "
         "it cannot be reinitialized.")
    .staticmethod("dispose")

    .def("idle", &v8::V8::IdleNotification,
         "Optional notification that the embedder is idle.")
    .staticmethod("idle")

    .def("lowMemory", &v8::V8::LowMemoryNotification,
         "Optional notification that the system is running low on memory.")
    .staticmethod("lowMemory")

    .def("setMemoryLimit", &CEngine::SetMemoryLimit, (py::arg("max_young_space_size") = 0,
                                                      py::arg("max_old_space_size") = 0,
                                                      py::arg("max_executable_size") = 0),
         "Specifies the limits of the runtime's memory use."
         "You must set the heap size before initializing the VM"
         "the size cannot be adjusted after the VM is initialized.")
    .staticmethod("setMemoryLimit")

    .def("ignoreOutOfMemoryException", &v8::V8::IgnoreOutOfMemoryException,
         "Ignore out-of-memory exceptions.")
    .staticmethod("ignoreOutOfMemoryException")

    .def("setStackLimit", &CEngine::SetStackLimit, (py::arg("stack_limit_size") = 0),
         "Uses the address of a local variable to determine the stack top now."
         "Given a size, returns an address that is that far from the current top of stack.")
    .staticmethod("setStackLimit")

    .def("setMemoryAllocationCallback", &MemoryAllocationManager::SetCallback,
                                        (py::arg("callback"),
                                         py::arg("space") = v8::kObjectSpaceAll,
                                         py::arg("action") = v8::kAllocationActionAll),
                                        "Enables the host application to provide a mechanism to be notified "
                                        "and perform custom logging when V8 Allocates Executable Memory.")
    .staticmethod("setMemoryAllocationCallback")

    .def("precompile", &CEngine::PreCompile, (py::arg("source")))
    .def("precompile", &CEngine::PreCompileW, (py::arg("source")))

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

  #ifdef SUPPORT_AST
    .def("visit", &CScript::visit, (py::arg("handler"),
                                    py::arg("mode") = v8i::CLASSIC_MODE),
         "Visit the AST of code with the callback handler.")
  #endif
    ;

  py::objects::class_value_wrapper<boost::shared_ptr<CScript>,
    py::objects::make_ptr_instance<CScript,
    py::objects::pointer_holder<boost::shared_ptr<CScript>, CScript> > >();

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
}

#ifdef SUPPORT_SERIALIZE

int *CEngine::CounterLookup(const char* name)
{
  CounterTable::const_iterator it = m_counters.find(name);

  if (it == m_counters.end())
    m_counters[name] = 0;

  return &m_counters[name];
}

void CEngine::SetSerializeEnable(bool value)
{
  if (value)
    v8i::Serializer::Enable();
  else
    v8i::Serializer::Disable();
}
bool CEngine::IsSerializeEnabled(void)
{
  return v8i::Serializer::enabled();
}

struct PyBufferByteSink : public v8i::SnapshotByteSink
{
  std::vector<v8i::byte> m_data;

  virtual void Put(int byte, const char* description)
  {
    m_data.push_back(byte);
  }

  virtual int Position()
  {
    return (int) m_data.size();
  }
};

py::object CEngine::Serialize(void)
{
  v8::V8::Initialize();

  PyBufferByteSink sink;

  v8i::StartupSerializer serializer(&sink);

  serializer.Serialize();

  py::object obj(py::handle<>(::PyByteArray_FromStringAndSize((const char *) &sink.m_data[0], sink.m_data.size())));

  return obj;
}
void CEngine::Deserialize(py::object snapshot)
{
  Py_buffer buf;

  if (!snapshot.is_none() && PyObject_CheckBuffer(snapshot.ptr()))
  {
    if (0 == ::PyObject_GetBuffer(snapshot.ptr(), &buf, PyBUF_WRITABLE))
    {
      Py_BEGIN_ALLOW_THREADS

      v8i::SnapshotByteSource source((const v8i::byte *) buf.buf, (int) buf.len);
      v8i::Deserializer deserializer(&source);

      deserializer.Deserialize();

      v8i::V8::Initialize(&deserializer);

      Py_END_ALLOW_THREADS

      ::PyBuffer_Release(&buf);
    }
  }
}

#endif // SUPPORT_SERIALIZE

void CEngine::CollectAllGarbage(bool force_compaction)
{
  v8i::HandleScope handle_scope(v8i::Isolate::Current());

  if (force_compaction) {
    v8i::Isolate::Current()->heap()->CollectAllAvailableGarbage();
  } else {
    v8i::Isolate::Current()->heap()->CollectAllGarbage(v8i::Heap::kMakeHeapIterableMask);
  }
}

void CEngine::TerminateAllThreads(void)
{
  v8::V8::TerminateExecution(v8::Isolate::GetCurrent());
}

void CEngine::ReportFatalError(const char* location, const char* message)
{
  std::cerr << "<" << location << "> " << message << std::endl;
}

void CEngine::ReportMessage(v8::Handle<v8::Message> message, v8::Handle<v8::Value> data)
{
  v8::String::Utf8Value filename(message->GetScriptResourceName());
  int lineno = message->GetLineNumber();
  v8::String::Utf8Value sourceline(message->GetSourceLine());

  std::cerr << *filename << ":" << lineno << " -> " << *sourceline << std::endl;
}

bool CEngine::SetMemoryLimit(int max_young_space_size, int max_old_space_size, int max_executable_size)
{
  v8::ResourceConstraints limit;

  if (max_young_space_size) limit.set_max_young_space_size(max_young_space_size);
  if (max_old_space_size) limit.set_max_old_space_size(max_old_space_size);
  if (max_executable_size) limit.set_max_executable_size(max_executable_size);

  return v8::SetResourceConstraints(v8::Isolate::GetCurrent(), &limit);
}

// Uses the address of a local variable to determine the stack top now.
// Given a size, returns an address that is that far from the current
// top of stack.
uint32_t *CEngine::CalcStackLimitSize(uint32_t size)
{
  uint32_t* answer = &size - (size / sizeof(size));

  // If the size is very large and the stack is very near the bottom of
  // memory then the calculation above may wrap around and give an address
  // that is above the (downwards-growing) stack.  In that case we return
  // a very low address.
  if (answer > &size) return reinterpret_cast<uint32_t*>(sizeof(size));

  return answer;
}

bool CEngine::SetStackLimit(uint32_t stack_limit_size)
{
  v8::ResourceConstraints limit;

  limit.set_stack_limit(CalcStackLimitSize(stack_limit_size));

  return v8::SetResourceConstraints(v8::Isolate::GetCurrent(), &limit);
}

py::object CEngine::InternalPreCompile(v8::Handle<v8::String> src)
{
  v8::TryCatch try_catch;

  std::auto_ptr<v8::ScriptData> precompiled;

  Py_BEGIN_ALLOW_THREADS

  precompiled.reset(v8::ScriptData::PreCompile(src));

  Py_END_ALLOW_THREADS

  if (!precompiled.get()) CJavascriptException::ThrowIf(m_isolate, try_catch);
  if (precompiled->HasError()) throw CJavascriptException("fail to compile", ::PyExc_SyntaxError);

  py::object obj(py::handle<>(::PyByteArray_FromStringAndSize(precompiled->Data(), precompiled->Length())));

  return obj;
}

boost::shared_ptr<CScript> CEngine::InternalCompile(v8::Handle<v8::String> src,
                                                    v8::Handle<v8::Value> name,
                                                    int line, int col,
                                                    py::object precompiled)
{
  v8::HandleScope handle_scope(m_isolate);

  v8::TryCatch try_catch;

  v8::Persistent<v8::String> script_source(m_isolate, src);

  v8::Handle<v8::Script> script;
  v8::Handle<v8::String> source = v8::Local<v8::String>::New(m_isolate, script_source);
  std::auto_ptr<v8::ScriptData> script_data;

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

  Py_BEGIN_ALLOW_THREADS

  if (line >= 0 && col >= 0)
  {
    v8::ScriptOrigin script_origin(name, v8::Integer::New(m_isolate, line), v8::Integer::New(m_isolate, col));

    script = v8::Script::Compile(source, &script_origin, script_data.get());
  }
  else
  {
    v8::ScriptOrigin script_origin(name);

    script = v8::Script::Compile(source, &script_origin, script_data.get());
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

  return boost::shared_ptr<CScript>(new CScript(m_isolate, *this, script_source, script));
}

py::object CEngine::ExecuteScript(v8::Handle<v8::Script> script)
{
#ifdef SUPPORT_PROBES
  if (ENGINE_SCRIPT_RUN_ENABLED()) {
    ENGINE_SCRIPT_RUN(&script);
  }
#endif

  v8::HandleScope handle_scope(m_isolate);

  v8::TryCatch try_catch;

  v8::Handle<v8::Value> result;

  Py_BEGIN_ALLOW_THREADS

  result = script->Run();

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

  return CJavascriptObject::Wrap(result);
}

#ifdef SUPPORT_AST

void CScript::visit(py::object handler, v8i::LanguageMode mode) const
{
  v8::HandleScope handle_scope(m_isolate);

  v8i::Handle<v8i::Object> obj = v8::Utils::OpenHandle(*Source());

  v8i::Handle<v8i::Script> script(v8i::Isolate::Current()->factory()->NewScript(obj));

  v8i::CompilationInfoWithZone info(script);
  v8i::Isolate *isolate = info.isolate();

  info.MarkAsGlobal();
  info.SetContext(isolate->native_context());
  info.SetLanguageMode(mode);

  v8i::ZoneScope zone_scope(info.zone());
  v8i::PostponeInterruptsScope postpone(isolate);

  v8i::FixedArray* array = isolate->native_context()->embedder_data();
  script->set_context_data(array->get(0));

  {
    v8i::Parser parser(&info);

    if (parser.Parse()) {
      if (::PyObject_HasAttrString(handler.ptr(), "onProgram"))
      {
        handler.attr("onProgram")(CAstFunctionLiteral(info.zone(), info.function()));
      }
    }
  }
}

#endif

const std::string CScript::GetSource(void) const
{
  v8::HandleScope handle_scope(m_isolate);

  v8::String::Utf8Value source(Source());

  return std::string(*source, source.length());
}

py::object CScript::Run(void)
{
  v8::HandleScope handle_scope(m_isolate);

  return m_engine.ExecuteScript(Script());
}

#ifdef SUPPORT_EXTENSION

class CPythonExtension : public v8::Extension
{
  py::object m_callback;

  static void CallStub(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    v8::HandleScope handle_scope(args.GetIsolate());
    CPythonGIL python_gil;
    py::object func = *static_cast<py::object *>(v8::External::Cast(*args.Data())->Value());

    py::object result;

    switch (args.Length())
    {
    case 0: result = func(); break;
    case 1: result = func(CJavascriptObject::Wrap(args[0])); break;
    case 2: result = func(CJavascriptObject::Wrap(args[0]), CJavascriptObject::Wrap(args[1])); break;
    case 3: result = func(CJavascriptObject::Wrap(args[0]), CJavascriptObject::Wrap(args[1]),
                          CJavascriptObject::Wrap(args[2])); break;
    case 4: result = func(CJavascriptObject::Wrap(args[0]), CJavascriptObject::Wrap(args[1]),
                          CJavascriptObject::Wrap(args[2]), CJavascriptObject::Wrap(args[3])); break;
    case 5: result = func(CJavascriptObject::Wrap(args[0]), CJavascriptObject::Wrap(args[1]),
                          CJavascriptObject::Wrap(args[2]), CJavascriptObject::Wrap(args[3]),
                          CJavascriptObject::Wrap(args[4])); break;
    case 6: result = func(CJavascriptObject::Wrap(args[0]), CJavascriptObject::Wrap(args[1]),
                          CJavascriptObject::Wrap(args[2]), CJavascriptObject::Wrap(args[3]),
                          CJavascriptObject::Wrap(args[4]), CJavascriptObject::Wrap(args[5])); break;
    case 7: result = func(CJavascriptObject::Wrap(args[0]), CJavascriptObject::Wrap(args[1]),
                          CJavascriptObject::Wrap(args[2]), CJavascriptObject::Wrap(args[3]),
                          CJavascriptObject::Wrap(args[4]), CJavascriptObject::Wrap(args[5]),
                          CJavascriptObject::Wrap(args[6])); break;
    case 8: result = func(CJavascriptObject::Wrap(args[0]), CJavascriptObject::Wrap(args[1]),
                          CJavascriptObject::Wrap(args[2]), CJavascriptObject::Wrap(args[3]),
                          CJavascriptObject::Wrap(args[4]), CJavascriptObject::Wrap(args[5]),
                          CJavascriptObject::Wrap(args[7]), CJavascriptObject::Wrap(args[8])); break;
    default:
      args.GetIsolate()->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(args.GetIsolate(), "too many arguments")));
      break;
    }

    if (result.is_none()) {
      args.GetReturnValue().SetNull();
    } else if (result.ptr() == Py_True) {
      args.GetReturnValue().Set(true);
    } else if (result.ptr() == Py_False) {
      args.GetReturnValue().Set(false);
    } else {
      args.GetReturnValue().Set(CPythonObject::Wrap(result));
    }
  }
public:
  CPythonExtension(const char *name, const char *source, py::object callback, int dep_count, const char**deps)
    : v8::Extension(strdup(name), strdup(source), dep_count, deps), m_callback(callback)
  {

  }

  virtual v8::Handle<v8::FunctionTemplate> GetNativeFunctionTemplate(v8::Isolate* isolate, v8::Handle<v8::String> name)
  {
    v8::EscapableHandleScope handle_scope(isolate);
    CPythonGIL python_gil;

    py::object func;
    v8::String::Utf8Value func_name(name);
    std::string func_name_str(*func_name, func_name.length());

    try
    {
      if (::PyCallable_Check(m_callback.ptr()))
      {
        func = m_callback(func_name_str);
      }
      else if (::PyObject_HasAttrString(m_callback.ptr(), *func_name))
      {
        func = m_callback.attr(func_name_str.c_str());
      }
      else
      {
        return v8::Handle<v8::FunctionTemplate>();
      }
    }
    catch (const std::exception& ex) { isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(isolate, ex.what()))); }
    catch (const py::error_already_set&) { CPythonObject::ThrowIf(isolate); }
    catch (...) { isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(isolate, "unknown exception"))); }

    v8::Handle<v8::External> func_data = v8::External::New(isolate, new py::object(func));
    v8::Handle<v8::FunctionTemplate> func_tmpl = v8::FunctionTemplate::New(isolate, CallStub, func_data);

    return handle_scope.Escape(v8::Local<v8::FunctionTemplate>(func_tmpl));
  }
};

std::vector< boost::shared_ptr<v8::Extension> > CExtension::s_extensions;

CExtension::CExtension(const std::string& name, const std::string& source,
                       py::object callback, py::list deps, bool autoRegister)
  : m_deps(deps), m_registered(false)
{
  for (Py_ssize_t i=0; i<PyList_Size(deps.ptr()); i++)
  {
    py::extract<const std::string> extractor(::PyList_GetItem(deps.ptr(), i));

    if (extractor.check())
    {
      m_depNames.push_back(extractor());
      m_depPtrs.push_back(m_depNames.rbegin()->c_str());
    }
  }

  m_extension.reset(new CPythonExtension(name.c_str(), source.c_str(),
    callback, m_depPtrs.size(), m_depPtrs.empty() ? NULL : &m_depPtrs[0]));

  if (autoRegister) this->Register();
}

void CExtension::Register(void)
{
  v8::RegisterExtension(m_extension.get());

  m_registered = true;

  s_extensions.push_back(m_extension);
}

py::list CExtension::GetExtensions(void)
{
  v8::RegisteredExtension *ext = v8::RegisteredExtension::first_extension();
  py::list extensions;

  while (ext)
  {
    extensions.append(ext->extension()->name());

    ext = ext->next();
  }

  return extensions;
}

#endif // SUPPORT_EXTENSION

