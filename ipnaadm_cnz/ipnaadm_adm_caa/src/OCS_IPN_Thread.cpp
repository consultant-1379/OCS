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
// OCS_IPN_Thread.cpp
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

#include "OCS_IPN_Thread.h"
#include "OCS_IPN_Utils.h"
#include "OCS_IPN_Trace.h"

#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <vector>
#include <algorithm>

using namespace std;

int    OCS_IPN_Thread::s_commandConnected = 0;  // Indication of command handler connection
int    OCS_IPN_Thread::s_commandData[MAX_IPNAS] // Command data from command handler
										= {
											0, 0, 0, 0, 0, 0, 0, 0,
											0, 0, 0, 0, 0, 0, 0, 0,
											0, 0, 0, 0, 0, 0, 0, 0,
											0, 0, 0, 0, 0, 0, 0, 0,
											0, 0, 0, 0, 0, 0, 0, 0,
											0, 0, 0, 0, 0, 0, 0, 0,
											0, 0, 0, 0, 0, 0, 0, 0,
											0, 0, 0, 0, 0, 0, 0, 0
										 };

int    OCS_IPN_Thread::s_replyData[MAX_IPNAS]        // Reply data to command handler
									   = {
											0, 0, 0, 0, 0, 0, 0, 0,
											0, 0, 0, 0, 0, 0, 0, 0,
											0, 0, 0, 0, 0, 0, 0, 0,
											0, 0, 0, 0, 0, 0, 0, 0,
											0, 0, 0, 0, 0, 0, 0, 0,
											0, 0, 0, 0, 0, 0, 0, 0,
											0, 0, 0, 0, 0, 0, 0, 0,
											0, 0, 0, 0, 0, 0, 0, 0
										};

bool   OCS_IPN_Thread::s_replied[MAX_IPNAS] // IPNA replied indication array
									    = {
										    0, 0, 0, 0, 0, 0, 0, 0,
										    0, 0, 0, 0, 0, 0, 0, 0,
										    0, 0, 0, 0, 0, 0, 0, 0,
										    0, 0, 0, 0, 0, 0, 0, 0,
										    0, 0, 0, 0, 0, 0, 0, 0,
										    0, 0, 0, 0, 0, 0, 0, 0,
										    0, 0, 0, 0, 0, 0, 0, 0,
										    0, 0, 0, 0, 0, 0, 0, 0
									    };

int    OCS_IPN_Thread::s_connected[MAX_IPNAS]           // IPNA connection array
								       = {
										    0, 0, 0, 0, 0, 0, 0, 0,
										    0, 0, 0, 0, 0, 0, 0, 0,
										    0, 0, 0, 0, 0, 0, 0, 0,
										    0, 0, 0, 0, 0, 0, 0, 0,
										    0, 0, 0, 0, 0, 0, 0, 0,
										    0, 0, 0, 0, 0, 0, 0, 0,
										    0, 0, 0, 0, 0, 0, 0, 0,
										    0, 0, 0, 0, 0, 0, 0, 0
									    };

uint32_t OCS_IPN_Thread::s_linkIP[2] = {0,0};

Event OCS_IPN_Thread::s_threadTermEvent;
Event OCS_IPN_Thread::s_commandEvents[MAX_IPNAS];
Event OCS_IPN_Thread::s_ipnaRepliedEvent;	// Reply event to command handler
Event OCS_IPN_Thread::s_multiConTermEvent[MAX_IPNAS];    // Events for killing multiple IPNA connections
Event OCS_IPN_Thread::s_multiCmdTermEvent;              // Event for killing multiple command connections
Event OCS_IPN_Thread::s_stopEvent;

long   OCS_IPN_Thread::s_noConns = 0;                    // Number of connections made since service started
int    OCS_IPN_Thread::s_currCons = 0;                   // Number of current connections

CriticalSection OCS_IPN_Thread::s_connSec;          // Critical section for changing curr_cons
CriticalSection OCS_IPN_Thread::s_ipnaSec;
CriticalSection OCS_IPN_Thread::m_critSec;

/****************************************************************************
 * Method:	constructor
 * Description:
 * Param [in]: N/A
 * Param [out]: N/A
 * Return: N/A
 *****************************************************************************
 */
OCS_IPN_Thread::OCS_IPN_Thread()
: m_newSocket(-1),
  m_stop(false)
{
    OCS_IPN_Thread::s_stopEvent.resetEvent();
}

/****************************************************************************
 * Method:	destructor
 * Description:
 * Param [in]: N/A
 * Param [out]: N/A
 * Return: N/A
 *****************************************************************************
 */
OCS_IPN_Thread::~OCS_IPN_Thread()
{

    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Thread::~OCS_IPN_Thread",0));

}

/****************************************************************************
 * Method:	start()
 * Description: Use to start IPN Thread
 * Param [in]: N/A
 * Param [out]: N/A
 * Return: N/A
 *****************************************************************************
 */
