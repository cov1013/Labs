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
#include "CLanClient.h"
#include "CNetServer.h"

#include "ChatServer.h"

void main()
{
	cov1013::ChatServer* p_server = new cov1013::ChatServer;
	p_server->Run();

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
}