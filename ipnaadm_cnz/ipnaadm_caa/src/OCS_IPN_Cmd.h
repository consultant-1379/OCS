
//******************************************************************************
// COPYRIGHT Ericsson Utvecklings AB, Sweden 2000.
// All rights reserved.
//
// The Copyright to the computer program(s) herein 
// is the property of Ericsson Utvecklings AB, Sweden.
// The program(s) may be used and/or copied only with 
// the written permission from Ericsson Utvecklings AB or in 
// accordance with the terms and conditions stipulated in the 
// agreement/contract under which the program(s) have been 
// supplied.
//
// NAME
// OCS_IPN_Thread.h
//
// DESCRIPTION 
// This file contains local prototypes and definitions.
//
// DOCUMENT NO
// 9/190 55-CAA 109 0392
//
// AUTHOR 
// 000927 EPA/D/U Michael Bentley
//
//******************************************************************************
// *** Revision history ***
// 000927 MJB Created
//******************************************************************************

#ifndef OCS_IPN_cmd_h
#define OCS_IPN_cmd_h

#include "OCS_IPN_Sig.h"          // Signal definitions

/*****************************************************************************/

// Global variables

char     rec_buf[16];                     // Buffer big enough to receive signal head
char     send_buf[100];                   // Small send buffer

SOCKET   cmd_sock;                        // Command Socket
HANDLE   CmdEvent;                        // Command socket events

#define  ADM_PORT_NUMBER        2346      // Server port no.
#define  MAX_IPNAS                64      // Maxmium number of IPNAS

#define  MAXRETRY               1000      // Maximum number of retries of blocked recv
#define  CmdTimeout            60000      // 1 minute timeout for receiving command replies

WSANETWORKEVENTS    cmd_events;           // Command socket events

//******************************************************************************
// Function Prototypes
//******************************************************************************

//******************************************************************************
// 
// usage
// 
// print out some help
//

//******************************************************************************

void usage(void);

//******************************************************************************
// 
// server_connect
// 
// Connect to the ipnaadm server
//
//******************************************************************************

void server_connect(void);

//******************************************************************************
// makeitsend
//
// Send a signal and make sure it's all sent
//
//******************************************************************************

bool makeitsend(SOCKET its_sock,char *its_buf,int its_len, char *its_name);


//******************************************************************************
// recvitall
//
// Receive multiple bytes from a socket and ensure it is all received
//
//******************************************************************************

bool recvitall(SOCKET its_sock,char *its_buf,int its_len,char *its_name);

//******************************************************************************
// my_exit
//
// Clean up and die
//
// 
//******************************************************************************


void my_exit(int err_code, SOCKET my_sock);

//******************************************************************************
// rollBackFunctionChange(char ipnx)
//
// Roll back status of IPNA software configuration files
//
// 
//******************************************************************************


DWORD rollBackFunctionChange(char ipnx);

//******************************************************************************
// checkNodeState(void)
//
// Check whether I am executing in Active node
//
// 
//******************************************************************************


unsigned short checkNodeState(unsigned short &node);

//******************************************************************************
// print_signal
//
// print the signal pointed at by signal
//
// 
//******************************************************************************

void print_signal(void);

#endif