void OCS_IPN_Thread::start()
{
	newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Thread::start()",0));

    int maxfd;
	fd_set readset, allset;
	char   signal;
	int	result;

	int    rest,nget, remain;
	int    dumpavail,code,state,substate;
	int    ipnano;
	int    dest_ipna;
	char * listp;

	char ch;
	char send_buf[SENDBUFSIZE];                    // Buffer for sending to socket
	char rec_buf[6];                               // Buffer for receiving signal header from socket
	char *data_buf;                                // Buffer for receiving signal data from socket
	int sig_len;                                   // The length of a signal
	bool hbts;                                     // Heart beat time supervision in progress
	int ipna_version;                              // sig_ver from ID conf tells ipna version
	ssize_t lresult;

	struct thread_data thread_data;

	int hLogFile = -1; //ose dump file
	int numfile;
	char filenam[256];  // Place to create dumpfile name
	char filepath[256]; // Full path to logfile
	int fileHandle;

	DIR *dir;
	struct dirent *dirEnt;
	struct stat st;
	const string directory(LOGPATH);

	ipnano = 128; // Start with unused value

	hbts = false;       // Not time supervising heartbeat
	ipna_version = 0;   // IPNA can't heartbeat unless it says otherwise

	maxfd = max(m_newSocket,OCS_IPN_Thread::s_stopEvent.getFd());

	FD_ZERO(&allset);

	//Add thread's socket to select set
	FD_SET(m_newSocket, &allset);

	// Add stop event to allset
	FD_SET(OCS_IPN_Thread::s_stopEvent.getFd(), &allset);

	
	// Sent IDENTITYREQ message to client (IPNAOS or ipnaadm command)

	((struct sig_head *)send_buf)->sig_len = htons(SL_IDREQ);
	((struct sig_head *)send_buf)->sig_num = IDENTITYREQ;
	((struct sig_head *)send_buf)->sig_ver = 0;

	if(!makeItSend(m_newSocket,send_buf,SL_IDREQ+sizeof(struct sig_head),"IDENTITYREQ",ipnano))
	{
		killThread(8,&thread_data);              // Clean up and die
	}

	int ret_val;
	timeval timeOutValue;
	timeval* timeOutPtr;
	timeOutValue.tv_sec = 0;
	timeOutValue.tv_usec = 0;

	timeOutPtr = NULL;

	while (!m_stop )
	{
		readset = allset;
		ret_val = ::select(maxfd+1, &readset, NULL, NULL, timeOutPtr);

		if(ret_val == 0)// Timeout 
		{
			if(hbts)
			{
                TRACE((LOG_LEVEL_TRACE, "IPNA %d: Heartbeat timeout ",0,ipnano));
                killThread(3,&thread_data);              // Clean up and die
			}
			else if(ipna_version == CAN_HEARTBEAT)
            {
               // No dump is available and IPNA can use heartbeat mechanism
                timeOutValue.tv_sec = HEARTBEAT_TIMEOUT;
                timeOutValue.tv_usec = 0;
                timeOutPtr = &timeOutValue;

                hbts = true;                  // Enable heartbeat supervision

                // Send heartbeat signal to ipna

                ((struct sig_head *)send_buf)->sig_len = htons(SL_HBREQ);
                ((struct sig_head *)send_buf)->sig_num = HEARTBEATREQ;
                ((struct sig_head *)send_buf)->sig_ver = 0;

                if(!makeItSend(m_newSocket,send_buf,SL_HBREQ+sizeof(struct sig_head),"HEARTBEATREQ",ipnano))
                  killThread(8,&thread_data);              // Clean up and die

            }
			else
			{
			    TRACE((LOG_LEVEL_TRACE, "IPNA %d : Select timeout",0,ipnano));
                killThread(4,&thread_data);              // Clean up and die
			}
		}
		else if(ret_val == -1)// Error
		{
		    TRACE((LOG_LEVEL_ERROR, "select failed. error message: %s",0, strerror(errno)));

		    if(errno == EINTR)// A signal was caught
			{
				continue;
			}

			killThread(2,&thread_data);
			break;
		}
		else
		{
			//Check for stop event
			if(FD_ISSET(OCS_IPN_Thread::s_stopEvent.getFd(),&readset))
			{
                TRACE((LOG_LEVEL_INFO, "OCS_IPN_Thread: receive stop event: %s ",0,strerror(errno)));
                m_stop = true;
			}

			//Check s_commandEvents
			int i;
			for(i = 0; i < MAX_IPNAS; ++i)
			{
				if(FD_ISSET(OCS_IPN_Thread::s_commandEvents[i].getFd(),&readset))
				{
					TRACE((LOG_LEVEL_INFO, "IPNA %d: received command event",0, ipnano));

					OCS_IPN_Thread::s_ipnaSec.enter();
					OCS_IPN_Thread::s_commandEvents[i].resetEvent();
					OCS_IPN_Thread::s_ipnaSec.leave();

					// The only command sent to IPNAs so far is set state with a state and sub-state value
					if((ipnano >= 0) && (ipnano < MAX_IPNAS)) // If valid IPNA id range
					{
						// Send SETIPNASTATEREQ with state, sub-state from command
						((struct ss_sig *)send_buf)->sh.sig_len = htons(SL_SSREQ);
						((struct ss_sig *)send_buf)->sh.sig_num = SETIPNASTATEREQ;
						((struct ss_sig *)send_buf)->sh.sig_ver = 0;

						if(OCS_IPN_Thread::s_commandData[ipnano] & STATE_SEP)
							((struct ss_sig *)send_buf)->state = IPNA_STATE_SEP;
						else
							((struct ss_sig *)send_buf)->state = IPNA_STATE_NORM;

						// The sub-state is locked if state=sep, open if norm

						if(OCS_IPN_Thread::s_commandData[ipnano] & STATE_SEP)
							((struct ss_sig *)send_buf)->substate = IPNA_SUBSTATE_LOCK;
						else
							((struct ss_sig *)send_buf)->substate = IPNA_SUBSTATE_OPEN;

						if(!makeItSend(m_newSocket,send_buf,SL_SSREQ+sizeof(struct sig_head),"SETIPNASTATEREQ",ipnano))
							killThread(8,&thread_data);              // Clean up and die

						OCS_IPN_Thread::s_commandData[ipnano] = 0;
					}
					else
					{
					    TRACE((LOG_LEVEL_INFO, "Command event received by the command handler!",0));
					    killThread(129,&thread_data);              // Clean up and die
					}
				}
			}

			// Check for reply event
			if(FD_ISSET(OCS_IPN_Thread::s_ipnaRepliedEvent.getFd(),&readset))
			{
				TRACE((LOG_LEVEL_INFO, "IPNA %d ",0,ipnano));

				OCS_IPN_Thread::s_ipnaRepliedEvent.resetEvent();

				// At the moment, there is only one command generating a reply event
				if(ipnano == ID_COMMAND)               // Test for command handler
				{
					for(i=0;i<MAX_IPNAS;i++)
					{
						if(OCS_IPN_Thread::s_replied[i])  // Test if IPNA has a reply
						{
							// Send CMSSCONF back to command handler with result from Reply_Data[i]

							((struct cmdssconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDSSCONF);
							((struct cmdssconf_sig *)send_buf)->sh.sig_num = CMDSSCONF;
							((struct cmdssconf_sig *)send_buf)->sh.sig_ver = 0;
							((struct cmdssconf_sig *)send_buf)->result = OCS_IPN_Thread::s_replyData[i];

							if(!makeItSend(m_newSocket,send_buf,SL_CMDSSCONF+sizeof(struct sig_head),"CMDSSCONF",ipnano))
							killThread(8,&thread_data);              // Clean up and die
							OCS_IPN_Thread::s_replied[i] = false;          // Handled a reply
						}
					}

					OCS_IPN_Thread::s_ipnaRepliedEvent.resetEvent(); // Probably safe to reset this now
				}
				else
				{
				    TRACE((LOG_LEVEL_INFO, "ipna replied event received by ipna handler!",0));
				    killThread(130,&thread_data);              // Clean up and die
				}
			}

			// Check for Multiple connection terminate events (for IPNAOS)
			for(i = 0; i < MAX_IPNAS; ++i)
			{
				if(FD_ISSET(OCS_IPN_Thread::s_multiConTermEvent[i].getFd(),&readset))
				{
				    TRACE((LOG_LEVEL_INFO, "IPNA %d: received multi connection terminate event",0,ipnano));
				    //OCS_IPN_Thread::s_multiConTermEvent[i].resetEvent();

					if((ipnano >=0) && (ipnano < MAX_IPNAS))    // If valid IPNA id range
					{
						//ResetEvent(MultiConTermEvent[ipnano]);   // multicon term is done
						killThread(888,&thread_data);           // just kill thread
					}
					else if(ipnano == ID_COMMAND)               // Test for command handler
					{
						//ResetEvent(MultiCmdTermEvent);           // multicmd term is done
						killThread(889,&thread_data);           // just kill thread

					}
				}
			}

			//Check for Multiple command terminate event
			if(FD_ISSET(OCS_IPN_Thread::s_multiCmdTermEvent.getFd(),&readset))
			{
				TRACE((LOG_LEVEL_INFO, "IPNA %d: Thread: %d received multi connection terminate event",0, ipnano, pthread_self()));
				//OCS_IPN_Thread::s_multiCmdTermEvent.resetEvent();

				if((ipnano >=0) && (ipnano < MAX_IPNAS))    // If valid IPNA id range
				{
					//ResetEvent(MultiConTermEvent[ipnano]);   // multicon term is done
					killThread(888,&thread_data);           // just kill thread
				}
				else if(ipnano == ID_COMMAND)               // Test for command handler
				{
					//ResetEvent(MultiCmdTermEvent);           // multicmd term is done
					killThread(889,&thread_data);           // just kill thread
				}

			}

			// Socket signaled
			if (FD_ISSET(this->m_newSocket, &readset))
			{
				TRACE((LOG_LEVEL_INFO, "IPNA %d : Socket is signaled: %d",0, ipnano, m_newSocket));

				// receive the signal header from IPNA
				if(!recvItAll(this->m_newSocket,rec_buf,sizeof(struct sig_head),"Signal Header",0))
				{
					killThread(5,&thread_data);              // Clean up and die
				}

				signal = ((struct sig_head *)rec_buf)->sig_num;
				switch(signal)
				{
					// Signal from either IPNAs or command handler(s)
					case IDENTITYCONF:
					{
						TRACE((LOG_LEVEL_INFO, "IPNA %d: received IDENTITYCONF",0, ipnano));

						sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);
						if(sig_len != SL_IDENTITYCONF)
						{
							// The signal length received is not correct, throw it away and die
							killThread(102,&thread_data);              // Clean up and die
						}

						ipna_version = ((struct sig_head *)rec_buf)->sig_ver; // Get IPNA version

                        if(ipna_version == CAN_HEARTBEAT)
                        {
                            TRACE((LOG_LEVEL_INFO, "IPNA %d: can heartbeat",0, ipnano));
                        }
                        else
                        {
                            TRACE((LOG_LEVEL_INFO, "IPNA %d: can not heartbeat",0, ipnano));
                        }

							// read identity from signal
						result = recv(m_newSocket,&ch,1,0);
						if(result == -1)
						{
							TRACE((LOG_LEVEL_ERROR, "IPNA %d :Read of id failed with error %s",0,ipnano,strerror(errno)));

							killThread(103,&thread_data);              // Clean up and die
						}

						ipnano = (int)ch & 255;
						thread_data.ipnano = ipnano;

						if(ipnano == ID_COMMAND)               // Test for command handler
						{
							OCS_IPN_Thread::s_ipnaSec.enter();

							// ID from IDENTITYCONF signal indicates connection from command handler
							OCS_IPN_Thread::s_multiCmdTermEvent.resetEvent();      // Reset event in case it's still active

							// Add multiple connection event from command to allset
							FD_SET(OCS_IPN_Thread::s_multiCmdTermEvent.getFd(), &allset);
							maxfd = max(maxfd,OCS_IPN_Thread::s_multiCmdTermEvent.getFd());

							OCS_IPN_Thread::s_commandConnected++;               // Indicate command handler connected
							if(OCS_IPN_Thread::s_commandConnected > 1)
							{
								OCS_IPN_Thread::s_multiCmdTermEvent.setEvent();    // Order termination of all command threads
							}

							// Add reply event to allset. Only for Command handler thread.
							OCS_IPN_Thread::s_ipnaRepliedEvent.resetEvent();
							FD_SET(OCS_IPN_Thread::s_ipnaRepliedEvent.getFd(), &allset);
							maxfd = max(maxfd,OCS_IPN_Thread::s_ipnaRepliedEvent.getFd());

							for(i=0;i<MAX_IPNAS;i++)
							{
								s_replied[i] = false;              // Clear any outstanding IPNA replies
							}
							OCS_IPN_Thread::s_ipnaSec.leave();

						}
						else if((ipnano >=0) && (ipnano < MAX_IPNAS)) // Test valid IPNA id range
						{
							// ID from IDENTITYCONF signal indicates an IPNA
							OCS_IPN_Thread::s_ipnaSec.enter();
							OCS_IPN_Thread::s_multiConTermEvent[ipnano].resetEvent(); // Reset event in case it's still active

							// Add mutiple connection event from IPNA to allset
							FD_SET(OCS_IPN_Thread::s_multiConTermEvent[ipnano].getFd(), &allset);
							maxfd = max(maxfd,OCS_IPN_Thread::s_multiConTermEvent[ipnano].getFd());

							OCS_IPN_Thread::s_connected[ipnano]++;                   // Indicate IPNA connected
							if(OCS_IPN_Thread::s_connected[ipnano] >1)               // Check for multiple connections
							{
								OCS_IPN_Thread::s_multiConTermEvent[ipnano].setEvent();    // Kill multiple connections
							}

							OCS_IPN_Thread::s_commandData[ipnano] = 0;           // Clear command data
							OCS_IPN_Thread::s_replyData[ipnano] = 0;             // Clear reply data
							OCS_IPN_Thread::s_replied[ipnano] = false;            // Clear replied indicator

							OCS_IPN_Thread::s_ipnaSec.leave();

							// Add command event to allset. Only for the IPNA thread
							OCS_IPN_Thread::s_commandEvents[ipnano].resetEvent();
							FD_SET(OCS_IPN_Thread::s_commandEvents[ipnano].getFd(), &allset);
							maxfd = max(maxfd,OCS_IPN_Thread::s_commandEvents[ipnano].getFd());


							// Send DUMPAVAILREQ to IPNA

							((struct sig_head *)send_buf)->sig_len = htons(SL_DAVAIL);
							((struct sig_head *)send_buf)->sig_num = DUMPAVAILREQ;
							((struct sig_head *)send_buf)->sig_ver = 0;

							if(!makeItSend(m_newSocket,send_buf,SL_DAVAIL+sizeof(struct sig_head),"DUMPAVAILREQ",ipnano))
							{
							   killThread(8,&thread_data);              // Clean up and die
							}
						}
						else
						{
							TRACE((LOG_LEVEL_INFO, "Invalid ID ",0));
							killThread(104,&thread_data);              // Clean up and die
						}

						break;
					} // End case IDENTITYCONF

					// Signals from IPNAs only
					case DUMPAVAILCONF:
					{
						TRACE((LOG_LEVEL_INFO, "IPNA %d : received DUMPAVAILCONF",0, ipnano));

						sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);
						if(sig_len != SL_DACONF)
						{
							// The signal length received is not correct, throw it away and die
							killThread(105,&thread_data);              // Clean up and die
						}

						if((ipnano >=0) && (ipnano < MAX_IPNAS)) // If valid IPNA id range
						{

							// Check fetch dump avail result and proceed if dump is avail
							result = recv(this->m_newSocket,&ch,1,0);
							dumpavail = (int)ch;
							if(result == -1)
							{
								TRACE((LOG_LEVEL_ERROR, "IPNA %d : Read of result of DUMPAVAILCONF failed with error: %s ",0,ipnano,strerror(errno)));

								killThread(106,&thread_data);              // Clean up and die
							}
							else if( dumpavail == DUMPAVAIL)
							{
								// Create unique filename based on ipnano, date and time
								time_t curentTime;
								struct tm * timeinfo;
								curentTime = time(NULL);
								timeinfo = localtime(&curentTime);

								sprintf(filenam,"%sipna%02d-%02d%02d%02d-%02d%02d%02d.oselog",LOGPATH,ipnano,timeinfo->tm_year + 1900,
									timeinfo->tm_mon + 1,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);

								OCS_IPN_Thread::m_critSec.enter(); // Avoid problems with directory lists

								hLogFile = open(filenam,O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
								if(hLogFile == -1)
								{

									TRACE((LOG_LEVEL_ERROR, "IPNA %d : Create file failed : %s",0,ipnano,strerror(errno)));

									OCS_IPN_Thread::m_critSec.leave();
									killThread(109,&thread_data);              // Clean up and die
								}

								TRACE((LOG_LEVEL_INFO, "IPNA %d : Create osedump log file : %s",0,ipnano,filenam));
								OCS_IPN_Thread::m_critSec.leave();

								// Send FETCHDUMPREQ

								((struct sig_head *)send_buf)->sig_len = htons(SL_FDUMP);
								((struct sig_head *)send_buf)->sig_num = FETCHDUMPREQ;
								((struct sig_head *)send_buf)->sig_ver = 0;

								if(!makeItSend(m_newSocket,send_buf,SL_FDUMP+sizeof(struct sig_head),"FETCHDUMPREQ",ipnano))
								  killThread(8,&thread_data);              // Clean up and die

							}
							else if((dumpavail != DUMPAVAIL) && (ipna_version == CAN_HEARTBEAT))
							{
							   // No dump is available and IPNA can use heartbeat mechanism
								timeOutValue.tv_sec = HEARTBEAT_TIMEOUT;
								timeOutValue.tv_usec = 0;
								timeOutPtr = &timeOutValue;

								hbts = true;                  // Enable heartbeat supervision

								// Send heartbeat signal to ipna

								((struct sig_head *)send_buf)->sig_len = htons(SL_HBREQ);
								((struct sig_head *)send_buf)->sig_num = HEARTBEATREQ;
								((struct sig_head *)send_buf)->sig_ver = 0;

								if(!makeItSend(m_newSocket,send_buf,SL_HBREQ+sizeof(struct sig_head),"HEARTBEATREQ",ipnano))
								  killThread(8,&thread_data);              // Clean up and die

							}

							if(dumpavail != DUMPAVAIL)
							{
							    TRACE((LOG_LEVEL_INFO, "IPNA %d : OSE dump is not available.",0,ipnano));
							}

							if(ipna_version != CAN_HEARTBEAT)
								timeOutPtr = NULL;           // Set select timeout to infinite

						}
						else
						{
							TRACE((LOG_LEVEL_INFO, "IPNA %d : DUMPAVAILCONF received by command handler",0,ipnano));
							killThread(110,&thread_data);              // Clean up and die
						}
						break;
					}

					case SENDDUMPDATA:
					{
						TRACE((LOG_LEVEL_INFO, "IPNA %d : received SENDDUMPDATA",0,ipnano));

						if((ipnano >=0) && (ipnano < MAX_IPNAS)) // If valid IPNA id range
						{

							data_buf = new char[DBUFSIZE];   // Do this to reduce run time memory usage

							// This is how much data we're expecting
							sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);
							rest = sig_len;

							// Read 256 byte chunks of data and dump into file

							while(rest !=0)
							{
								if(rest > 256)
								{
									nget = 256;
									rest -= 256;
								}
								else
								{
									nget = rest;
									rest = 0;
								}


								if(!recvItAll(this->m_newSocket,data_buf,nget,"Dump data",0))
								{

									delete[] data_buf;        // free_data_buf
									close(hLogFile);    // Close file
									killThread(111,&thread_data);              // Clean up and die
								}
								else
								{

									// Write data to file and check success
									if(write(hLogFile,data_buf,nget) == -1)
									{
										TRACE((LOG_LEVEL_ERROR, "IPNA %d : Write to file %s failed with error %s ",ipnano,filenam, strerror(errno)));

										close(hLogFile);    // Close file
										delete[] data_buf;    // free_data_buf
										killThread(112,&thread_data);   // Clean up and die
									}

								}
							}

							delete[] data_buf;    // De-allocate buffer memory
							close(hLogFile);    // Close file

							//  Perform dump file maintenance - remove old files if more than 64 dump files
							OCS_IPN_Thread::m_critSec.enter();

							dir = opendir(directory.c_str());
							if(dir == NULL)  // Change to the directory containing the logs
							{
							    TRACE((LOG_LEVEL_ERROR, "IPNA %d: Open the logs directory: %s failed with error %s",0,ipnano, LOGPATH,strerror(errno)));
							}
							else
							{
								numfile = 0;
								filenam[0] = '\0';
								
								while ((dirEnt = readdir(dir))!= NULL)
								{
									const string currentFile = dirEnt->d_name;
									const string fullcurrentFile = directory + "/" + currentFile;

									if (stat(fullcurrentFile.c_str(), &st) == -1)
										   break;

									const bool isDirectory = (st.st_mode & S_IFDIR) != 0;

									if (isDirectory)
										   continue;


									numfile++;
									if(fileOlder(filenam,dirEnt->d_name))    // Compare file names to get oldest
									{
										strcpy(filenam,dirEnt->d_name);
									}


								}

								if(numfile > 63)
								{
									// Delete oldest file found if more than 63 of them
									const string fullOldestFile = directory + "/" + filenam;
									remove(fullOldestFile.c_str());

									TRACE((LOG_LEVEL_INFO, "IPNA %d: Deleting file %s ",0, ipnano,filenam));
								}
								
							}

							closedir(dir);

							OCS_IPN_Thread::m_critSec.leave();

							if(ipna_version == CAN_HEARTBEAT)
							{
								// IPNA can use heartbeat mechanism
								timeOutValue.tv_sec = HEARTBEAT_TIMEOUT;
								timeOutValue.tv_usec = 0;
								timeOutPtr = &timeOutValue;

								hbts = true;                  // Enable heartbeat supervision

								// Send heartbeat signal to ipna
								((struct sig_head *)send_buf)->sig_len = htons(SL_HBREQ);
								((struct sig_head *)send_buf)->sig_num = HEARTBEATREQ;
								((struct sig_head *)send_buf)->sig_ver = 0;

								if(!makeItSend(m_newSocket,send_buf,SL_HBREQ+sizeof(struct sig_head),"HEARTBEATREQ",ipnano))
								  killThread(8,&thread_data);              // Clean up and die
							}
						}
						else
						{
							TRACE((LOG_LEVEL_INFO, "IPNA %d: SENDDUMPDTA received by command handler",0,ipnano));
							killThread(113,&thread_data);              // Clean up and die
						}
						break;
					}

					case HEARTBEATCONF:
					{
						TRACE((LOG_LEVEL_INFO, "IPNA %d: received HEARTBEATCONF",0,ipnano));

						sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);
						if(sig_len != SL_HBCONF)
						{
							// The signal length received is not correct, throw it away and die

							killThread(1135,&thread_data);              // Clean up and die
						}

							hbts = false;     // Let WMO timeout so new heartbeat request can be sent
							break;
					}

					case SETIPNASTATECONF:
					{
					    TRACE((LOG_LEVEL_INFO, "IPNA %d: received SETIPNASTATECONF",0,ipnano));

						sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);
						if(sig_len != SL_SSCONF)
						{
							// The signal length received is not correct, throw it away and die

							killThread(114,&thread_data);              // Clean up and die
						}
						if((ipnano >=0) && (ipnano < MAX_IPNAS)) // If valid IPNA id range
						{
							result = recv(m_newSocket,&ch,1,0);   // Receive result code
							code = (int)ch;
							if(result == -1)
							{

								TRACE((LOG_LEVEL_ERROR, "IPNA %d : Read of result of SETIPNASTATECONF failed: %s",0,ipnano, strerror(errno)));
								killThread(115,&thread_data);              // Clean up and die
							}
							else
							{
								// Send reply
								OCS_IPN_Thread::s_replyData[ipnano] = code;
								OCS_IPN_Thread::s_replied[ipnano] = true;          // Indicate reply waiting
								OCS_IPN_Thread::s_ipnaRepliedEvent.setEvent();  // Inform command thread of reply
							}
						}
						else
						{
						    TRACE((LOG_LEVEL_INFO, "IPNA %d: SETIPNASTATECONF received by command handler",0,ipnano));
						    killThread(116,&thread_data);              // Clean up and die
						}
						break;
					}

					// -------------------------------------------------------------------------------------
					// Signals from command handler(s) only

					// -------------------------------------------------------------------------------------

					// Set IPNA state

					case CMDSSREQ:
					{
						TRACE((LOG_LEVEL_INFO, "IPNA %d: received CMDSSREQ",0,ipnano));

						sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);
						if(sig_len != SL_CMDSSREQ)
						{
							// The signal length received is not correct, throw it away and die

							killThread(117,&thread_data);              // Clean up and die

						}

						if(ipnano == ID_COMMAND)               // Test for command handler, ignore if not
						{
							// Read dest ipna, state, sub-state from signal

							result = recv(m_newSocket,&ch,1,0);   // Receive destination IPNA from signal
							if(result == -1)
							{
                                TRACE((LOG_LEVEL_ERROR, "IPNA %d : Read of CMDSSREQ signal data failed: %s",0,ipnano,strerror(errno)));
                                killThread(118,&thread_data);              // Clean up and die
							}

							dest_ipna = (int)ch;

							result = recv(m_newSocket,&ch,1,0);  // Receive state from signal
							if(result == -1)
							{
							   	TRACE((LOG_LEVEL_ERROR, "IPNA %d: Read of CMDSSREQ signal data failed: %s",0, ipnano,strerror(errno)));
							   	killThread(119,&thread_data);              // Clean up and die
							}
							
							state = (int)ch;

							result = recv(m_newSocket,&ch,1,0);  // Receive sub-state from signal
							if(result == -1)
							{
							    TRACE((LOG_LEVEL_ERROR, "IPNA %d: Read of CMDSSREQ signal data failed: %s",0,ipnano,strerror(errno)));
								killThread(120,&thread_data);              // Clean up and die
							}
							substate = (int)ch;

							if(OCS_IPN_Thread::s_connected[dest_ipna]==0)  // Send failure if ipna not connected
							{
							  // Send CMSSCONF back to command handler with result from Reply_Data[i]
							  ((struct cmdssconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDSSCONF);
							  ((struct cmdssconf_sig *)send_buf)->sh.sig_num = CMDSSCONF;
							  ((struct cmdssconf_sig *)send_buf)->sh.sig_ver = 0;
							  ((struct cmdssconf_sig *)send_buf)->result = CMDSS_NOTCON;

							  if(!makeItSend(m_newSocket,send_buf,SL_CMDSSCONF+sizeof(struct sig_head),"CMDSSCONF",ipnano))
								 killThread(8,&thread_data);              // Clean up and die
							}

							// set Command_Data[dest ipnano]
							OCS_IPN_Thread::s_commandData[dest_ipna] = 0;

							// This is a bit simple as both IPNA_STATE_SEP and IPNA_SUBSTATE_LOCK are 1 and their opposites
							// are 0. STATE_SEP and SUBSTATE_LOCK are bit positions. Woe betide he who changes this

							if(state == IPNA_STATE_SEP)
							   OCS_IPN_Thread::s_commandData[dest_ipna] |= STATE_SEP;

							if(substate == IPNA_SUBSTATE_LOCK)
							   OCS_IPN_Thread::s_commandData[dest_ipna] |= SUBSTATE_LOCK;


							// Set Command_Event[dest ipnano]
							OCS_IPN_Thread::s_ipnaSec.enter();
							OCS_IPN_Thread::s_commandEvents[dest_ipna].setEvent();  // Tell thread for destination IPNA to send SETIPNASTATEREQ signal
							OCS_IPN_Thread::s_ipnaSec.leave();
						}
						else
						{
								TRACE((LOG_LEVEL_INFO, "IPNA %d: CMDSSREQ received by ipna handler",0,ipnano));
								killThread(121,&thread_data);              // Clean up and die
						}

						break;
					}

					// -------------------------------------------------------------------------------------

					// List connected IPNAs

					case CMDLISTREQ:
					{
					    TRACE((LOG_LEVEL_INFO, "IPNA %d: received CMDLISTREQ",0,ipnano));

						sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);
						if(sig_len != SL_CMDLIST)
						{
							// The signal length received is not correct, throw it away and die
							killThread(122,&thread_data);              // Clean up and die

						}

						if(ipnano == ID_COMMAND)               // Test for command handler
						{
							// Send CMDLISTCONF back to command handler with list of connected IPNAs

							((struct cmdlistconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDLISTCONF);
							((struct cmdlistconf_sig *)send_buf)->sh.sig_num = CMDLISTCONF;
							((struct cmdlistconf_sig *)send_buf)->sh.sig_ver = 0;
							listp = &((struct cmdlistconf_sig *)send_buf)->list[0]; // Pointer to start of list
							/*
							for(i=0; i<2; i++)
							{
								if (connectToCPon14000(i) == true)
									*listp++ = 1;				// Return element of connected array
								else
									*listp++ = 0;
							}
							*/
							for(i=0; i<MAX_IPNAS; i++)
							{
								*listp++ = (char) OCS_IPN_Thread::s_connected[i];	// Return element of connected array
							}

							if(!makeItSend(m_newSocket,send_buf,SL_CMDLISTCONF+sizeof(struct sig_head),"CMDLISTCONF",ipnano))
								killThread(8,&thread_data);              // Clean up and die

						}
						else
						{
						    TRACE((LOG_LEVEL_INFO, "IPNA %d : CMDLISTREQ received by ipna handler",0,ipnano));
						}

						killThread(253,&thread_data);              // Clean up and die
						break;
					}

					// -------------------------------------------------------------------------------------

					// Handle OSEDUMP commands

					case CMDOSDREQ:
					{
					    TRACE((LOG_LEVEL_INFO, "IPNA %d: received CMDOSDREQ",0,ipnano));

						if(ipnano == ID_COMMAND)               // Test for command handler
						{
							result = recv(m_newSocket,&ch,1,0);   // Receive dump sub-command from signal
							if(result == -1)
							{
								TRACE((LOG_LEVEL_ERROR, "IPNA %d: Read of CMDOSDREQ signal data failed %s",0,ipnano, strerror(errno)));
								killThread(124,&thread_data);              // Clean up and die
							}

							switch(ch)
							{
							    case CMDOSD_LIST:
								{
								    TRACE((LOG_LEVEL_INFO, "IPNA %d: received CMDOSD_LIST",0,ipnano));

								    sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);
									if(sig_len != SL_CMDOSD_LIST)
									{
										// The signal length received is not correct, throw it away and die
										killThread(126,&thread_data);              // Clean up and die
									}

									// Search for osedump files in LOGPATH and return in CMDOSDCONF signal
									((struct cmdosdconf_sig *)send_buf)->sh.sig_num = CMDOSDCONF;
									((struct cmdosdconf_sig *)send_buf)->sh.sig_ver = 0;
									((struct cmdosdconf_sig *)send_buf)->cmd = CMDOSD_LIST;  // Indicate which sub-command rely comes from
									((struct cmdosdconf_sig *)send_buf)->result = CMDOSD_SUCCESS; // No error


									dir = opendir(directory.c_str());
									if(dir == NULL)  // Change to the directory containing the logs
									{
										((struct cmdosdconf_sig *)send_buf)->result = CMDOSD_FAIL; // Return error value
										((struct cmdosdconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDOSD_LISTR); // Signal length

										if(!makeItSend(m_newSocket,send_buf,SL_CMDOSD_LISTR+sizeof(struct sig_head),"CMDOSDCONF",ipnano))
											killThread(8,&thread_data);              // Clean up and die
									}
									else
									{
										OCS_IPN_Thread::m_critSec.enter();    // Prevent ipna threads creating new files until list finished

										listp = &((struct cmdosdconf_sig *)send_buf)->data;// Pointer to start of file list data
										*listp = '\0';                                     // Empty string

										vector<string> fileList(64, "");
										vector<string>::iterator it;

										while ((dirEnt = readdir(dir))!= NULL)
										{
											const string currentFile = dirEnt->d_name;
											const string fullcurrentFile = directory + "/" + currentFile;

											if (stat(fullcurrentFile.c_str(), &st) == -1)
											   break;

											const bool isDirectory = (st.st_mode & S_IFDIR) != 0;

											if (isDirectory)
											   continue;

											fileList.insert(fileList.begin(), string(dirEnt->d_name));

										}

										// Sort the file list
										sort (fileList.begin(), fileList.end());

										for (it=fileList.begin(); it!=fileList.end(); ++it)
										{
											if(strcmp((*it).c_str(),"") != 0)
											{
												strcat(listp,(*it).c_str());
												strcat(listp,"\n");

											}
										}

										OCS_IPN_Thread::m_critSec.leave();

										remain = SL_CMDOSD_LISTR+strlen(listp)+1;   // Signal length + length of list string (including terminating 0)
										((struct cmdosdconf_sig *)send_buf)->sh.sig_len = htons(remain); // Signal length

										if(!makeItSend(m_newSocket,send_buf,remain+sizeof(struct sig_head),"CMDOSDCONF",ipnano))      // Send the CMDOSDCONF signal
											killThread(8,&thread_data);              // Clean up and die

									}

									closedir(dir);

									break;
								}

								// -------------------------------------------------------------------------------------

								case CMDOSD_GET:
								{
								    TRACE((LOG_LEVEL_INFO, "IPNA %d: received CMDOSD_GET",0,ipnano));

								    sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);

									// Open the file in the signal and send it back in the reply
									// N.B. Dump files are about 8K in length and will fit into
									// the 64k send buffer

									((struct cmdosdconf_sig *)send_buf)->sh.sig_num = CMDOSDCONF;
									((struct cmdosdconf_sig *)send_buf)->sh.sig_ver = 0;
									((struct cmdosdconf_sig *)send_buf)->cmd = CMDOSD_GET;  // Indicate which sub-command rely comes from
									((struct cmdosdconf_sig *)send_buf)->result = CMDOSD_SUCCESS; // No error


									dir = opendir(directory.c_str());
									if(dir == NULL)  // Change to the directory containing the logs
									{
										((struct cmdosdconf_sig *)send_buf)->result = CMDOSD_FAIL;       // Return error value
										((struct cmdosdconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDOSD_GETR); // Signal length

										if(!makeItSend(m_newSocket,send_buf,SL_CMDOSD_GETR+sizeof(struct sig_head),"CMDOSDCONF",ipnano))
											killThread(8,&thread_data);              // Clean up and die
									}
									else
									{
										// Receive rest of signal -> filename
										recvItAll(m_newSocket,filenam,sig_len - SL_CMDOSD_GET ,"CMDOSD get filename",ipnano);
										filenam[sig_len - SL_CMDOSD_GET] = '\0';  // Terminate string
										strcpy(filepath,LOGPATH);    // Full path to file starts with log directory
										strcat(filepath,filenam);    // Add file name

										TRACE((LOG_LEVEL_INFO, "IPNA %d: getting file %s",0,ipnano,filenam));

										TRACE((LOG_LEVEL_INFO, "IPNA %d: file with path %s",0, ipnano,filepath));

										if((fileHandle = open(filepath,O_RDONLY)) == -1)
										{
											TRACE((LOG_LEVEL_ERROR, "IPNA %d: Open of file %s with error %s",0,ipnano,filepath,strerror(errno)));
											close(fileHandle);
											((struct cmdosdconf_sig *)send_buf)->result = CMDOSD_FAIL;       // File doesn't exist, return error value
											((struct cmdosdconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDOSD_GETR); // Signal length
											if(!makeItSend(m_newSocket,send_buf,SL_CMDOSD_GETR+sizeof(struct sig_head),"CMDOSDCONF",ipnano))
											   killThread(8,&thread_data);              // Clean up and die
										}
										else
										{
											listp = &((struct cmdosdconf_sig *)send_buf)->data;     // Pointer to start of data in return signal

											// Read whole file into send buffer, ask for bufsize -sig length up to now

											if((lresult = read(fileHandle,listp,SENDBUFSIZE-SL_CMDOSD_GETR-sizeof(struct sig_head))) == -1)
											{
												close(fileHandle);
												((struct cmdosdconf_sig *)send_buf)->result = CMDOSD_FAIL;       // Read file failed, return error value
												((struct cmdosdconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDOSD_GETR); // Signal length

												if(!makeItSend(m_newSocket,send_buf,SL_CMDOSD_GETR+sizeof(struct sig_head),"CMDOSDCONF",ipnano))
													killThread(8,&thread_data);              // Clean up and die
											}
											else
											{
											   close(fileHandle);
											   ((struct cmdosdconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDOSD_GETR +(int)lresult); // Signal length

											   if(!makeItSend(m_newSocket,send_buf,SL_CMDOSD_GETR+sizeof(struct sig_head) +(int)lresult,"CMDOSDCONF",ipnano))
												  killThread(8,&thread_data);              // Clean up and die
											}
										 }
									  }

									  break;
								}
								default:
								{
									TRACE((LOG_LEVEL_INFO, "IPNA %d: didn't handle signal %d in CMDOSD",0,ipnano,(int)ch));
									break;
								}

							}
						}
						else
						{
						    TRACE((LOG_LEVEL_INFO, "IPNA %d: CMDLISTREQ received by ipna handler",0,ipnano));
						    killThread(127,&thread_data);              // Clean up and die
						}

						killThread(254,&thread_data);              // Clean up and die
						break;
					}

					// -------------------------------------------------------------------------------------

					// Prepare for Function Change in IPNA

					case CMDFCPREPREQ:
					{
						TRACE((LOG_LEVEL_INFO, "IPNA %d: received CMDFCPREPREQ",0,ipnano));
						sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);
						if(sig_len != SL_CMDFCPREPREQ)
						{
							// The signal length received is not correct, throw it away and die

							killThread(117,&thread_data);      // Clean up and die

						}
						if(ipnano == ID_COMMAND)               // Test for command handler, ignore if not
						{
							// Read dest ipna from signal
							result = recv(m_newSocket,&ch,1,0);    // Receive destination IPNA from signal
							if(result == -1)
							{
							    TRACE((LOG_LEVEL_ERROR, "IPNA %d: Read of CMDFCPREPREQ signal data failed: %s",0,ipnano,strerror(errno)));
							    killThread(118,&thread_data);   // Clean up and die
							}

							dest_ipna = (int)ch;

							result = OCS_IPN_Thread::prepareFunctionChange(dest_ipna);

							// Send CMSSCONF back to command handler with result from Reply_Data[i]
							((struct cmdssconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDFCPREPCONF);
							((struct cmdssconf_sig *)send_buf)->sh.sig_num = CMDFCPREPCONF;
							((struct cmdssconf_sig *)send_buf)->sh.sig_ver = 0;
							((struct cmdssconf_sig *)send_buf)->result = result;

							if (!makeItSend(m_newSocket, send_buf, SL_CMDFCPREPCONF+sizeof(struct sig_head), "CMDFCPREPCONF",ipnano))
								killThread(8,&thread_data);            // Clean up and die
						 }
						 else
						 {
								TRACE((LOG_LEVEL_INFO, "IPNA %d: CMDFCPREPREQ received by ipna handler %s",0,ipnano,strerror(errno)));
								killThread(121,&thread_data);              // Clean up and die
						 }

						 break;
					}
					default:
					{
					    TRACE((LOG_LEVEL_INFO, "IPNA %d didn't handle signal %d",0,ipnano,(int)ch));
					    break;
					}
				}
			}

		}
	}

	killThread(0,&thread_data);              // Clean up and die

	TRACE((LOG_LEVEL_INFO, "OCS_IPN_Thread: exit start()",0));
}

