//******************************************************************************
// NAME
// OCS_OCP_svcMain.cpp
//
// COPYRIGHT Ericsson AB, Sweden 2010.
// All rights reserved.
//
// The Copyright to the computer program(s) herein
// is the property of Ericsson AB, Sweden.
// The program(s) may be used and/or copied only with
// the written permission from Ericsson AB or in
// accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been
// supplied.

// DOCUMENT NO
// ???

// AUTHOR
// 2010-12-01 by XDT/DEK XTUANGU

// DESCRIPTION
// This file is main entry for this block.

// LATEST MODIFICATION
// -
//******************************************************************************

#include "OCS_OCP_Trace.h"
#include "OCS_OCP_Service.h"
#include "OCS_OCP_Server.h"
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

int run_as_console()
{
    // No TRACE shall be invoked before this point
    initTRACE();

    newTRACE((LOG_LEVEL_INFO,"run_as_console()",0));

	try
    {
        // Block all signals for background thread.
        sigset_t new_mask;
        sigfillset(&new_mask);
        sigset_t old_mask;
        pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

        //Start OCP server
        TRACE((LOG_LEVEL_INFO, "Server started.",0));


        OCS_OCP_Server ocs_ocp_server;
        ocs_ocp_server.setRunningMode(2); // run with noservice
        boost::thread ocs_ocp_thread(boost::bind(&OCS_OCP_Server::run, &ocs_ocp_server));

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

       //Stop OCP server
        ocs_ocp_server.stop();

        ocs_ocp_thread.join();

        TRACE((LOG_LEVEL_INFO, "Server stopped.",0));

    }
    catch (std::exception& e)
    {
    		ostringstream trace;
			trace << e.what() << endl;
			TRACE((LOG_LEVEL_ERROR, "%s",0,trace.str().c_str()));
    }

    termTRACE();

    return 0;
}

int run_as_service()
{

	ACS_APGCC_HA_ReturnType retCode = ACS_APGCC_HA_SUCCESS;

	// Construct server
    OCS_OCP_Service *serviceObj = new OCS_OCP_Service("ocs_tocapd", "root");

     // No TRACE shall be invoked before this point
    initTRACE();

    newTRACE((LOG_LEVEL_INFO, "run_as_service()", 0));

	try
	    {
			retCode = serviceObj->amfInitialize();

	    }
	    catch (...)
	    {
            TRACE((LOG_LEVEL_ERROR, "Exception. Exit tocap.",0));

			return FAILED;
	    }

        TRACE((LOG_LEVEL_INFO, "Exit tocap.",0));

        termTRACE();

        return retCode;
}

void help()
{
    std::cout << "Usage: " << "ocs_tocapd" << " [noservice]" << std::endl;
}
