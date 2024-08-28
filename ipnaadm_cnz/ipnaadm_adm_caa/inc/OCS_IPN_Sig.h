
//******************************************************************************
// COPYRIGHT Ericsson Utvecklings AB, Sweden 2011.
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
// OCS_IPN_sig.h
//
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

#ifndef OCS_IPN_Sig_h
#define OCS_IPN_Sig_h

// Signal numbers
#define IDENTITYREQ       1 
#define IDENTITYCONF      2 
#define DUMPAVAILREQ      3 
#define DUMPAVAILCONF     4 
#define FETCHDUMPREQ      5 
#define SENDDUMPDATA      6 
#define HEARTBEATREQ      7
#define HEARTBEATCONF     8

#define SETIPNASTATEREQ  10 
#define SETIPNASTATECONF 11 

#define CMDSSREQ         20
#define CMDSSCONF        21
#define CMDLISTREQ       22
#define CMDLISTCONF      23
#define CMDOSDREQ        24
#define CMDOSDCONF       25
#define CMDFCPREPREQ     26
#define CMDFCPREPCONF    27
#define CMDROLLBACKREQ   28
#define CMDROLLBACKCONF  29

#define CMDDEBUGREQ      30
#define CMDDEBUGCONF     31





// Signal structures and constant definitions

/*****************************************************************************/

// Common signal header

#pragma pack (push)
#pragma pack (1)

struct sig_head 
{ 
   unsigned short sig_len;                     // Length of signal (network byte order !)
   char  sig_num;                              // Signal number   
   char  sig_ver;                              // Version (not used) 
};    

#pragma pack (pop)

/*****************************************************************************/

// IDENTITYREQ signal

#define SL_IDREQ        0                     // Length of IDENTITYREQ signal
#define ID_COMMAND    255                     // ID of command handler

#pragma pack (push)
#pragma pack (1)

struct idconf_sig
{
    struct sig_head sh;
    unsigned char   id;
};


#pragma pack (pop)


#define SL_IDENTITYCONF 1

/*****************************************************************************/

// DUMPAVAILREQ signal

#define SL_DAVAIL 0                          // Length of DUMPAVAILREQ signal
#define DUMPAVAIL 1
#define SL_DACONF 1                          // Length of DUMPAVAILCONF signal

/*****************************************************************************/

// FETCHDUMPREQ signal

#define SL_FDUMP 0                            // Length of FETCHDUMPREQ signal

/*****************************************************************************/

// HEARTBEATREQ signal

#define SL_HBREQ 0                            // Length of HEARTBEATREQ signal
#define CAN_HEARTBEAT 1              // IPNA can use heartbeats if sig_ver = 1

/*****************************************************************************/

// HEARTBEATCONF signal

#define SL_HBCONF 0                            // Length of HEARTBEATCONF signal

/*****************************************************************************/
// SETIPNASTATEREQ signal

#pragma pack (push)
#pragma pack (1)

struct ss_sig 
{
   struct sig_head sh;
   char   state;
   char   substate;
};

#pragma pack (pop)


#define SL_SSREQ            2                 // Length of SETIPNASTATEREQ signal
#define SL_SSCONF           1                 // Length of SETIPNASTATECONF signal

#define STATE_SEP           2                 // Bit in command for ipna state = seperated
#define IPNA_STATE_SEP      1                 // IPNA state = seperated in signal
#define IPNA_STATE_NORM     0                 // IPNA state = normal in signal

#define SUBSTATE_LOCK       1                 // Bit in command for ipna sub-state = lock
#define IPNA_SUBSTATE_OPEN  0                 // IPNA sub-state = open in signal
#define IPNA_SUBSTATE_LOCK  1                 // IPNA sub-state = lock in signal

/*****************************************************************************/

// CMDSSREQ signal

#pragma pack (push)
#pragma pack (1)

struct cmdssreq_sig                            // Structure for CMDSSREQ signal
{
   struct sig_head sh;                         // Signal header
   char ipnano;                                // IPNA number
   char state;                                 // Desired IPNA state
   char substate;                              // Desired IPNA substate
};

#pragma pack (pop)


#define SL_CMDSSREQ 3                          // Length of CMDSSREQ signal

// State and substate value

#define CMDSS_NORM  0                          // State = normal
#define CMDSS_SEP   1                          // State = seperated
#define CMDSS_OPEN  0                          // Sub-state = open
#define CMDSS_LOCK  1                          // Sub-state = lock

#pragma pack (push)
#pragma pack (1)

struct cmdssconf_sig                           // Structure for CMDSSCONF signal
{
   struct sig_head sh;                         // Signal header
   char result;                                // Result of operation 
};


#pragma pack (pop)


#define SL_CMDSSCONF 1                         // Length of CMDSSCONF signal 
// Result codes