/****************************************************************************
 * Method:	stop()
 * Description: Use to stop IPN Thread
 * Param [in]: N/A
 * Param [out]: N/A
 * Return: N/A
 *****************************************************************************
 */
void OCS_IPN_Thread::stop() const
{

}

/****************************************************************************
 * Method:	makeItSend
 * Description: Send message to client
 * Param [in]: its_sock - socket to send data
 * Param [in]: its_buf - data to send
 * Param [in]: its_len - data lengh to send
 * Param [in]: its_name - name of data
 * Param [in]: ipnano - ipna #
 * Param [out]: N/A
 * Return: true or false
 *****************************************************************************
 */
bool OCS_IPN_Thread::makeItSend(int its_sock,char *its_buf,int its_len, const char *its_name,int ipnano)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Thread::makeItSend()",0));

    int result = 0;
    int aktuell;
    char *datap;


	datap = its_buf;
	aktuell = its_len;

	while (aktuell)
	{
		if (aktuell > 1000)
		{
			aktuell = 1000;
		}
		result = send(its_sock, datap, aktuell, 0);
		if(result == -1)
		{
			return(false);
		}
		its_len -= result;
		aktuell = its_len;
		datap += result;
	}

		TRACE((LOG_LEVEL_INFO, "IPNA %d: sent %s signal ",0,ipnano,its_name));
		memset(its_buf,0,4); // clear the signal header part of the send buffer
    return(true);
}

