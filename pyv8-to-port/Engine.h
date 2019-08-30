#pragma once

#include <string>
#include <vector>
#include <map>

#include <boost/shared_ptr.hpp>

#include "Context.h"
#include "Utils.h"

#include "V8Internal.h"

class CScript;

typedef boost::shared_ptr<CScript> CScriptPtr;

class CEngine
{
  v8::Isolate *m_isolate;

  static uint32_t *CalcStackLimitSize(uint32_t size);
protected:
  py::object InternalPreCompile(v8::Handle<v8::String> src);
  CScriptPtr InternalCompile(v8::Handle<v8::String> src, v8::Handle<v8::Value> name, int line, int col, py::object precompiled);

#ifdef SUPPORT_SERIALIZE

  typedef std::map<std::string, int> CounterTable;
  static CounterTable m_counters;

  static int *CounterLookup(const char* name);
#endif

  static void CollectAllGarbage(bool force_compaction);
  static void TerminateAllThreads(void);

  static void ReportFatalError(const char* location, const char* message);
  static void ReportMessage(v8::Handle<v8::Message> message, v8::Handle<v8::Value> data);
public:
  CEngine(v8::Isolate *isolate = NULL) : m_isolate(isolate ? isolate : v8::Isolate::GetCurrent()) {}

  py::object PreCompile(const std::string& src)
  {
    v8::HandleScope scope(m_isolate);

    return InternalPreCompile(ToString(src));
  }
  py::object PreCompileW(const std::wstring& src)
  {
    v8::HandleScope scope(m_isolate);

    return InternalPreCompile(ToString(src));
  }

  CScriptPtr Compile(const std::string& src, const std::string name = std::string(),
                     int line = -1, int col = -1, py::object precompiled = py::object())
  {
    v8::HandleScope scope(m_isolate);

    return InternalCompile(ToString(src), ToString(name), line, col, precompiled);
  }
  CScriptPtr CompileW(const std::wstring& src, const std::wstring name = std::wstring(),
                      int line = -1, int col = -1, py::object precompiled = py::object())
  {
    v8::HandleScope scope(m_isolate);

    return InternalCompile(ToString(src), ToString(name), line, col, precompiled);
  }

  void RaiseError(v8::TryCatch& try_catch);
public:
  static void Expose(void);

  static const std::string GetVersion(void) { return v8::V8::GetVersion(); }
  static bool SetMemoryLimit(int max_young_space_size, int max_old_space_size, int max_executable_size);
  static bool SetStackLimit(uint32_t stack_limit_size);

  py::object ExecuteScript(v8::Handle<v8::Script> script);

  static void SetFlags(const std::string& flags) { v8::V8::SetFlagsFromString(flags.c_str(), flags.size()); }

  static void SetSerializeEnable(bool value);
  static bool IsSerializeEnabled(void);

  static py::object Serialize(void);
  static void Deserialize(py::object snapshot);
};

class CScript
{
  v8::Isolate *m_isolate;
  CEngine& m_engine;

  v8::Persistent<v8::String> m_source;
  v8::Persistent<v8::Script> m_script;
public:
  CScript(v8::Isolate *isolate, CEngine& engine, v8::Persistent<v8::String>& source, v8::Handle<v8::Script> script)
    : m_isolate(isolate), m_engine(engine), m_source(m_isolate, source), m_script(m_isolate, script)
  {

  }

  CScript(const CScript& script)
    : m_isolate(script.m_isolate), m_engine(script.m_engine)
  {
    v8::HandleScope handle_scope(m_isolate);

    m_source.Reset(m_isolate, script.Source());
    m_script.Reset(m_isolate, script.Script());
  }

  ~CScript()
  {
    m_source.Reset();
    m_script.Reset();
  }

  v8::Handle<v8::String> Source() const { return v8::Local<v8::String>::New(m_isolate, m_source); }
  v8::Handle<v8::Script> Script() const { return v8::Local<v8::Script>::New(m_isolate, m_script); }

#ifdef SUPPORT_AST
  void visit(py::object handler, v8i::LanguageMode mode=v8i::CLASSIC_MODE) const;
#endif

  const std::string GetSource(void) const;

  py::object Run(void);
};

#ifdef SUPPORT_EXTENSION

class CExtension
{
  py::list m_deps;
  std::vector<std::string> m_depNames;
  std::vector<const char *> m_depPtrs;

  bool m_registered;

  boost::shared_ptr<v8::Extension> m_extension;

  static std::vector< boost::shared_ptr<v8::Extension> > s_extensions;
public:
  CExtension(const std::string& name, const std::string& source, py::object callback, py::list dependencies, bool autoRegister);

  const std::string GetName(void) { return m_extension->name(); }
  const std::string GetSource(void) { return std::string(m_extension->source()->data(), m_extension->source()->length()); }

  bool IsRegistered(void) { return m_registered; }
  void Register(void);

  bool IsAutoEnable(void) { return m_extension->auto_enable(); }
  void SetAutoEnable(bool value) { m_extension->set_auto_enable(value); }

  py::list GetDependencies(void) { return m_deps; }

  static py::list GetExtensions(void);
};

#endif // SUPPORT_EXTENSION
