#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "Utils.h"

class CScript;

typedef std::shared_ptr<CScript> CScriptPtr;

class CEngine
{
    v8::Isolate *m_isolate;

    static uintptr_t CalcStackLimitSize(uintptr_t size);
protected:
    CScriptPtr InternalCompile(v8::Handle<v8::String> src, v8::Handle<v8::Value> name, int line, int col);

    static void TerminateAllThreads(void);

    static void ReportFatalError(const char* location, const char* message);
    static void ReportMessage(v8::Handle<v8::Message> message, v8::Handle<v8::Value> data);
public:
    CEngine(v8::Isolate *isolate = NULL) : m_isolate(isolate ? isolate : v8::Isolate::GetCurrent()) {}

    CScriptPtr Compile(const std::string& src, const std::string name = std::string(),
                       int line = -1, int col = -1)
    {
        v8::HandleScope scope(m_isolate);

        return InternalCompile(ToString(src), ToString(name), line, col);
    }

    CScriptPtr CompileW(const std::wstring& src, const std::wstring name = std::wstring(),
                        int line = -1, int col = -1)
    {
        v8::HandleScope scope(m_isolate);

        return InternalCompile(ToString(src), ToString(name), line, col);
    }

    void RaiseError(v8::TryCatch& try_catch);
public:
    static void Expose(void);

    static const std::string GetVersion(void) {
        return v8::V8::GetVersion();
    }

    // static bool SetMemoryLimit(int max_young_space_size, int max_old_space_size, int max_executable_size);
    static void SetStackLimit(uintptr_t stack_limit_size);

    py::object ExecuteScript(v8::Handle<v8::Script> script);

    static void SetFlags(const std::string& flags) {
        v8::V8::SetFlagsFromString(flags.c_str(), flags.size());
    }

    static void SetSerializeEnable(bool value);
    static bool IsSerializeEnabled(void);

    static py::object Serialize(void);
    static void Deserialize(py::object snapshot);
    static bool IsDead(void);
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

    v8::Handle<v8::String> Source() const {
        return v8::Local<v8::String>::New(m_isolate, m_source);
    }

    v8::Handle<v8::Script> Script() const {
        return v8::Local<v8::Script>::New(m_isolate, m_script);
    }

    const std::string GetSource(void) const;

    py::object Run(void);
};
