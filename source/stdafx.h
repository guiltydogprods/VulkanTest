// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// TODO: reference additional headers your program requires here

#pragma hdrstop

#if defined(WIN32)
#include <Windows.h>
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <stdint.h>
#include <string>

#include "Framework/LinearAllocator.h"
#include "Framework/ScopeStackAllocator.h"
#include "Framework/Application.h"
#include "Framework/File.h"
#include "Framework/System.h"
#include "Framework/Singleton.h"
