
#ifdef DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif // DEBUG

#include <stdio.h>
#include <stdlib.h>
#include "CMonitorServer_Lan.h"
#include "CMonitorServer_Net.h"

using namespace cov1013;

CMonitorServer_Net MonitorServer_Net;

int main(void)
{
	MonitorServer_Net.Run();

#ifdef DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	return 0;
}