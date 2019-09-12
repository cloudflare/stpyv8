
#include "Config.h"
#include "Exception.h"
#include "Wrapper.h"
#include "Context.h"
#include "Engine.h"

BOOST_PYTHON_MODULE(_SoirV8)
{
  CJavascriptException::Expose();
  CWrapper::Expose(); 
  CContext::Expose();
  CEngine::Expose();
}
