//******************************************************************************
// NAME
// OCS_OCP_svcMain.cpp
//
// COPYRIGHT Ericsson AB, Sweden 2011.
// All rights reserved.
//
// The Copyright to the computer program(s) herein
// is the property of Ericsson AB, Sweden.
// The program(s) may be used and/or copied only with
// the written permission from Ericsson AB or in
// accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been
// supplied.

// DESCRIPTION
// This file is used for the ipn server class implementation.
//
// DOCUMENT NO
// 190 89-CAA 109 1405
//
// AUTHOR
// XDT/DEK XTUANGU
//
//******************************************************************************
// *** Revision history ***
// 2011-07-14 Created by XTUANGU
//******************************************************************************
#include "OCS_IPN_Trace.h"
#include "OCS_IPN_Service.h"
#include "OCS_IPN_Server.h"
#include "OCS_IPN_Common.h"
#include "ACS_APGCC_AmfTypes.h"

#include <iostream>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <string>
#include <signal.h>


using namespace std;


const char* NOSERVICE = "noservice";
const int FAILED = -100;

void help();
int run_as_console();
int run_as_service();

/****************************************************************************
 * Method:	main()
 * Description: main entry to start the server
 * Param [in]: 
 * Param [out]:
 * Return: error code
 *****************************************************************************
 */
int main(int argc, char* argv[])
{

    int retVal = 0;

	// ------------------------------------------------------------------------
	// Parsing command line
	// ------------------------------------------------------------------------

	int opt;
	bool helpOpt = false;
	opterr = 0;
	while((opt = getopt(argc, argv, "h")) != -1) {
		switch(opt) {
		case 'h':
		case '?':
			helpOpt = true;
			break;
		default:;
		}
	}

	if (helpOpt)
	{
		help();
		return FAILED;
	}

	if (argc == 1)
	{
		retVal = run_as_service();
	}
	else if (argc == 2)
	{
		string t(argv[1]);
		transform(t.begin(), t.end(), t.begin(),(int (*)(int)) ::tolower);
		if (t == NOSERVICE)
		{
			retVal = run_as_console();
		}
		else
		{
			help();
			retVal = FAILED;
		}
	}
	else
	{
		help();
		retVal = FAILED;
	}

	return retVal;
}

/****************************************************************************
 * Method:	run_as_console()
 * Description: run server in noservice mode
 * Param [in]: N/A
 * Param [out]: N/A
 * Return: error code
 *****************************************************************************
 */
int run_as_console()
{
    // No TRACE shall be invoked before this point
    initTRACE();
    newTRACE((LOG_LEVEL_INFO, "run_as_console()",0));

	try
	{
		// Block all signals for background thread.
		sigset_t new_mask;
		sigfillset(&new_mask);
		sigset_t old_mask;
		pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

		TRACE((LOG_LEVEL_INFO, "Server started.",0));


		OCS_IPN_Server ocs_ipn_server;
		ocs_ipn_server.setRunningMode(2); // run with noservice
		boost::thread ocs_ipn_server_thread(boost::bind(&OCS_IPN_Server::start, &ocs_ipn_server));

		OCS_IPN_CpRelatedSwManagerOI cprelatedswmanageroi(OCS_IPN_Common::IMM_CPRELATEDMANAGER_IMPL_NAME);
        boost::thread cprelatedswmanageroi_thread(boost::bind(&OCS_IPN_CpRelatedSwManagerOI::run, &cprelatedswmanageroi));

		// Restore previous signals.
		pthread_sigmask(SIG_SETMASK, &old_mask, 0);

		// Wait for signal indicating time to shut down.
		sigset_t wait_mask;
		sigemptyset(&wait_mask);
		sigaddset(&wait_mask, SIGINT);
		sigaddset(&wait_mask, SIGQUIT);
		sigaddset(&wait_mask, SIGTERM);
		pthread_sigmask(SIG_BLOCK, &wait_mask, 0);

		int sig = 0;
		sigwait(&wait_mask, &sig);

		//Stop OCP IPN server & OI

		cprelatedswmanageroi.stop();
		ocs_ipn_server.stop();

		cprelatedswmanageroi_thread.join();
		ocs_ipn_server_thread.join();

		TRACE((LOG_LEVEL_INFO, "Server stopped.",0));

	}
	catch (std::exception& e)
	{
	    TRACE((LOG_LEVEL_ERROR, "%s", 0, e.what()));
	}

	termTRACE();

	return 0;
}


/****************************************************************************
 * Method:	run_as_service()
 * Description: run server in AMF service mode
 * Param [in]: N/A
 * Param [out]: N/A
 * Return: error code
 *****************************************************************************
 */
int run_as_service()
{
  	ACS_APGCC_HA_ReturnType retCode = ACS_APGCC_HA_SUCCESS;

    // Construct server
    OCS_IPN_Service *serviceObj = new OCS_IPN_Service("ocs_ipnaadmd", "root");

     // No TRACE shall be invoked before this point
    initTRACE();
    newTRACE((LOG_LEVEL_INFO, "Start IPNAADM server as service.",0));

	try
	{
		retCode = serviceObj->amfInitialize();

	}
	catch (...)
	{
	    TRACE((LOG_LEVEL_ERROR, "Exception. Exit IPNAADM server.",0));
	    return FAILED;
	}

	TRACE((LOG_LEVEL_INFO, "Exit IPNAADM server.",0));

	termTRACE();

	return retCode;
}


/****************************************************************************
 * Method:	help()
 * Description: show help
 * Param [in]: 
 * Param [out]:
 * Return: error code
 *****************************************************************************
 */
void help()
{
    std::cout << "Usage: " << "ocs_ipnaadmd" << " [noservice]" << std::endl;
}
