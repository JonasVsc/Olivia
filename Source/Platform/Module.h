#pragma once

struct Allocator;
struct InputState;

typedef size_t (*PFN_GetStateSize)(void);
typedef void (*PFN_Init)(void*);
typedef void (*PFN_Update)(void*, InputState*, float);

class Module
{
public:

	bool Load(Allocator* allocator);

	void* memory;
	PFN_Init Func_Init;
	PFN_Update Func_Update;

private:

	void* handle;

};