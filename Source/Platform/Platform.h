#pragma once

#ifdef OLIVIA_WIN32
	#include "Win32/Win32_Platform.h"
	using Platform = Win32Platform;
#else
	#error win32 only
#endif

