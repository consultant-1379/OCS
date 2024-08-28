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
// OCS_IPN_Global.h
//
// DESCRIPTION 
// This file contains global prototypes and definitions.
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

#ifndef  OCS_IPN_Global_h
#define  OCS_IPN_Global_h

#include "ACS_CS_API.h"

// OSELOG directory definitions
#define LOGPATH     "/data/ocs/logs/"        // Real path to log files


// Function Change Directory and files
#define TFTPBOOTDIR   "/data/apz/data/"
#define BOOTFILE      "/data/apz/data/boot.ipn"
#define BACKUPFILE    "/data/apz/data/backup.ipn"
#define OLDBACKUPFILE "/data/apz/data/old_backup.ipn"
#define TEMPBOOTFILE  "/data/apz/data/boot.temp"

// Function Change Directory and files
#define IPNAADMJTPNAME "IPNAADM_AP01"


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

#define ADM_PORT_NUMBER        2346      // Server port no. 

// IPNA specific 
#define MAX_IPNAS 64                      // Maximum number of IPNAS

#endif
