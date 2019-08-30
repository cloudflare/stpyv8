// PyV8.cpp : Defines the entry point for the DLL application.
//
#include "Config.h"
#include "Engine.h"
#include "Debug.h"
#include "Locker.h"

#ifdef SUPPORT_AST
  #include "AST.h"
#endif

BOOST_PYTHON_MODULE(_PyV8)
{
  CJavascriptException::Expose();
  CWrapper::Expose(); 
  CContext::Expose();
#ifdef SUPPORT_AST
  CAstNode::Expose();
#endif
  CEngine::Expose();
  CDebug::Expose();  
  CLocker::Expose();
}
