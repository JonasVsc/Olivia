#pragma once
#include <Windows.h>
#include <cstdint>
#include <cstddef>
#include <cassert>
#include <cstdio>

namespace Olivia { struct API; }

typedef void(*DLL_Init)(Olivia::API* api, void*);
typedef void(*DLL_Update)(float dt);
typedef size_t(*DLL_GetStateSize)(void);

namespace Olivia
{

	struct Allocator
	{
		uint8_t* start{ nullptr };
		size_t size{ 0 };
		size_t offset{ 0 };
	};

	struct Window
	{
		void* handle{ nullptr };
		uint32_t width{ 0 };
		uint32_t height{ 0 };
	};

	struct Input
	{
		bool curr_keys[256]{};
		bool prev_keys[256]{};
	};

	struct DLL
	{
		void* handle;
		void* memory;
		const char* file;
		DLL_Update Update;
	};

	Allocator CreateAllocator(size_t size);

	void* Allocate(Allocator* allocator, size_t size, size_t alignment = alignof(std::max_align_t));

	void FreeAllocator(Allocator* allocator);

	void EnableConsole();

	Window CreateWindowR(const char* title, uint32_t w, uint32_t h);

	void DestroyWindowR(Window* window);

	Input* GetInput();

	void UpdateInput();

	DLL LoadDLL(Allocator* allocator, API* api);

	void ReloadDLL(API* api, DLL* dll);

	float UpdateFrame();

	inline bool IsKeyDown(int32_t k)
	{ 
		Input* input = GetInput();
		return (k >= 0 && k < 256) ? input->curr_keys[k] : false;
	}

	inline bool IsKeyPressed(int32_t k) 
	{
		Input* input = GetInput();
		return (k >= 0 && k < 256) ? (input->curr_keys[k] && !input->prev_keys[k]) : false;
	}

}