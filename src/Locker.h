#pragma once

#include <v8-isolate.h>
#include <v8-value.h>
#include <v8-context.h>
#include <v8-message.h>
#include <v8-local-handle.h>
#include <v8-exception.h>
#include <v8-persistent-handle.h>
#include <v8-script.h>

#include "Exception.h"
#include "Context.h"
#include "Utils.h"


class CLocker
{
    static bool s_preemption;

    std::unique_ptr<v8::Locker> m_locker;
    CIsolatePtr m_isolate;
public:
    CLocker() {}
    CLocker(CIsolatePtr isolate) : m_isolate(isolate) {}
    ~CLocker()
    {
        if (NULL != m_locker.get())
            m_locker.release();
    }

    bool entered(void)
    {
        return NULL != m_locker.get();
    }

    void enter(void);
    void leave(void);

    static bool IsLocked();

    static void Expose(void);
};


class CUnlocker
{
    std::unique_ptr<v8::Unlocker> m_unlocker;
public:
    bool entered(void) {
        return NULL != m_unlocker.get();
    }

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
