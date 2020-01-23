#include "libplatform/libplatform.h"

#include "Platform.h"

std::unique_ptr<v8::Platform> CPlatform::platform;
bool CPlatform::inited = false;

void CPlatform::Init()
{
  if(inited) return;

  v8::V8::InitializeICUDefaultLocation(argv.c_str(), GetICUDataFile());
  v8::V8::InitializeExternalStartupData(argv.c_str());
  
  platform = v8::platform::NewDefaultPlatform();
  
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();
  
  inited = true;
}
