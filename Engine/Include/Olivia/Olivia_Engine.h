#pragma once
#include "Olivia/Olivia_Core.h"
#include "Olivia/Olivia_Graphics.h"
#include "Olivia/Olivia_Runtime.h"

namespace Olivia
{
	class Engine
	{
	public:
		
		Engine(const char* title, int32_t width, int32_t height);
		Engine(const Engine&) = delete;
		~Engine();

		void run();

	private:
		bool           m_running{};
		SDL_Window*    m_window{};
		Renderer*      m_renderer{};
	};
} // Olivia