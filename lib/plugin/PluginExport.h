#pragma once
#ifdef _WIN32
#    define PLUGIN_API extern "C" __declspec(dllexport)
#else
#    define PLUGIN_API extern "C"
#endif