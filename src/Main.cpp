/*
 * Rough working notes and random flailing for learning
 * To be deleted
 */

#include <stdlib.h>
#include <iostream>
#include <vector>

#include <boost/python/raw_function.hpp>
#include <descrobject.h>
#include <datetime.h>

#include "libplatform/libplatform.h"

#include "Wrapper.h"
#include "Context.h"
#include "Utils.h"
#include "Engine.h"

static v8::Isolate* GetNewIsolate()
{
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
    v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  v8::Isolate* isolate = v8::Isolate::New(create_params);
  return isolate;
}

std::string toString(v8::Isolate* isolate, v8::Handle<v8::Value> v)
{
  auto name = v->ToString(isolate->GetCurrentContext()).ToLocalChecked();
  int length = name->Utf8Length(isolate);
  char* buffer = new char[length + 1]{};
  name->WriteUtf8(isolate, buffer);
  return(std::string(buffer));
}

std::string heavyToString(v8::Isolate* isolate, v8::Persistent<v8::Value>* p)
{
  v8::HandleScope handle_scope(isolate); 
  v8::Local<v8::Value> v = p->Get(isolate);

  auto name = v->ToString(isolate->GetCurrentContext()).ToLocalChecked();
  int length = name->Utf8Length(isolate);
  char* buffer = new char[length + 1]{};
  name->WriteUtf8(isolate, buffer);
  return(std::string(buffer));
}



py::object run_python_code(const char *code)
{
  py::object main_module = py::import("__main__");
  py::object main_namespace = main_module.attr("__dict__");
  py::object result = py::exec(code, main_namespace);
  return main_namespace["result"];
}

int main___test_wrapping_python_code(int argc, char* argv[])
{
  /*
     const char *code =
     "import datetime\n"
     "result = datetime.time()\n"
     "print(result)";

  const char *code =
    "def foo():\n"
    "  print('foo called')\n"
    "\n"
    "result = foo\n"
    "print(result)";

     */
     const char *code =
    "result = ['a','b','c']\n"
    "print(result)";


  Py_Initialize();
  PyDateTime_IMPORT;

  py::object pyvar = run_python_code(code); 

  v8::V8::InitializeICUDefaultLocation(argv[0]);
  v8::V8::InitializeExternalStartupData(argv[0]);
  std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();
  v8::Isolate* isolate = GetNewIsolate();
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = v8::Context::New(isolate);
  v8::Context::Scope context_scope(context);

  v8::Handle<v8::Value> v = CPythonObject::Wrap(pyvar);
  std::cout << "Wrapped it" << std::endl;
  std::cout << "Got a value back of size " << sizeof(v) << std::endl;
  CPythonObject::Dispose(v);
  return 0;
}

v8::Local<v8::Value> run_javascript(v8::Isolate* isolate, v8::Local<v8::Context> context, const char *code)
{
  v8::Local<v8::String> source =
    v8::String::NewFromUtf8(isolate, code, v8::NewStringType::kNormal)
    .ToLocalChecked();

  v8::Local<v8::Script> script =
    v8::Script::Compile(context, source).ToLocalChecked();

  return script->Run(context).ToLocalChecked();
}

int main__test_wrapping_v8_code(int argc, char* argv[])
{
  //const char* code = "['apples','oranges']";
  //const char* code = "x = { 'a' : 'b' }";
  //const char* code = "fn = function foo(baz) { console.write(baz); }";
  const char* code = "date1 = new Date('December 17, 1995 03:24:00');";

  Py_Initialize();

  /*
  PyDateTime_IMPORT;

  py::class_<CJavascriptObject, boost::noncopyable>("JSObject", py::no_init)
    ;

  py::class_<CJavascriptArray, py::bases<CJavascriptObject>, boost::noncopyable>("JSArray", py::no_init)
    .def(py::init<py::object>())
    ;

  py::objects::class_value_wrapper<boost::shared_ptr<CJavascriptObject>,
    py::objects::make_ptr_instance<CJavascriptObject,
    py::objects::pointer_holder<boost::shared_ptr<CJavascriptObject>,CJavascriptObject> > >();
*/
  CWrapper::Expose();
  v8::V8::InitializeICUDefaultLocation(argv[0]);
  v8::V8::InitializeExternalStartupData(argv[0]);
  std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();
  v8::Isolate* isolate = GetNewIsolate();
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = v8::Context::New(isolate);
  v8::Context::Scope context_scope(context);

  v8::Handle<v8::Value> result = run_javascript(isolate, context, code);
  v8::String::Utf8Value utf8(isolate, result);
  printf("script result: %s\n", *utf8);

  v8::Local<v8::ObjectTemplate> templ = v8::ObjectTemplate::New(isolate);
  v8::Local<v8::Object> dummy = templ->NewInstance(context).ToLocalChecked();
  CJavascriptObject wrapper(dummy);

  py::object pyres = wrapper.Wrap(result);
  std::cout << "Python result size: " << sizeof(pyres) << std::endl;
  return 0;
}

BOOST_PYTHON_MODULE(_SoirV8)
{
  CJavascriptException::Expose();
  CWrapper::Expose(); 
  CContext::Expose();
  CEngine::Expose();
}

#if PY_MAJOR_VERSION >= 3
#   define INIT_MODULE PyInit_SoirV8
    extern "C" PyObject* INIT_MODULE();
#else
#   define INIT_MODULE init_SoirV8
    extern "C" void INIT_MODULE();
#endif

int main__run_code(int argc, char* argv[])
{
  std::cout << "Init python" << std::endl;
  PyImport_AppendInittab((char*)"_SoirV8", INIT_MODULE);
  Py_Initialize();
  PyDateTime_IMPORT;

  std::cout << "Argv0: " << argv[0] << std::endl;

  const char* code =
    "print('Importing')\n"
    "import _SoirV8\n"
    "x = _SoirV8.JSNull()\n"
    "print(x)\n"
    //"y = _SoirV8.JSIsolate(False,'./port')\n"
    "y = _SoirV8.JSIsolate()\n"
    "print('Hot dang, an isolate: {}'.format(y))\n"
    "y.enter()\n"
    "print('Entered')\n"
    "z = _SoirV8.JSContext()\n"
    "print('Hotter dang, a context: {}'.format(z))\n"
    "result = 42"
    ;

  try {
    py::object pyvar = run_python_code(code);
  } catch (py::error_already_set& e) {
    PyErr_PrintEx(0);
    return 1;
  }

  return 0;
}

int main(int argc, char* argv[])
{
  std::cout << "Init python" << std::endl;
  PyImport_AppendInittab((char*)"_SoirV8", INIT_MODULE);
  Py_Initialize();
  PyDateTime_IMPORT;

  std::cout << "Run code file" << std::endl;
  try {
    py::object main_module = py::import("__main__");
    py::object main_namespace = main_module.attr("__dict__");
    py::exec_file("../tests/smalltest.py", main_namespace, main_namespace);
  } catch (py::error_already_set& e) {
    PyErr_PrintEx(0);
    return 1;
  }

  return 0;
}

