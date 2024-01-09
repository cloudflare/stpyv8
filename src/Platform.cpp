#include "libplatform/libplatform.h"

#include "Platform.h"

std::unique_ptr<v8::Platform> CPlatform::platform;
bool CPlatform::inited = false;

void CPlatform::Init()
{
    if(inited)
        return;

    v8::V8::InitializeICUDefaultLocation(argv.c_str(), GetICUDataFile());
    v8::V8::InitializeExternalStartupData(argv.c_str());

    platform = v8::platform::NewDefaultPlatform();

    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();

    inited = true;
}

const char * CPlatform::GetICUDataFile()
{
#if defined(_WIN32) || defined (_WIN64)
    boost::filesystem::path icu_data_path = getenv("APPDATA");
#else
    boost::filesystem::path icu_data_path = getenv("HOME");
#endif
    if (icu_data_user == nullptr)
        return nullptr;

    if (boost::filesystem::is_directory(icu_data_path)) {
        icu_data_path /= icu_data_user;

        std::string icu_data_path_str = icu_data_path.string();
        const char *icu_data_path_ptr = icu_data_path_str.c_str();

        std::ifstream ifile(icu_data_path_ptr);
        if (ifile.good())
            return icu_data_path_ptr;
    }

    if (icu_data_system != nullptr) {
#if defined(_WIN32) || defined (_WIN64)
        boost::filesystem::path icu_windows_data_path = getenv("PROGRAMDATA");
        if (boost::filesystem::is_directory(icu_windows_data_path)) {
            icu_windows_data_path /= icu_data_system;

            std::string icu_windows_data_path_str = icu_windows_data_path.string();
            const char *icu_windows_data_path_ptr = icu_windows_data_path_str.c_str();

            std::ifstream ifile(icu_windows_data_path_ptr);
            if (ifile.good())
                return icu_windows_data_path_ptr;
        }
#else
        std::ifstream ifile(icu_data_system);
        if (ifile.good())
            return icu_data_system;
#endif
    }

    return nullptr;
}
