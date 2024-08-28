//******************************************************************************
// COPYRIGHT Ericsson Utvecklings AB, Sweden 2000,2010-2011.
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
// OCS_IPN_ExtFunx.h
//
// DESCRIPTION
// This file contains external prototypes and definitions.
//
// DOCUMENT NO
// 190 55-CAA 109 1405
//
// AUTHOR
// 2011xxxx XDT/DEK Rex Barnett
// 20110815 XDT/DEK Danh Nguyen
//
//******************************************************************************
// *** Revision history ***
// 20100916 Rex Created
// 20110801 Danh Updated
//******************************************************************************

#ifndef OCS_IPN_ExtFunx_h
#define OCS_IPN_ExtFunx_h

#include <string>
#include <vector>
#include <dirent.h>
#include <algorithm>
#include <errno.h>

using namespace std;

// Type definitions for vector
typedef vector<string> StrVec;
typedef vector<string>::iterator StrVecIter;

namespace OCS_IPN_ExtFnx
{

//******************************************************************************
// fileCopy
//
// Copy from filename1 to filename2
//
// INPUT:  src_file, dest_file
// OUTPUT: true/false
// OTHER:  -
//******************************************************************************

bool fileCopy(const char* src_file, const char* dest_file);

//******************************************************************************
// fileExists
//
// Check if the file exists
//
// INPUT:  filename
// OUTPUT: true/false
// OTHER:  -
//******************************************************************************

bool fileExists(const char* filename);

//******************************************************************************
// is_dotfile
//
// Check if the filename begins with a dot
//
// INPUT:  file name
// OUTPUT: true/false
// OTHER:  -
//******************************************************************************

bool is_dotFile(const string &);

//******************************************************************************
//
// is_notLogFile
//
// Checks if the filename is not a ".oselog" log file
//
// INPUT:  file name
// OUTPUT: true/false
// OTHER:  -
//******************************************************************************

bool is_notLogFile(const string &);

//******************************************************************************
// getDirFiles
//
// Get all files within a directory
//
// INPUT:  -
// OUTPUT: error code
// OTHER:  -
//******************************************************************************

int getDirFiles(const string &, StrVec&);

}

#endif

