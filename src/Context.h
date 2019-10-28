#pragma once

#include <cassert>

#include <boost/shared_ptr.hpp>

#include "Isolate.h"
#include "Platform.h"
#include "Wrapper.h"
#include "Utils.h"


class CContext;
class CIsolate;

typedef boost::shared_ptr<CContext> CContextPtr;
typedef boost::shared_ptr<CIsolate> CIsolatePtr;


class CContext
{
  py::object m_global;
  v8::Persistent<v8::Context> m_context;
public:
  CContext(v8::Handle<v8::Context> context);
  CContext(const CContext& context);
  CContext(py::object global, py::list extensions);

  ~CContext()
  {
    m_context.Reset();
  }

  v8::Handle<v8::Context> Handle(void) const { return v8::Local<v8::Context>::New(v8::Isolate::GetCurrent(), m_context); }

  py::object GetGlobal(void);

  py::str GetSecurityToken(void);
  void SetSecurityToken(py::str token);

  bool IsEntered(void) { return !m_context.IsEmpty(); }
  void Enter(void) { v8::HandleScope handle_scope(v8::Isolate::GetCurrent()); Handle()->Enter(); }
  void Leave(void) { v8::HandleScope handle_scope(v8::Isolate::GetCurrent()); Handle()->Exit(); }

  py::object Evaluate(const std::string& src, const std::string name = std::string(),
                      int line = -1, int col = -1);
  py::object EvaluateW(const std::wstring& src, const std::wstring name = std::wstring(),
                       int line = -1, int col = -1);

  static py::object GetEntered(void);
  static py::object GetCurrent(void);
  static py::object GetCalling(void);
  static bool InContext(void) { return v8::Isolate::GetCurrent()->InContext(); }

  static void Expose(void);
};
