#pragma once

#ifdef AIM_BUILD
    #define AIM_API __declspec(dllexport)
#else
    #define AIM_API __declspec(dllimport)
#endif