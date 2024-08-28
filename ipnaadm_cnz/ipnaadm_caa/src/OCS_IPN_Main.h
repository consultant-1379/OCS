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
// OCS_IPN_Main.h
//
// DESCRIPTION 
// This file contains local prototypes and definitions.
//
// DOCUMENT NO
// 3/190 55-CAA 109 0392
//
// AUTHOR 
// 0000912 EPA/D/U Michael Bentley
//
//******************************************************************************
// *** Revision history ***
// 0000803 MJB Created
//******************************************************************************
#ifndef OCS_IPN_Main_h
#define OCS_IPN_Main_h

#include "OCS_IPN_Global.h"
#include "OCS_IPN_Sig.h"

extern HANDLE hIpnaAdmMainThread;

//******************************************************************************
// Internal variables   
//******************************************************************************

#define MAXBUF 256
#define DATABUF_SIZE 65536            

#define LIST_BACKLOG 10                       // Backlog of connections for listen

char           databuf[DATABUF_SIZE];         // Buffer for transfering data
char                 LineBuf[MAXBUF];         // A buffer to receive commands 
int                  LineLen;                 // Length of line 
char                 MsgBuf[MAXBUF];          // Control socket message buffer
char                 LogMsg[MAXBUF];          // Place to store log messages

SOCKET               list_sock;               // Listening Socket
SOCKET               ctrl_sock;               // Control Socket
SOCKADDR_IN          s_control;               // Control socket address 
HANDLE               hListEvent;              // List socket events
HANDLE               hCtrlEvent;              // Control socket events
HANDLE hMainObjectArray[NO_OF_EVENTS_MAIN];   // Array of Main Obj. handles
HANDLE               ThreadTermEvent;         // Thread terminate event
HANDLE               ipna_replied_event;      // IPNA reply event
HANDLE          MultiConTermEvent[MAX_IPNAS]; // Kill multiple ipna connection event
HANDLE               MultiCmdTermEvent;       // Event for managing multiple command connections
int					 foo;					  // Pointer into hThreads
HANDLE				 hThreads[50];			  // ALl thread handles are here gathered


// Parameters to threads are passed via a void pointer. 
// This is a structure into which parameters are placed
// 

struct thread_param 
{
   SOCKET thread_sock;
};

// Tracking items for cleanup

typedef enum tracking_items
{
  ListenSocket,                          
  Max_Tracking_Item
};

int Tracking[Max_Tracking_Item];              // Array for tracking things

// Globals for threads

int    command_connected;                     // Indication of command handler connection
int    Command_Data[MAX_IPNAS];               // Command data from command handler
int    Reply_Data[MAX_IPNAS];                 // Reply data to command handler
int    connected[MAX_IPNAS];                  // IPNA connection array
bool   Replied[MAX_IPNAS];                    // IPNA replied indication array
HANDLE Command_Events[MAX_IPNAS];             // Command events from command handler
u_long link_IP[2];


// Specials for debug

HANDLE debug_file_handle;           // Debug file handle
HANDLE debug_console_handle;        // Debug console window handle
HANDLE debug_stderr_handle;         // Debug console window handle

bool debug_file_active;             // Debug messages goto debug file, if true

// Specials for connection counting

long   no_conns;                    // Number of connections made since service started
int    curr_cons;                   // Number of current connections

CRITICAL_SECTION connsec;           // Critical section for changing curr_cons
CRITICAL_SECTION ipnasec;           // Critical section for changing globals handling connections

//******************************************************************************
// OCS_IPN_Main prototypes
//******************************************************************************

void resetEvents(void);


//******************************************************************************
//
// put_buffer
//
// Put data in buffer for IPNA Fucntion Change function
//
//******************************************************************************

void put_buffer(char* buf, DWORD len, char* bootcontents_out);



//******************************************************************************
//
// prepareFunctionChange
//
// Prepare for IPNA Fucntion Change
//
//******************************************************************************

DWORD prepareFunctionChange(char ipnx);



//******************************************************************************
//
// rollBackFunctionChange
//
// Roolback of IPNA Fucntion Change files in AP
//
//******************************************************************************

DWORD rollBackFunctionChange(char ipnx);



//******************************************************************************
// terminateMain
//
// terminate current file transfer. 
//
// INPUT:  error code
// OUTPUT: -
// OTHER:  -
//******************************************************************************

void terminateMain(char *, DWORD);



//******************************************************************************
// cleanup
//
// Clean up things set up during RETR command
//
// INPUT:  -
// OUTPUT: -
// OTHER:  -
//******************************************************************************

void cleanup(void);

//******************************************************************************
// DoMsg
//
// Handle error messages
//
// INPUT:  -
// OUTPUT: -
// OTHER:  -
//******************************************************************************

void DoMsg(char *);

#endif
