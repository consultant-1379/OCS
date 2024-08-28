//******************************************************************************
// COPYRIGHT Ericsson Utvecklings AB, Sweden 2000,2010.
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
// OCS_IPN_ExtFnx.cpp
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

#include "OCS_IPN_ExtFnx.h"
#include "OCS_IPN_Trace.h"

#include <fstream>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sstream>
#include <cstring>

using namespace std;


//******************************************************************************
// fileCopy
//
// Copy from filename1 to filename2
//
// INPUT:  src_file, dest_file
// OUTPUT: true/false
// OTHER:  -
//******************************************************************************

bool OCS_IPN_ExtFnx::fileCopy(const char* src_file, const char* dest_file)
{

    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_ExtFnx::fileCopy",0));

    int read_fd;
    int write_fd;
    struct stat stat_buf;

    off_t offset = 0;

    /* Open the input file. */
    if ((read_fd = open(src_file, O_RDONLY)) < 0)
    {
        TRACE((LOG_LEVEL_INFO, "Open file failed with error: %s ",0,strerror(errno)));
        return false;
    }
    /* Stat the input file to obtain its size. */
    if (fstat(read_fd, &stat_buf) < 0)
    {
        TRACE((LOG_LEVEL_INFO, "Read file information failed with error: %s",0,strerror(errno)));

        close(read_fd);
        return false;
    }
    /* Open the output file for writing, with the same permissions as the
     source file. */
    if ((write_fd = open(dest_file, O_WRONLY | O_CREAT, stat_buf.st_mode)) < 0)
    {
        TRACE((LOG_LEVEL_INFO, "Open file failed with error: %s",0,strerror(errno)));

        close(read_fd);
        return false;
    }

    /* Blast the bytes from one file to the other. */
    bool res = sendfile(write_fd, read_fd, &offset, stat_buf.st_size) >= 0;

    /* Close up. */
    close(read_fd);
    close(write_fd);

    return res;
}

//******************************************************************************
// fileExists
//
// Check if the file exists
//
// INPUT:  filename
// OUTPUT: true/false
// OTHER:  -
//******************************************************************************

bool OCS_IPN_ExtFnx::fileExists(const char* filename)
{
    ifstream ifile(filename, ios_base::in);
    bool exist = ifile.good();
    if (exist)
        ifile.close();
    return exist;
}

//******************************************************************************
// is_dotfile
//
// Check if the filename begins with a dot
//
// INPUT:  file name
// OUTPUT: true/false
// OTHER:  -
//******************************************************************************

bool OCS_IPN_ExtFnx::is_dotFile(const string &filename)
{
    return (filename.substr(0, 1) == ".");
}

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

bool OCS_IPN_ExtFnx::is_notLogFile(const string &filename)
{
    size_t pos;
    std::string tempfile;

    pos = filename.rfind(".");
    if (pos == string::npos)
        return true;

    tempfile = filename.substr(pos + 1);
    return (tempfile.compare("oselog") != 0);
}

//******************************************************************************
// getDirFiles
//
// Get all files within a directory
//
// INPUT:  -
// OUTPUT: error code
// OTHER:  -
//******************************************************************************

int OCS_IPN_ExtFnx::getDirFiles(const string &dir, StrVec& files)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_ExtFnx::getDirFiles",0));

    DIR *dp;
    struct dirent *dirp;

    if ((dp = opendir(dir.c_str())) == NULL)
    {
        TRACE((LOG_LEVEL_INFO, "Open directory failed with error: %s",0,strerror(errno)));
        return errno;
    }

    while ((dirp = readdir(dp)) != NULL)
    {
        files.push_back(string(dirp->d_name));
    }

    StrVecIter new_begin = remove_if(files.begin(), files.end(), is_dotFile);
    files.erase(new_begin, files.end());

    closedir(dp);

    return 0;
}

