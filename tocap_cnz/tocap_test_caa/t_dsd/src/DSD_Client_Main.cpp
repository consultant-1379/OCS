

#include <iostream>
#include <string>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "DSD_Client.h"
#include "Menu.h"


using namespace std;

const char* USAGE = "t_tocap <ip_addr> <port>\n";
char g_buf[255];

bool operation(Menu::OPERATION op, DSD_Client& dsd_client, Menu& menu);

int main(int argc, char* argv[])
{
	if(argc == 3)
	{
		Menu menu;

		DSD_Client dsd_client(argv[1], argv[2]);

		while(operation(menu.run(), dsd_client, menu))
		{
			;
		}

	}
	else
		puts(USAGE);

	return 0;
}

bool operation(Menu::OPERATION op, DSD_Client& dsd_client, Menu& menu)
{

	memset(g_buf, 0, 255 * sizeof(char));

	switch(op)
	{
	case Menu::QUIT:
		return false;
	case Menu::PRINT_ERROR_CODES:
		//menu.printErrorCodes();
		break;
	case Menu::CONNECT:
		dsd_client.isConnected() ? dsd_client.disConnect() : dsd_client.connect();
		break;
	case Menu::SEND_ECHO:
		if(menu.promptEcho(g_buf))
		{
			if(dsd_client.isConnected())
			{
				dsd_client.sendEcho(g_buf[0]);
			}
			else
				puts("Not connected");
		}
		break;
	default:
		puts("Unknown operation requested.");
		break;
	}
	return true;
}
