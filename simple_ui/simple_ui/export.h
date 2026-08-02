#pragma once

#ifdef WINDOW_EXPORTS
#define SIMPLE_UI_API __declspec(dllexport)
#else
#define SIMPLE_UI_API __declspec(dllimport)
#endif
