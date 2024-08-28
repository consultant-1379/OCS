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
// OCS_IPN_Global.h
//
// DESCRIPTION 
// This file contains global prototypes and definitions.
//
// DOCUMENT NO
// 1/190 55-CAA 109 0392
//
// AUTHOR 
// 000803 EPA/D/U Michael Bentley
//
//******************************************************************************
// *** Revision history ***
// 000803 MJB Created 
//******************************************************************************

#ifndef  OCS_IPN_Global_h
#define  OCS_IPN_Global_h
#define  _WIN32_WINNT 0x400

#include <winsock2.h>
#include <windows.h>
#include <process.h>
#include <ACS_JTP.H>
#include <ACS_PHA_Parameter.H>
#include "ACS_CS_Api.h"

#ifdef OCS_IPN_DEBUG
  #include <stdio.h>
  #include "OCS_IPN_syslog.h"
#endif

#ifdef _DEBUG
//   #define INC_SYSLOG   // Enable this to get use CPS_SYSLOG, include CPS_syslog.lib in link settings
#endif

extern HANDLE debug_file_handle;           // Debug file handle



#define DEBUG_FILE_PATH "C:\\ipnaadm"
#define DEBUG_FILE_NAME "c:\\ipnaadm\\ipnaadm.debuglog"

// OSELOG directory definitions

#ifdef _DEBUG
  #define LOGPATH_1ST "C:\\OCS"                // First directory in path
  #define LOGPATH     "C:\\OCS\\LOGS\\"        // Path to log files when debugging
#else
  #define LOGPATH_1ST "K:\\OCS"                // First directory in path
  #define LOGPATH     "K:\\OCS\\LOGS\\"        // Real path to log files
#endif


// Function Change Directory and files

#define TFTPBOOTPATH  "C:\\tftpboot\\"
#define TFTPBOOTDIR   "C:\\tftpboot\\*"
#define BOOTFILE      "C:\\tftpboot\\boot.ipn"
#define BACKUPFILE    "C:\\tftpboot\\backup.ipn"
#define OLDBACKUPFILE "C:\\tftpboot\\old_backup.ipn"
#define TEMPBOOTFILE  "C:\\tftpboot\\boot.temp"

// Function Change Directory and files

#define IPNAADMJTPNAME "IPNAADM_AP01"

// No of handles used in object handle arrays

#define  NO_OF_EVENTS_MAIN             6   // Max no of wait obj in Main

// Main object handle array

#define  SERVICE_ABORT_EVENT           0  // Abort,Service -> Main
#define  LISTSOCK_EVENTS               1  // Events from listening socket
#define  JTP_EVENTS                    2  // Events from JTP object

// JTP order from CP

#define PREPARE_FC 0

// Hosts and alias from hosts file

#define CP_EX_low      "cp0ex-stoc0-l1"
#define CP_EX_high     "cp0ex-stoc1-l2"
#define CP_SB_low      "cp0sb-stoc0-l1"
#define CP_SB_high     "cp0sb-stoc1-l2"
#define CP21240Ind     "cp0-Aside"

#define AP1_A_low      "ap1a-l1"
#define AP1_A_high     "ap1a-l2"
#define AP1_B_low      "ap1b-l1"
#define AP1_B_high     "ap1b-l2"


// TCP/FTP  codes


#define ADM_PORT_NUMBER        2346      // Server port no. 

// IPNA specific 

#define MAX_IPNAS 64                      // Maxmium number of IPNAS

// Internally generated error codes

#define USER_ERROR 0x23000000

#define ADM_ERR_STOP_TIMEOUT        (USER_ERROR | 0x00000001)
#define ADM_ERR_SERVICE_SHUTDOWN    (USER_ERROR | 0x00000002)
#define ADM_ERR_NO_HANDLE_EXIST     (USER_ERROR | 0x00000003)
#define ADM_ERR_TIMEOUT_DURING_STOP (USER_ERROR | 0x00000004)
#define ADM_ERR_UNEXPECTED_ERROR    (USER_ERROR | 0x00000005)

// Message handling levels

#define DEBUG_LEVEL_NONE 0               // Default level - no logging
#define DEBUG_LEVEL_BAD  1               // Only log bad errors
#define DEBUG_LEVEL_MILD 2               // Log mildly bad errors
#define DEBUG_LEVEL_INFO 3               // Information
#define DEBUG_LEVEL_ALL  4               // All messages   
                                          
// External variables   


#define SECURITY_ATTRIBUTES                     NULL
#define THREAD_STACK_SIZE                       0
// This macro simplifies the use of _beginthreadex.
// The call can be implemented the same way as a call to CreateThread, the same
// parameters can be used. 
// If this macro is used process.h have to be included and the code have to be 
// compiled for multi threading (one of /MD, /MDd, /MT or /MTd).

typedef unsigned (WINAPI *ThreadStart_p) (void*);
 
#define CREATE_THREAD(lpThreadAttributes,    \
                      dwStackSize,           \
                      lpStartAddress,        \
                      lpParameter,           \
                      dwCreationFlags,       \
                      lpThreadId)            \
                                             \
   ((HANDLE) _beginthreadex(                 \
      (void *)        (lpThreadAttributes),  \
      (unsigned)      (dwStackSize),         \
      (ThreadStart_p) (lpStartAddress),      \
      (void *)        (lpParameter),         \
      (unsigned)      (dwCreationFlags),     \
      (unsigned *)    (lpThreadId)))


// This macro simplifies the use of _endthreadex.
// The call can be implemented the same way as a call to ExitThread, the same
// parameters can be used. 
// If this macro is used process.h have to be included and the code have to be 
// compiled for multi threading (one of /MD, /MDd, /MT or /MTd).

#define EXIT_THREAD(dwExitCode) (_endthreadex((unsigned)dwExitCode))
 

     
//******************************************************************************
// Function Prototypes
//******************************************************************************
//******************************************************************************
// OCS_IPN_Main
//
// 
//
// INPUT: -
// OUTPUT:- 
// OTHER:- 
//******************************************************************************

DWORD OCS_IPN_Main(LPDWORD param);


//******************************************************************************
// OCS_IPN_Thread
//
// Thread for handling connections
//
// INPUT:  -
// OUTPUT: -
// OTHER:  -
//******************************************************************************


DWORD OCS_IPN_Thread(LPDWORD param);

#endif
