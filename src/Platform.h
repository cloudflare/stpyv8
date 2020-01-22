#pragma once

#include <fstream>
#include <v8.h>

#include "Config.h"


class CPlatform
{
private:
  static bool inited;
  static std::unique_ptr<v8::Platform> platform;
  constexpr static const char *icu_data = ICU_DATA;

  const char *GetICUDataFile()
  {
    if (icu_data == nullptr) return nullptr;

    std::ifstream ifile(icu_data);
    if (ifile) return icu_data;

    return nullptr;
  }

  std::string argv;
public:
  CPlatform() : argv(std::string()) {};
  CPlatform(std::string argv0) : argv(argv0) {};
  ~CPlatform() {};
  void Init();
};
