
#ifdef DEBUG
	#define _CRTDBG_MAP_ALLOC
	#include <crtdbg.h>
#endif // DEBUG

#include <stdio.h>
#include <stdlib.h>
#include "CLoginServer.h"

using namespace cov1013;

CLoginServer Server;

int main(void)
{
	Server.Run();

#ifdef DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	return 0;
}