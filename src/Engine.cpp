#include "Engine.h"
#include "Exception.h"
#include "Wrapper.h"

#include <iostream>

#include <boost/preprocessor.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

void CEngine::Expose(void)
{
    py::class_<CEngine, boost::noncopyable>("JSEngine", "JSEngine is a backend Javascript engine.")
    .def(py::init<>("Create a new script engine instance."))
    .add_static_property("version", &CEngine::GetVersion,
                         "Get the V8 engine version.")

    .add_static_property("dead", &CEngine::IsDead,
                         "Check if V8 is dead and therefore unusable.")

    .def("setFlags", &CEngine::SetFlags, "Sets V8 flags from a string.")
    .staticmethod("setFlags")

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
    */

    .def("compile", &CEngine::Compile, (py::arg("source"),
                                        py::arg("name") = std::string(),
                                        py::arg("line") = -1,
                                        py::arg("col") = -1))
    .def("compile", &CEngine::CompileW, (py::arg("source"),
                                         py::arg("name") = std::wstring(),
                                         py::arg("line") = -1,
                                         py::arg("col") = -1))
    ;

    py::class_<CScript, boost::noncopyable>("JSScript", "JSScript is a compiled JavaScript script.", py::no_init)
    .add_property("source", &CScript::GetSource, "the source code")

    .def("run", &CScript::Run, "Execute the compiled code.")
    ;

    py::objects::class_value_wrapper<std::shared_ptr<CScript>,
    py::objects::make_ptr_instance<CScript,
    py::objects::pointer_holder<std::shared_ptr<CScript>, CScript> > >();
}

bool CEngine::IsDead(void)
{
    return v8::Isolate::GetCurrent()->IsDead();
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

/*
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
*/

void CEngine::SetStackLimit(uintptr_t stack_limit_size)
{
    // This function uses a local stack variable to determine the isolate's
    // stack limit
    uint32_t here;
    uintptr_t stack_limit = reinterpret_cast<uintptr_t>(&here) - stack_limit_size;

    // If the size is very large and the stack is near the bottom of memory
    // then the calculation above may wrap around and give an address that is
    // above the (downwards-growing) stack. In such case we alert the user
    // that the new stack limit is not going to be set and return
    if (stack_limit > reinterpret_cast<uintptr_t>(&here)) {
        std::cerr << "[ERROR] Attempted to set a stack limit greater than available memory" << std::endl;
        return;
    }

    v8::Isolate::GetCurrent()->SetStackLimit(stack_limit);
}

std::shared_ptr<CScript> CEngine::InternalCompile(v8::Handle<v8::String> src,
        v8::Handle<v8::Value> name,
        int line, int col)
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    v8::TryCatch try_catch(isolate);

    v8::Persistent<v8::String> script_source(m_isolate, src);

    v8::MaybeLocal<v8::Script> script;
    v8::Handle<v8::String> source = v8::Local<v8::String>::New(m_isolate, script_source);

    Py_BEGIN_ALLOW_THREADS

    if (line >= 0 && col >= 0)
    {
        v8::ScriptOrigin script_origin(m_isolate, name, line, col);
        script = v8::Script::Compile(context, source, &script_origin);
    }
    else
    {
        v8::ScriptOrigin script_origin(m_isolate, name);
        script = v8::Script::Compile(context, source, &script_origin);
    }

    Py_END_ALLOW_THREADS

    if (script.IsEmpty()) CJavascriptException::ThrowIf(m_isolate, try_catch);

    return std::shared_ptr<CScript>(new CScript(m_isolate, *this, script_source, script.ToLocalChecked()));
}

py::object CEngine::ExecuteScript(v8::Handle<v8::Script> script)
{
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