/****************************************************************************
 * Method:	recvItAll
 * Description: receive message from client
 * Param [in]: its_sock - socket to receiv data
 * Param [in]: its_buf - buffer to receive data
 * Param [in]: its_len - maximum data to receive
 * Param [in]: its_name - name of data
 * Param [in]: ipnano - ipna #
 * Param [out]: N/A
 * Return: true or false
 *****************************************************************************
 */
bool OCS_IPN_Thread::recvItAll(int its_sock,char *its_buf,int its_len,const char* its_name,int ipnano)
{
	newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Thread::recvItAll",0));

    int result;

	// Set timeout to 100 milliseconds
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 100*1000;

	::setsockopt(its_sock,SOL_SOCKET,SO_RCVTIMEO, &tv,sizeof(tv));
	result = recv(its_sock,its_buf,its_len,	MSG_WAITALL); // Try to read all bytes asked for

	
    if(result == -1)
    {
        TRACE((LOG_LEVEL_ERROR, "IPNA %d: recv %s failed with error: %s",0,ipnano,its_name,strerror(errno)));
        return(false);
    }
	else if(result < its_len)
	{
	    TRACE((LOG_LEVEL_ERROR, "IPNA %d: recv %d bytes of %s with error %s",0,ipnano,result,its_name,strerror(errno)));
	    return(false);
	 }
	 else if(result == 0)
	 {
		TRACE((LOG_LEVEL_INFO, "IPNA %d: recv zero byte: %s",0,ipnano,strerror(errno)));
		return(false);
	 }

    return(true);
}

