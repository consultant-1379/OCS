///******************************************************************************
// NAME
// OCS_OCP_CheckNodeState.h
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
// This file contains class declaration to check AP node state.

#ifndef OCS_OCP_CHECKAPNODESTATE_H_
#define OCS_OCP_CHECKAPNODESTATE_H_

#include "OCS_OCP_CSfunctions.h"
#include "Event.h"


class OCS_OCP_CheckAPNodeState
{
public:
    OCS_OCP_CheckAPNodeState();
    ~OCS_OCP_CheckAPNodeState();

    void run();
    void stop();

private:

    AP_Node_State m_apNodeState;
    static Event s_stopEvent;

};

#endif /* OCS_OCP_CHECKAPNODESTATE_H_ */
