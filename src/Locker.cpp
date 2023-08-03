#include "Locker.h"

bool CLocker::s_preemption = false;

void CLocker::enter(void)
{
    Py_BEGIN_ALLOW_THREADS

    m_locker.reset(new v8::Locker(m_isolate.get() ? m_isolate->GetIsolate() : v8::Isolate::GetCurrent()));

    Py_END_ALLOW_THREADS
}

void CLocker::leave(void)
{
    Py_BEGIN_ALLOW_THREADS

    m_locker.reset();

    Py_END_ALLOW_THREADS
}

bool CLocker::IsLocked()
{
    return v8::Locker::IsLocked(v8::Isolate::GetCurrent());
}

void CLocker::Expose(void)
{
    py::class_<CLocker, boost::noncopyable>("JSLocker", py::no_init)
    .def(py::init<>())
    .def(py::init<CIsolatePtr>((py::arg("isolate"))))

    .add_static_property("locked", &CLocker::IsLocked,
                         "whether or not the locker is locked by the current thread.")

    .def("entered", &CLocker::entered)
    .def("enter", &CLocker::enter)
    .def("leave", &CLocker::leave)
    ;

    py::class_<CUnlocker, boost::noncopyable>("JSUnlocker")
    .def("entered", &CUnlocker::entered)
    .def("enter", &CUnlocker::enter)
    .def("leave", &CUnlocker::leave)
    ;
}