#define CMDSS_SUCCESS 0                        // Operation succeeded
#define CMDSS_UNKNOWN 1                        // IPNA reports unknown state
#define CMDSS_EX_CP   2                        // IPNA reports order received in EX-CP state
#define CMDSS_NOTCON  3                        // IPNA is not connected


/*****************************************************************************/

// CMDLISTREQ signal

#pragma pack (push)
#pragma pack (1)

struct cmdlistreq_sig                          // Structure for CMDLISTREQ signal
{
   struct sig_head sh;                         // Signal header
};


#pragma pack (pop)


#define SL_CMDLIST 0                           // Length of list signal

#pragma pack (push)
#pragma pack (1)

struct cmdlistconf_sig                         // Structure for CMDLISTCONF signal
{
   struct sig_head sh;                         // Signal header
   char list[64];                              // Connection array
};


#pragma pack (pop)


#define SL_CMDLISTCONF  64                     // Length of list reply signal

/*****************************************************************************/

// CMDOSDREQ signal

#pragma pack (push)
#pragma pack (1)

struct cmdosdreq_sig                           // Structure for CMDOSDREQ signal
{
   struct sig_head sh;                         // Signal header
   char cmd;                                   // OSD sub-command
   char filenam;                               // Start of filename
};

#pragma pack (pop)


#define CMDOSD_DIR    0                        // ose dump dir sub-command
#define CMDOSD_LIST   1                        // ose dump dir list sub-command
#define CMDOSD_GET    2                        // ose dump file get sub-command

// OSDUMP signal lengths

#define SL_CMDOSD_DIR   1                      // Length of the directory request 
#define SL_CMDOSD_LIST  1                      // Length of the list request 
#define SL_CMDOSD_GET   1                      // Length of the get request less filename

#pragma pack (push)
#pragma pack (1)

struct cmdosdconf_sig                          // Structure for CMDOSDCONF signal
{
   struct sig_head sh;                         // Signal header
   char cmd;                                   // OSD sub-command
   char result;                                // Result of sub-command
   char data;                                  // Start of data from sub-command
};


#pragma pack (pop)


#define CMDOSD_SUCCESS 0                       // Result = sucess
#define CMDOSD_FAIL    1                       // Result = fail

#define SL_CMDOSD_DIRR  2                      // Length of the dir reply, no data 
#define SL_CMDOSD_LISTR 2                      // Length of the list reply, no data 
#define SL_CMDOSD_GETR  2                      // Length of the get reply, no data 


/*****************************************************************************/

// CMDFCPREPREQ signal

#pragma pack (push)
#pragma pack (1)

struct cmdfcprepreq_sig                        // Structure for CMDFCPREPREQ sig
{
   struct sig_head sh;                         // Signal header
   char ipnano;                                // IPNA number
};

#pragma pack (pop)


#define SL_CMDFCPREPREQ 1                      // Length of CMDFCPREPREQ signal


#pragma pack (push)
#pragma pack (1)

struct cmdfcprepconf_sig                       // Structure for CMDSSCONF signal
{
   struct sig_head sh;                         // Signal header
   char result;                                // Result of operation 
};

#pragma pack (pop)


#define SL_CMDFCPREPCONF 1                     // Length of CMDSSCONF signal 
// Result codes

#define CMDFCPREP_SUCCESS 0                    // Operation succeeded
#define CMDFCPREP_NOCHANGE 1                   // No Function Change files loaded
#define CMDFCPREP_FAULT 2                      // Other fault
#define CMDFCPREP_NOROLLBACK 3                 // No rollback possible to perform


/*****************************************************************************/

// CMDDEBUG signal

#pragma pack (push)
#pragma pack (1)

struct cmddebugreq_sig                         // Structure for CMDDEBUGREQ signal
{
   struct sig_head sh;                         // Signal header
   char cmd;                                   // DEBUG sub-command
};
 
#pragma pack (pop)

#define SL_CMDDEBUGREQ         1

#define CMDDEBUG_START         1
#define CMDDEBUG_STOP          2
#define CMDDEBUG_GET           3
#define CMDDEBUG_DELETE        4
#define CMDDEBUG_UNKNOWN       5


#pragma pack (push)
#pragma pack (1)

struct cmddebugconf_sig                        // Structure for CMDDEBUGCONF signal
{
   struct sig_head sh;                         // Signal header
   char cmd;                                   // DEBUG sub-command
   char result;                                // DEBUG sub-command result
   char data;                                  // Start of data returned by get
};
 
#pragma pack (pop)

#define SL_CMDDEBUGCONF        2

#define CMDDEBUG_OK            1
#define CMDDEBUG_FAIL          2

/*****************************************************************************/

#endif