/****************************************************************************
 * Method:	killThread
 * Description: kill thread
 * Param [in]: err_code - error code
 * Param [in]: thread_data - thread infromation
 * Param [out]: N/A
 * Return: N/A
 *****************************************************************************
 */
void OCS_IPN_Thread::killThread(int err_code,struct thread_data *thread_data)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Thread::killThread()",0));

    // Print debug information
	TRACE((LOG_LEVEL_INFO, "Thread for ipna %d terminated with code: %d",0,thread_data->ipnano,err_code));
	m_stop = true;

	// Reset s_connected and s_commandConnected
	if((thread_data->ipnano >=0) && (thread_data->ipnano  < MAX_IPNAS))   // If valid IPNA id range
	{
		OCS_IPN_Thread::s_ipnaSec.enter();

		OCS_IPN_Thread::s_connected[thread_data->ipnano] = 0;

		OCS_IPN_Thread::s_ipnaSec.leave();
	}
	else if(thread_data->ipnano == ID_COMMAND)          // ID for command handler
	{
		OCS_IPN_Thread::s_ipnaSec.enter();
		OCS_IPN_Thread::s_commandConnected = 0;                           // Indicate command handler not connected
		OCS_IPN_Thread::s_ipnaSec.leave();
	}

	// Close socket
	::shutdown(m_newSocket, SHUT_RDWR);
	::close(m_newSocket);

	// Exit thread
	::pthread_exit(NULL);

}

