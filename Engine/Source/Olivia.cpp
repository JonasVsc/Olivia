#include "Olivia/Olivia.h"

int main(int argc, char* argv[])
{
	Olivia::Engine eng("Olivia Engine", 640, 480);

	try
	{
		eng.run();
	}
	catch (std::exception& e)
	{
		printf("%s\n", e.what());
	}

	return 0;
}