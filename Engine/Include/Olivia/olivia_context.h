#pragma once
#include "olivia/olivia_core.h"

namespace olivia
{
	OLIVIA_DEFINE_HANDLE(context);
	
	bool init(const char* game_dll);

	void run();
}