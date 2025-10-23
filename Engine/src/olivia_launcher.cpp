#include "olivia/olivia.h"

int main(int argc, char* argv[])
{
	if (olivia::init("sample.dll"))
	{
		olivia::run();
	}

	return 0;
}