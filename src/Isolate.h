#pragma once

#include <v8.h>
#include "Exception.h"

class CIsolate
{
    v8::Isolate *m_isolate;
    bool m_owner;
    void Init(bool owner);

    static constexpr int KB = 1024;
    static constexpr int MB = KB * 1024;
    static constexpr size_t heap_increase = 8 * MB;
    static constexpr size_t heap_max_increase = 64 * MB;
public:
    CIsolate();
    CIsolate(bool owner);
    CIsolate(v8::Isolate *isolate);
    ~CIsolate(void);

    v8::Isolate *GetIsolate(void);

    CJavascriptStackTracePtr GetCurrentStackTrace(int frame_limit,
            v8::StackTrace::StackTraceOptions options);

    static py::object GetCurrent(void);
    static size_t NearHeapLimitCallback(void* data, size_t current_heap_limit,
                                        size_t initial_heap_limit);

    void Enter(void) {
        m_isolate->Enter();
    }
    void Leave(void) {
        m_isolate->Exit();
    }
    void Dispose(void) {
        m_isolate->Dispose();
    }

    bool IsLocked(void) {
        return v8::Locker::IsLocked(m_isolate);
    }
};