/****************************************************************************
 * Method:	connectToCPon14000
 * Description: Connect to IPNAs at port 14000
 * Param [in]: net 
 * Param [out]: N/A
 * Return: true or false
 *****************************************************************************
 */
bool OCS_IPN_Thread::connectToCPon14000(uint16_t net)
{
    bool retCode = false;

	int sock;
	sockaddr_in localAddr_in;


    if (OCS_IPN_Thread::s_linkIP[net&1] != 0)
	{
	    // Get the address of the requested host
	    memset(&localAddr_in, 0, sizeof(localAddr_in));
		localAddr_in.sin_family = AF_INET;
		localAddr_in.sin_addr.s_addr = OCS_IPN_Thread::s_linkIP[net&1];;
		localAddr_in.sin_port = htons(14000);

	    if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) > 0)
	    {
		    // set socket to non-blocking mode
		    int flags= fcntl(sock,F_GETFL, 0 );
		    fcntl(sock, F_SETFL, flags | O_NONBLOCK);


		    if (connect(sock, (sockaddr *) &localAddr_in, sizeof(sockaddr_in)) < 0)
		    {
		    	if(errno == EINPROGRESS)
			    {
		    		fd_set readset, writeset;
				    FD_ZERO(&readset);
				    FD_SET(sock, &readset);
				    FD_ZERO(&writeset);
				    FD_SET(sock, &writeset);
				    timeval selectTimeout;
				    selectTimeout.tv_sec = 0;
				    selectTimeout.tv_usec = 10000;
				    int select_return = select(sock+1,&readset, &writeset, NULL, &selectTimeout);
				    if(select_return == 0)
				    {
					    //cout << " select function timeout" << endl;
				    }
				    else if(select_return == -1)
				    {
					    //cout << " select function failed" << endl;
				    }
				    else
				    {
					    if(FD_ISSET(sock,&readset) || FD_ISSET(sock,&writeset))
					    {
						    int error;
						    socklen_t len = sizeof(error);
						    if(getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) == 0)
						    {
							    if(error == 0)
							    {
								    //cout << "connect successfully:" <<strerror(errno) << endl;
								    retCode = true;
							    }
							    else
							    {
								    //cout << "connect failed:" <<strerror(errno) << "error:" << error << ": " << strerror(error) << endl;
							    }
						    }
					    }
				    }
				}
			    else
			    {
				    //cout << "connect failed: " <<strerror(errno) << endl;

			    }
            }
		    else
		    {
			    //cout << " connect ok:" <<strerror(errno) << endl;
			    retCode = true;
		    }

		    fcntl(sock, F_SETFL, flags);

	        close(sock);
	    }
	}

    //cout << "return code: "<< retCode <<", exit: " <<strerror(errno) << endl;
	return retCode;
}


