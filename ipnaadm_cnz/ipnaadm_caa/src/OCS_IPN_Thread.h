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
// 7/190 55-CAA 109 0392
//
// AUTHOR 
// 0000912 EPA/D/U Michael Bentley
//
//******************************************************************************
// *** Revision history ***
// 0000912 MJB Created
//******************************************************************************

#ifndef OCS_IPN_Thread_h
#define OCS_IPN_Thread_h

#include "OCS_IPN_Global.h"
#include "OCS_IPN_Sig.h"

// Parameters to threads are passed via a void pointer. 
// This is a structure into which parameters are placed
// 

struct thread_param 
{
   SOCKET thread_sock;
};

#define NO_OF_THREAD_EVENTS 5     

#define THREADTERMEVENT   0                     // Thread tremination event from main
#define MULTICONTERMEVENT 1                     // Multi-connection termination event
#define THREADSOCKEVENTS  2                     // Network events from socket
#define COMMANDEVENT      3                     // Command event from event handler
#define REPLYEVENT        4                     // Reply event from IPNAs 


#define SENDBUFSIZE  65536
#define DBUFSIZE     65536                     // Size of data buffer 

#define MAXRETRY     10000                     // Maximum number of retries on recv

// #define QUICKTIMEOUT

#ifdef QUICKTIMEOUT
   #define HEARTBEAT_TIMEOUT  10000            // 10 secs for debugging
   #define CON_TIMEOUT        15000            // 15 sec. for debugging
#else
   #define HEARTBEAT_TIMEOUT  60000            // Timeout for receiving heartbeat is 1 minute
   #define CON_TIMEOUT       300000            // 5 minute timeout for IPNA initial connection
#endif

// Global variables

extern HANDLE ThreadTermEvent;
extern int    command_connected;               // Indication of command handler connection
extern int    Command_Data[MAX_IPNAS];         // Command data from command handler
extern int    Reply_Data[MAX_IPNAS];           // Reply data to command handler
extern bool   Replied[MAX_IPNAS];              // Reply events
extern int    connected[MAX_IPNAS];            // IPNA connection array
extern HANDLE Command_Events[MAX_IPNAS];       // Command events from command handler
extern HANDLE ipna_replied_event;              // Reply event to command handler
extern HANDLE MultiConTermEvent[MAX_IPNAS];    // Events for killing multiple IPNA connections
extern HANDLE MultiCmdTermEvent;               // Event for killing multiple command connections

extern char LogMsg[];                          // Place for log messages

// Specials for debugging
 
//#define DEBUG_FILE_PATH "C:\\ipnaadm"
//#define DEBUG_FILE_NAME "c:\\ipnaadm\\ipnaadm.debuglog"
 
//extern HANDLE debug_file_handle;           // Debug file handle
//extern HANDLE debug_console_handle;        // Debug console window handle
//extern HANDLE debug_stderr_handle;   

extern bool debug_file_active;             // Debug messages goto debug file, if true
extern bool debug_console_active;          // Debug messages goto debug console window, if true

extern long   no_conns;                    // Number of connections made since service started
extern int    curr_cons;                   // Number of current connections

CRITICAL_SECTION critsec;
extern CRITICAL_SECTION connsec;           // Critical section for changing curr_cons
extern CRITICAL_SECTION ipnasec;

struct thread_data                         // Pass these to kill thread routine
{
   int ipnano;
   SOCKET thd_sock;
   HANDLE dummy_event;
   HANDLE ThreadSockEvents;
   HANDLE Command_Event;
};

//******************************************************************************
// Function Prototypes
//******************************************************************************

//******************************************************************************
//
// kill_thread
//
// Clean up globals, events and die
//
// INPUT: -
// OUTPUT:- 
// OTHER:- 
//******************************************************************************

void kill_thread(int err_code,struct thread_data *);

//******************************************************************************
//
// DoMsg
//
// Handle error messages
//
// INPUT:  -
// OUTPUT: -
// OTHER:  -
//******************************************************************************

extern void DoMsg(char *);

//******************************************************************************
//
// makeitsend
//
// Send a signal and make sure it's all sent
//
//******************************************************************************

bool makeitsend(SOCKET its_sock,char *its_buf,int its_len, char *its_name,int ipna);


//******************************************************************************
//
// prepareFunctionChange
//
// Prepare for IPNA Fucntion Change
//
//******************************************************************************

extern DWORD prepareFunctionChange(char ipnx);


//******************************************************************************
//
// rollBackFunctionChange
//
// Roolback of IPNA Fucntion Change files in AP
//
//******************************************************************************

extern DWORD rollBackFunctionChange(char ipnx);


//******************************************************************************
//
// connectToCPon14000
//
// Make a connect attempt towards the CP IP address to see
// whether there is contact with the IPNA. 
//
//******************************************************************************

bool connectToCPon14000(short net);


//******************************************************************************
//
// recvitall
//
// Receive multiple bytes from a socket and ensure it is all received
//
//******************************************************************************

bool recvitall(SOCKET its_sock,char *its_buf,int its_len,char *its_name,int ipna);

//******************************************************************************
//
// find_older
//
// Compare two osedump file names and decide if the second is older 
// than the first. These files are created with names based on the date and
// time.
//
//******************************************************************************

bool file_older(char *file1, char *file2);

#endif