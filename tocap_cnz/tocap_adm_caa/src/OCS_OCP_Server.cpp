//******************************************************************************
// NAME
// OCS_OCP_server.cpp
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
// This class is use for presenting the OCP server.

// LATEST MODIFICATION
// -
//******************************************************************************

#include "OCS_OCP_Server.h"
#include "OCS_OCP_Service.h"
#include "OCS_OCP_Trace.h"
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

using namespace std;


// Socket port used by CP to communicate with frontend AP
uint16_t OCS_OCP_Server::s_tocapCPPort = getServerPort("tocapd_cp","tcp", TOCAP::TOCAP_CP);
// Socket port used by non-frontend AP to communicate with frontend AP
uint16_t OCS_OCP_Server::s_tocapFEPort = getServerPort("tocapd_fe","udp", TOCAP::TOCAP_FE);;
// Socket port used by frontend AP to communicate with non-frontend AP
uint16_t OCS_OCP_Server::s_tocapNFEPort = getServerPort("tocapd_nfe","tcp", TOCAP::TOCAP_NFE);;

/*===================================================================
   ROUTINE: OCS_OCP_Server
=================================================================== */
OCS_OCP_Server::OCS_OCP_Server()
: m_pOcsOCPService(0)
{
	m_frontEndAP = 0;
	m_nonFrontEndAP = 0;
	m_serviceStop = false;
	m_runningMode = 1;

} /* end OCS_OCP_Server */


/*===================================================================
   ROUTINE: OCS_OCP_Server
=================================================================== */
OCS_OCP_Server::OCS_OCP_Server(OCS_OCP_Service* pOcsOCPService)
: m_pOcsOCPService(pOcsOCPService)
{
    m_frontEndAP = 0;
    m_nonFrontEndAP = 0;
    m_serviceStop = false;
    m_runningMode = 1;

} /* end OCS_OCP_Server */

/*===================================================================
   ROUTINE: ~OCS_OCP_Server
=================================================================== */
OCS_OCP_Server::~OCS_OCP_Server()
{
	if(m_frontEndAP)
	{
		delete m_frontEndAP;
	}

	if(m_nonFrontEndAP)
	{
		delete m_nonFrontEndAP;
	}

} /* end ~OCS_OCP_Server */

/*===================================================================
   ROUTINE: run
=================================================================== */
int OCS_OCP_Server::run()
{

	TOCAP::RetCodeValue rc=TOCAP::ANOTHER_TRY;

	while ((rc==TOCAP::ANOTHER_TRY || rc==TOCAP::TIMEOUT_READ) && !m_serviceStop)
	{
		bool cpSystem=false;
		if (isMultipleCpSystem(cpSystem) == false)
		{
			::sleep(1);
			continue;
		}

		string thisNodeName="";
		bool frontEndSystem=false;
		try
		{
			if (isFrontEndActiveApNode(thisNodeName,frontEndSystem))
			{
				m_frontEndAP = new OCS_OCP_frontEndAP(cpSystem,thisNodeName);
				rc = m_frontEndAP->run();
			}
			else
			{
				m_nonFrontEndAP = new OCS_OCP_nonFrontEndAP(cpSystem,frontEndSystem);
				rc  = m_nonFrontEndAP->run();
			}
		}
		catch(...)
		{
			// Set new rc here.....
			if(m_frontEndAP)
			{
			    delete m_frontEndAP;
				m_frontEndAP = 0;
			}

			if(m_nonFrontEndAP)
			{
			    delete m_nonFrontEndAP;
				m_nonFrontEndAP = 0;
			}

			break;
		}

		if(m_frontEndAP)
		{
		    delete m_frontEndAP;
			m_frontEndAP = 0;
		}

		if(m_nonFrontEndAP)
		{
		    delete m_nonFrontEndAP;
			m_nonFrontEndAP = 0;
		}

	}

	// Report error to AMF framework if the server failed
	if((this->m_pOcsOCPService != 0) && (!m_serviceStop))
	    m_pOcsOCPService->componentReportError(ACS_APGCC_COMPONENT_RESTART);

	// Kill this process when running in noservice mode.
	if(this->m_runningMode == 2)
	{
	    kill(getpid(),SIGTERM);
	}

	return rc;
} /* end run */


/*===================================================================
   ROUTINE: stop
=================================================================== */
void OCS_OCP_Server::stop()
 {
    if(m_frontEndAP != 0)
    {
        m_frontEndAP->stop();
    }

    if(m_nonFrontEndAP != 0)
    {
        m_nonFrontEndAP->stop();
    }

    m_serviceStop = true;

 } /* end stop */


void OCS_OCP_Server::setRunningMode(int mode)
{
    this->m_runningMode = mode;
}

//----------------------------------------------------------------------------
//
//  COPYRIGHT Ericsson AB 2001-2004
//
//  The copyright to the computer program(s) herein is the property of
//  ERICSSON AB, Sweden. The programs may be used and/or copied only
//  with the written permission from ERICSSON AB or in accordance with
//  the terms and conditions stipulated in the agreement/contract under
//  which the program(s) have been supplied.
//
//----------------------------------------------------------------------------