/****************************************************************************
 * Method:	setNewSocket
 * Description: Set socket for thread
 * Param [in]: newSocket - socket descriptor for thread communicate with IPNAOS or ipnaadm command
 * Param [out]: N/A
 * Return: N/A
 *****************************************************************************
 */
void OCS_IPN_Thread::setNewSocket(int newSocket)
{
    m_newSocket = newSocket;
}


/****************************************************************************
 * Method:	fileOlder
 * Description: Compare two files to see which one is older
 * Param [in]: file1 - first file
 * Param [in]: file1 - second file
 * Param [out]: N/A
 * Return: true if file1 older than file2 and false otherwise
 *****************************************************************************
 */
bool OCS_IPN_Thread::fileOlder(char *file1, char *file2)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Thread::fileOlder",0));

    int f[7],h[7],result,i;
    char *thing;
    int where;

    // Extract the date and time related parts from file name 1, return false on error

    // First, extract path
    if(strcmp(file1,"") == 0)      // First file will be null, return true so copy happens
      return true;

    where = -1;
    thing = file1;
    for(i=0;i<(int)strlen(file1)-4;i++)
    {
      if((strncmp(thing,"ipna",4) == 0) || (strncmp(thing,"Ipna",4) == 0))
      {
        where = i;
        break;
      }
      thing++;
    }
 
    if(where == -1)
    {
        TRACE((LOG_LEVEL_INFO, "fileOlder: ipna not found in %s",0,file1));
        return false;
    }
    else
    {
      if((result = sscanf(thing,"ipna%2d-%4d%2d%2d-%2d%2d%2d.oselog",&f[0],&f[1],&f[2],&f[3],&f[4],&f[5],&f[6])) != 7)  // Will return different number if name doesn't match format
      {
         if((result = sscanf(thing,"Ipna%2d-%4d%2d%2d-%2d%2d%2d.oselog",&f[0],&f[1],&f[2],&f[3],&f[4],&f[5],&f[6])) != 7)  // Will return different number if name doesn't match format
         {
           TRACE((LOG_LEVEL_INFO, "fileOlder : scanf on file %s returned %d  match(es)",0,file1,result));
           return false;
         }
      }
    }

    // Extract the date and time related parts from file name 2, return false on error

    where = -1;
    thing = file2;
    for(i=0;i<(int)strlen(file1)-4;i++)
    {
        if(strncmp(thing,"ipna",4) == 0)
        {
        where = i;
        break;
        }
        thing++;
    }
     
    if(where == -1)
    {
        TRACE((LOG_LEVEL_INFO, "fileOlder : ipna not found in %s",0,file2));
        return false;
    }
    else
    {
        if((result = sscanf(thing,"ipna%2d-%4d%2d%2d-%2d%2d%2d.oselog",&h[0],&h[1],&h[2],&h[3],&h[4],&h[5],&h[6])) != 7)  // Will return different number if name doesn't match format
        {
            TRACE((LOG_LEVEL_INFO, "fileOlder : scanf on file %s returned %d match(es)",0,file2,result));
            return false;
      }
    }


    if(f[1] > h[1])
        return true;        // Year for file 2 is older than file 1
    else if(f[1] < h[1])
        return false;       // Year for file 2 is older than file 1
    else if(f[2] > h[2])
        return true;        // Month for file 2 is older than file 1
    else if(f[2] < h[2])
        return false;       // Month for file 1 is older than file 2
    else if(f[3] > h[3])
        return true;         // Day for file 2 is older than file 1
    else if(f[3] < h[3])
        return false;        // Day for file 1 is older than file 2
    else if(f[4] > h[4])
        return true;        // Hour for file 2 is older than file 1
    else if(f[4] < h[4])
        return false;       // Hour for file 1 is older than file 2
    else if(f[5] > h[5])
        return true;        // Minute for file 2 is older than file 1
    else if(f[5] < h[5])
        return false;       // Minute for file 1 is older than file 2
    else if(f[6] > h[6])
        return true;        // Second for file 2 is older than file 1
    else if(f[6] < h[6])
        return false;       // Second for file 1 is older than file 2
    else
    return false;       // file 2 is not older than file 1

}

/****************************************************************************
 * Method:	prepareFunctionChange
 * Description: prepare boot.ipn# for function change
 * Param [in]: ipnx - ipna #
 * Param [out]: N/A
 * Return: 
 *        CMDFCPREP_SUCCESS(0) : Operation succeeded
 *        CMDFCPREP_NOCHANGE(1): No Function Change files loaded
 *        CMDFCPREP_FAULT(2)   : Other fault
 *****************************************************************************
 */
