#include "TCP_Client.h"
#include "Menu.h"
#include "OCS_OCP_events.h"
#include "OCS_OCP_protHandler.h"
#include "DSD_Client.h"


#include <iostream>
#include <string>
#include <boost/thread.hpp>
#include <boost/bind.hpp>


using namespace std;

const char* USAGE = "t_tocap e \n";
const char* USAGE1 = "t_tocap p \n";

char g_buf[255];

bool operation(Menu::OPERATION op, DSD_Client& dsd_client, Menu& menu);
bool udp_operation(Menu::OPERATION op, ProtHandler& prot, Menu& menu);

int main(int argc, char* argv[])
{
	if((argc == 2)&& (string(argv[1]) == "e"))
	{
		Menu menu;

		DSD_Client dsd_client;
		while(operation(menu.run(), dsd_client, menu))
		{
			;
		}
	}
	if ((argc == 2)&& (string(argv[1]) == "p"))
	{
		Menu menu;
		TOCAP_Events* evRep = TOCAP_Events::getInstance();
		ProtHandler prot(evRep);
		if (ProtHandler::createUDPsock() == false)
			cout << " Create udp socket failed" << endl;


		while(udp_operation(menu.run_udp(), prot, menu))
		{
			;
		}

	}
	else
		puts(USAGE);
		puts(USAGE1);

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

bool udp_operation(Menu::OPERATION op, ProtHandler& prot, Menu& menu)
{
	memset(g_buf, 0, 255 * sizeof(char));
	char sstate;
	uint32_t apip, apip1, apip2;
	uint32_t cpip;
	uint32_t destip;

		switch(op)
		{
		case Menu::QUIT:
			return false;
		case Menu::PRINT_ERROR_CODES:
			//menu.printErrorCodes();
			break;
		case Menu::SEND_PRIMITIVE_11:
			if(menu.promptPrimitive11(sstate, apip, cpip, destip))
			{
				if(prot.send_11(sstate,apip,cpip,destip))
					cout << "Sent primitive 11 successfully" << endl;
				else
					cout << "Sent primitive 11 failed" << endl;
			}
			break;
		case Menu::SEND_PRIMITIVE_12:
			break;
		case Menu::SEND_PRIMITIVE_13:
			if(menu.promptPrimitive13(apip1, apip2, destip))
			{
				if(prot.send_13(apip1, apip2, destip))
					cout << "Sent primitive 13 successfully" << endl;
				else
					cout << "Sent primitive 13 failed" << endl;

			}
			break;
		case Menu::CONNECT_TO_AP:
			if(menu.connectToAP())
				cout << "Connect to AP successfully" << endl;
			else
				cout << "Connect to AP failed" << endl;
			break;
		default:
			puts("Unknown operation requested.");
			break;
		}
		return true;

}
