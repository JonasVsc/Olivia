#include "olivia/olivia_platform.h"

namespace olivia
{
	input_state_t g_input_state{};

	void update_input_state()
	{
		memcpy(g_input_state.prev_keys, g_input_state.curr_keys, sizeof(g_input_state.curr_keys));
	}

} // olivia