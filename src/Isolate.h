#pragma once

#include <v8.h>
#include "Exception.h"

class CIsolate
{
  v8::Isolate *m_isolate;
  bool m_owner;
  void Init(bool owner);
public:
  CIsolate();
  CIsolate(bool owner);
  CIsolate(v8::Isolate *isolate);
  ~CIsolate(void);

  v8::Isolate *GetIsolate(void);

  CJavascriptStackTracePtr GetCurrentStackTrace(int frame_limit,
    v8::StackTrace::StackTraceOptions options);

  static py::object GetCurrent(void);

  void Enter(void) { m_isolate->Enter(); }
  void Leave(void) { m_isolate->Exit(); }
  void Dispose(void) { m_isolate->Dispose(); }

  bool IsLocked(void) { return v8::Locker::IsLocked(m_isolate); }
};