uint32_t OCS_IPN_Thread::prepareFunctionChange(char ipnx)
{
	newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Thread::prepareFunctionChang",0));

    bool			equality;
	int			    highest;
	int			    open_handle;

	char*			p1;
	char*			p2;
	char*			p3;
	char*			p4;
	char*			ptr;

	uint32_t		index = 0;
	ssize_t			nbRead;
	uint32_t		nbWritten;
	uint32_t		len;
	uint32_t		ind;
	bool			changed1 = false;
	bool			changed2 = false;
	char			filecontents[16][120];
	char			module[16][32];
	char			module1[32];
	char			filename[80];
	char			prefix[80];
	char            tmp[80];
	char			oneline[120];
	char			ffile[80] = TFTPBOOTDIR;
	char			ffile_backup[80];
	char			ffile_oldbackup[80];
	sprintf(ffile_backup,    "%s%d", BACKUPFILE,    ipnx); //TODO: need review
	sprintf(ffile_oldbackup, "%s%d", OLDBACKUPFILE, ipnx); //TODO: need review
	char			bootcontents[2048];
	char			bootcontents_out[2048];
	bootcontents_out[0] = '\0';

	DIR *dir;
	struct dirent *dirEnt;
	struct stat st;
	const string directory(TFTPBOOTDIR);

	TRACE((LOG_LEVEL_INFO, "prepare function change",0));

	// First scan through all newrev_*.ver files and store their contents in memory
	dir = opendir(directory.c_str());
	if(dir == NULL)
	{
	    TRACE((LOG_LEVEL_ERROR, "opendir failed: %s",0,strerror(errno)));
	}
	else
	{
		while ((dirEnt = readdir(dir))!= NULL)
		{
			const string currentFile = dirEnt->d_name;
			const string fullcurrentFile = directory + "/" + currentFile;

			if (stat(fullcurrentFile.c_str(), &st) == -1)
			   break;

			const bool isDirectory = (st.st_mode & S_IFDIR) != 0;

			if (isDirectory)
			   continue;

			strcpy(filename, dirEnt->d_name);
			ptr = strchr(filename, 95);				// Underscore
			if (ptr != NULL)
			{
				strncpy(prefix, filename, (ptr-filename));
				prefix[ptr-filename] = '\0';
				if (OCS_IPN_Utils::strICompare(prefix, "newrev") == 0)
				{
					strcpy(filename, ++ptr);
					ptr = strchr(filename, 46);		// Dot
					if (ptr != NULL)
					{
						strncpy(prefix, filename, (ptr-filename));
						prefix[ptr-filename] = '\0';
						strcpy(&module[index][0], prefix);
						strcpy(filename, ++ptr);
						if (OCS_IPN_Utils::strICompare(filename, "ver") == 0)
						{
							strcpy(ffile, TFTPBOOTDIR);
							strcat(ffile, dirEnt->d_name);

							open_handle = open(ffile,O_RDONLY);
							if (open_handle != -1)
							{
								nbRead = read(open_handle,filecontents[index],119);
								if (nbRead != -1)
								{
									if (nbRead > 0)
									{
										filecontents[index][nbRead] = '\0';
										if (filecontents[index][nbRead-1] < 32)
											filecontents[index][nbRead-1] = '\0';
										index++;
									}
								}
							}
						}
					}
				}
			}
		}

		closedir(dir);
	}


	// Now go into the boot file and edit the lines with new module versions
	/***********************************************************************
	 * 1. Read boot "/data/apz/data/boot.ipnx" (x=0|1|2|3|..) to bootcontents buffer
	 * 2. Get one line of bootfile from bootcontents buffer and analyze
	 * 3. Get module name (ex. ipnaos) from oneline buffer to module1 buffer
	 * 4. Find the module name corresponding from filecontents if any and compare the with oneline from boot file.
	 * 5. If the line from booot file is different from filecontents that has the same module name, it means that
	 *    there is a change from from the revision of module, the boot file must be updated with the new name
	 *
	 ***********************************************************************/

	sprintf(ffile, "%s%d", BOOTFILE, ipnx);

	open_handle = open(ffile,O_RDWR);
	if (open_handle != -1)
	{
		nbRead = read(open_handle,bootcontents,2048);
		if (nbRead > 0)
		{
			ptr = strchr(bootcontents, 10);		// Line feed
			while (ptr != NULL)
			{
				strncpy(oneline, bootcontents, (ptr-bootcontents));
				oneline[ptr-bootcontents] = '\0';
				if (oneline[ptr-bootcontents-1] < 32)
					oneline[ptr-bootcontents-1] = '\0';

				p1 = strchr(oneline, 61);		// Equal sign
				p2 = strchr(oneline, 46);		// Dot
				if ((p1!=NULL) && (p2!=NULL) && (p2>p1))
				{
					strncpy(module1, p1+1, (p2-p1-1));
					module1[p2-p1-1] = '\0';
					for (ind=0; ind<index; ind++)
					{
						if (OCS_IPN_Utils::strICompare(module1, module[ind]) == 0)
						{
							if (OCS_IPN_Utils::strICompare(oneline, filecontents[ind]) != 0)
								changed1 = true;
							module[ind][0] = 0;
							strcpy(oneline, filecontents[ind]);
							break;
						}
					}
				}
				len = strlen(oneline);
				if (len == 0)
				{
					for (ind=0; ind<index; ind++)
					{
						if (module[ind][0] != 0)
						{
							module[ind][0] = 0;
							changed1 = true;
							len = strlen(filecontents[ind]);
							filecontents[ind][len] = 10; //Line feed
							OCS_IPN_Utils::putBuffer(filecontents[ind], len+1, bootcontents_out);
							break;
						}
					}
					len = 0;
				}
				oneline[len] = 10;			// Add some line feed signs
				OCS_IPN_Utils::putBuffer(oneline, len+1, bootcontents_out);
				ptr = ptr + 1;
				strcpy(bootcontents, ptr);
				ptr = strchr(bootcontents, 10);
			}
		}

		close(open_handle);
	}


	// If the boot file is to be been changed, push the existing files and create a new boot.ipnX
	if (changed1)
	{
		remove(ffile_oldbackup);
		rename(ffile_backup, ffile_oldbackup);
		rename(ffile, ffile_backup);

		open_handle = open(ffile, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

		if (open_handle == -1)
		{
			// Need review
			//CopyFile(ffile_backup, ffile, false);
		}
		else
		{
			nbWritten = write(open_handle,bootcontents_out, strlen(bootcontents_out));
					  
			close(open_handle);
		}
	}


	// Now scan through the directory again to check all *.babsc.* files and compare
	// with the contents of the boot.ipnX file. If there is mismatch between *.babsc.*
	// files and module revisions given in boot.ipnX, update boot.ipnX with the existing
	// *.babsc.* files (highest revision). An IPNA function change must be possible.

	// First scan through the existing *.basc.* files. Store in Primary Memory.
	/***************************************************************************
	 * Scan module file .babsc and get modules to module array, file name to filecontents array.
	 */


	index = 0;
	bootcontents_out[0] = '\0';
	strcpy(ffile, TFTPBOOTDIR);

	dir = opendir(ffile);
	if(dir == NULL)
	{
	    TRACE((LOG_LEVEL_ERROR, "opendir failed: %s",0,strerror(errno)));
	}
	else
	{
		while ((dirEnt = readdir(dir))!= NULL)
		{
			const string currentFile = dirEnt->d_name;
			const string fullcurrentFile = directory + currentFile;

			if (stat(fullcurrentFile.c_str(), &st) == -1)
			   break;

			const bool isDirectory = (st.st_mode & S_IFDIR) != 0;

			if (isDirectory)
			   continue;

			strcpy(filename, dirEnt->d_name);
			p1 = strchr(filename, 46);				// Dot
			if (p1 != NULL)
			{
			    p2 = strchr((char*)(p1+1), 46);         // Dot
			    if(p2 != NULL)
			    {
                    strncpy(prefix, (char*)(p1+1), p2-p1-1);
                    prefix[p2-p1-1] = '\0';
                    if (OCS_IPN_Utils::strICompare(prefix, "babsc") == 0)
                    {
                        strncpy(prefix, filename, p1-filename);
                        strcpy(module[index], prefix);
                        module[index][p1-filename] = '\0';
                        strcpy(filecontents[index], filename);
                        filecontents[index][strlen(filename)] = '\0';
                        index++;
                    }
			    }
			}
		}

		closedir(dir);
	}

	// Now we go into the boot file and update according to
	sprintf(ffile, "%s%d", BOOTFILE, ipnx);
	open_handle = open(ffile,O_RDWR);
	
	if (open_handle != -1)
	{
		nbRead = read(open_handle,bootcontents,2048);

		if (nbRead > 0)
		{
			ptr = strchr(bootcontents, 10);		// Line feed
			while (ptr != NULL)
			{
				strncpy(oneline, bootcontents, (ptr-bootcontents));
				oneline[ptr-bootcontents] = '\0';
			    if (oneline[ptr-bootcontents-1] < 32)
					oneline[ptr-bootcontents-1] = '\0';

			    equality = false;
				highest = -1;
				p1 = strchr(oneline, 61);		// Equal sign
				p2 = strchr(oneline, 46);		// Dot
				p3 = strchr(oneline, 44);		// Comma
				if ((p1!=NULL) && (p2!=NULL) && (p3!=NULL) && (p2>p1) && (p3>p2))
				{
					strncpy(module1, p1+1, (p2-p1-1));
					module1[p2-p1-1] = '\0';

					strncpy(prefix, p1+1, (p3-p1-1));
                    prefix[p3-p1-1] = '\0';
                    strcpy(tmp, prefix);


                    // Find the first revision of module1 from filecontents array
                    for (ind=0; ind<index; ind++)
                    {
                        if (OCS_IPN_Utils::strICompare(module1, module[ind]) == 0)
                        {
                                strcpy(tmp, filecontents[ind]);
                                highest = ind;
                                break;
                        }
                    }

                    // Find the highest revision of module1 from filecontents array
                    for (ind=0; ind<index; ind++)
                    {
                        if (OCS_IPN_Utils::strICompare(module1, module[ind]) == 0)
                        {
                            if (OCS_IPN_Utils::strICompare(tmp, filecontents[ind]) < 0)
                            {
                                strcpy(tmp, filecontents[ind]);
                                highest = ind;
                            }
                        }
                    }

                    // Compare the highest revision of from filecontents with prefix
                    if ((highest != -1) && (OCS_IPN_Utils::strICompare(prefix, filecontents[highest]) == 0))
                    {
                        equality = true;
                    }

					if ((equality == false) && (highest != -1))
					{
						changed2 = true;
						len = strlen(filecontents[highest]);
						strncpy(prefix, &filecontents[highest][len-5], 5);
						prefix[5] = '\0';
						strncpy(p3-5, prefix, 5);
						p4 = strchr(p3+1, 44);		  // Comma again
						if (p4 != NULL)
						{
							string temp(prefix);
							OCS_IPN_Utils::toUpper(temp);
							strcpy(prefix, temp.c_str());
							strncpy(p4-5, prefix, 5);
						}
					}
				}

				len = strlen(oneline);
				oneline[len] = 10;				// Add some line feed
				OCS_IPN_Utils::putBuffer(oneline, len+1, bootcontents_out);
				ptr = ptr + 1;
				strcpy(bootcontents, ptr);
				ptr = strchr(bootcontents, 10);
			}
		}
	}

	if(!changed1 && changed2)
	{
		if (open_handle != -1)
			close(open_handle);

		remove(ffile_oldbackup);
		rename(ffile_backup, ffile_oldbackup);
		rename(ffile, ffile_backup);

		open_handle = open(ffile, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

		if (open_handle == -1)
		{
			// Need review
			//CopyFile(ffile_backup, ffile, false);
		}
		else
		{
			nbWritten = write(open_handle,bootcontents_out, strlen(bootcontents_out));

			close(open_handle);
		}

	}
	else if (changed2)
	{
		lseek(open_handle, 0, SEEK_SET);
		nbWritten = write(open_handle,bootcontents_out,strlen(bootcontents_out));
		
	}

	if (open_handle != -1) 
		close(open_handle);


	if (changed1 || changed2)
		return CMDFCPREP_SUCCESS;
	else
		return CMDFCPREP_NOCHANGE;
}

/****************************************************************************
 * Method:	isStopped
 * Description: Check if thread is stopped or not
 * Param [in]: N/A
 * Param [out]: N/A
 * Return: N/A
 *****************************************************************************
 */
bool OCS_IPN_Thread::isStopped() const
{
	return m_stop;
}
