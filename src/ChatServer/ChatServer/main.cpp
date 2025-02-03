#define _CRTDBG_MAP_ALLOC

#include <stdlib.h>
#include <stdio.h>
#include <crtdbg.h>

#include <ws2tcpip.h>
#include <winsock.h>
#include <mstcpip.h>
#include <windows.h>
#include <time.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <unordered_map>
#include <memory>
#include "LanClient.h"
#include "NetServer.h"

#include "ChatServer.h"

int main()
{
	std::unique_ptr<cov1013::ChatServer> server = std::make_unique<cov1013::ChatServer>();
	server->Run();

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	return 0;
}