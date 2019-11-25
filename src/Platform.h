#pragma once

#include <v8.h>
#include <boost/shared_ptr.hpp>

class CPlatform
{
private:
  static bool inited;
  static std::unique_ptr<v8::Platform> platform;

  std::string argv;

public:
  CPlatform() : argv(std::string()) {};
  CPlatform(std::string argv0) : argv(argv0) {};
  ~CPlatform() {};
  void Init();
};
