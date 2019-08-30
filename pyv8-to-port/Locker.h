#pragma once

#include "Exception.h"
#include "Context.h"
#include "Utils.h"

class CLocker
{
  static bool s_preemption;

  std::auto_ptr<v8::Locker> m_locker;
  CIsolatePtr m_isolate;
public:
  CLocker() {}
  CLocker(CIsolatePtr isolate) : m_isolate(isolate)
  {

  }
  bool entered(void) { return NULL != m_locker.get(); }

  void enter(void);
  void leave(void);

  static bool IsLocked();

  static void Expose(void);
};

class CUnlocker
{
  std::auto_ptr<v8::Unlocker> m_unlocker;
public:
  bool entered(void) { return NULL != m_unlocker.get(); }

  void enter(void)
  {
    Py_BEGIN_ALLOW_THREADS

    m_unlocker.reset(new v8::Unlocker(v8::Isolate::GetCurrent()));

    Py_END_ALLOW_THREADS
  }
  void leave(void)
  {
    Py_BEGIN_ALLOW_THREADS

    m_unlocker.reset();

    Py_END_ALLOW_THREADS
  }
};
