
#ifdef DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif // DEBUG

#include <stdio.h>
#include <stdlib.h>
#include "MonitorServer_Internal.h"
#include "MonitorServer.h"

using namespace cov1013;

MonitorServer MonitorServer_Net;

int main(void)
{
	MonitorServer_Net.Run();

#ifdef DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	return 0;
}