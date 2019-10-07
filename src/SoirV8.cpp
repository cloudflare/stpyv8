#include "Config.h"
#include "Exception.h"
#include "Wrapper.h"
#include "Context.h"
#include "Engine.h"
#include "Locker.h"


BOOST_PYTHON_MODULE(_SoirV8)
{
  CJavascriptException::Expose();
  CWrapper::Expose();
  CContext::Expose();
  CEngine::Expose();
  CLocker::Expose();
}


#define INIT_MODULE PyInit__SoirV8
extern "C" PyObject* INIT_MODULE();
