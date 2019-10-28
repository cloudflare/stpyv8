#include "libplatform/libplatform.h"

#include "Platform.h"


void CPlatform::Init()
{
  if(inited) return;

  v8::V8::InitializeICUDefaultLocation(argv.c_str());
  v8::V8::InitializeExternalStartupData(argv.c_str());
  
  platform = v8::platform::NewDefaultPlatform();
  
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();
  
  inited = true;
}
