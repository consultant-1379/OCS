///******************************************************************************
// NAME
// OCS_OCP_CheckNodeState.cpp
//
// COPYRIGHT Ericsson AB, Sweden 2013.
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
// 190 89-CAA 109 1544

// AUTHOR
// 2013-08-27 by XDT/DEK XTUANGU

// DESCRIPTION
// This file contains class implementation to check AP node state.



#include "OCS_OCP_CheckAPNodeState.h"
#include "OCS_OCP_frontEndAP.h"
#include "OCS_OCP_nonFrontEndAP.h"
#include "OCS_OCP_Trace.h"

#include <sstream>
#include <fstream>
#include <iostream>


using namespace std;


Event OCS_OCP_CheckAPNodeState::s_stopEvent;

OCS_OCP_CheckAPNodeState::OCS_OCP_CheckAPNodeState():m_apNodeState(ERROR)
{
    OCS_OCP_CheckAPNodeState::s_stopEvent.resetEvent();
}

OCS_OCP_CheckAPNodeState::~OCS_OCP_CheckAPNodeState()
{

}

void OCS_OCP_CheckAPNodeState::run()
{
    newTRACE((LOG_LEVEL_INFO,"OCS_OCP_CheckAPNodeState::run()",0));

    int maxfd = 0;
    fd_set readset, allset;

    // Get AP state
    m_apNodeState = getAPNodeState();

    //Set allset to zero
    FD_ZERO(&allset);

    // Add stop event to allset
    FD_SET(OCS_OCP_CheckAPNodeState::s_stopEvent.getFd(), &allset);
    maxfd = OCS_OCP_CheckAPNodeState::s_stopEvent.getFd();

    int m_stop = false;

    timeval selectTimeout;
    selectTimeout.tv_sec = 10;
    selectTimeout.tv_usec = 0;

    AP_Node_State currentApState;

    while(!m_stop)
    {
        readset = allset;

        int select_return = ::select(maxfd+1,&readset, NULL, NULL, &selectTimeout);

        selectTimeout.tv_sec = 10;
        selectTimeout.tv_usec = 0;

        if(select_return == 0)//timeout
        {

            currentApState = getAPNodeState();

            if((m_apNodeState == ACTIVE) && (currentApState == PASSIVE))
            {
                m_apNodeState = currentApState;

                TRACE((LOG_LEVEL_INFO, "Set ap node state event to PASSIVE",0));

                // Signal AP state change...
                OCS_OCP_frontEndAP::setAPNodeStateEvent();
            }
            else if ((m_apNodeState == PASSIVE || m_apNodeState == ERROR) && (currentApState == ACTIVE))
            {
                m_apNodeState = currentApState;

                TRACE((LOG_LEVEL_INFO, "Set ap state change event ACTIVE",0));
               // Signal AP state change...
                OCS_OCP_nonFrontEndAP::setAPNodeStateEvent();
            }

        }
        else if(select_return == -1)// error
        {

        }
        else //Handle is signed.
        {
            if(FD_ISSET(OCS_OCP_CheckAPNodeState::s_stopEvent.getFd(),&readset))
            {
                OCS_OCP_CheckAPNodeState::s_stopEvent.resetEvent();
                m_stop = true;

                TRACE((LOG_LEVEL_INFO, "Check node state: receive stop event",0));
            }
        }
    }// End while
}

void OCS_OCP_CheckAPNodeState::stop()
{
    OCS_OCP_CheckAPNodeState::s_stopEvent.setEvent();
}
