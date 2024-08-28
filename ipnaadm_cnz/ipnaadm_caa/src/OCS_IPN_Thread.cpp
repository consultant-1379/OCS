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
// OCS_IPN_Thread.cpp
//
// DESCRIPTION 
//
// DOCUMENT NO
// 6/190 55-CAA 109 0392
//
// AUTHOR 
// 0000912 EPA/D/U Michael Bentley
//
//******************************************************************************
// *** Revision history ***
// 0000912 MJB Created 
//******************************************************************************


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "OCS_IPN_Thread.h"

#define INC_SYSLOG

#ifdef INC_SYSLOG
  #include "CPS_syslog.h"
#endif

#ifdef _DEBUG
 #define DEBUGPRINT
#endif


//******************************************************************************
// OCS_IPN_Thread
//
// Thread handling connections to server
//
// INPUT:  -
// OUTPUT:  
// OTHER:  -
//******************************************************************************


DWORD OCS_IPN_Thread(LPDWORD param)
{
   struct thread_param *thread_params;
   struct thread_data thread_data;
   SOCKET thd_sock;
   int    result;
   DWORD  nwrote,nread,lresult;
   char   signal;
   int    rest,nget,i,remain;
   int    dumpavail,code,state,substate;
   int    ipnano,path_len;
   int    dest_ipna;
   char * listp;


   HANDLE ThreadWaitObjects[NO_OF_THREAD_EVENTS]; // Wait object array for thread
   HANDLE ThreadSockEvents,hLogFile;
   HANDLE fileHandle;
   HANDLE dummy_event;                            // Dummy event (never signalled)

   WIN32_FIND_DATA findData;
 
   
   char ch;

   char send_buf[SENDBUFSIZE];                    // Buffer for sending to socket
   char rec_buf[6];                               // Buffer for receiving signal header from socket
   char *data_buf;                                // Buffer for receiving signal data from socket

   char filenam[256];                             // Place to create dumpfile name
   char filepath[256];                            // Full path to logfile
   int numfile;                                   // Number of files found in dump maintenance
   int nothreadobj;                               // Number of thread wait objects
   int sig_len;                                   // The length of a signal

   WSANETWORKEVENTS sock_events;                  // Socket network events

   SYSTEMTIME st;                                 // System time structure 

   bool hbts;                                     // Heart beat time supervision in progress
   DWORD timeout;                                 // Timeout for WMO used for heartbeat
   int ipna_version;                              // sig_ver from ID conf tells ipna version 

   #ifdef DEBUGPRINT
      GetLocalTime(&st);  // Get current time 
      sprintf(LogMsg,"Started thread %d, %d threads connected at %2d:%02d:%02d.%d\r\n",no_conns,curr_cons,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);
      DoMsg(LogMsg);
   #endif


   hbts = false;                                  // Not time supervising heartbeat
   timeout = INFINITE; 
// timeout = CON_TIMEOUT;                         // WMO timeout set to 5 minutes, initially
   ipna_version = 0;                              // IPNA can't heartbeat unless it says otherwise

   InitializeCriticalSection(&critsec);           // A critical section is required to manage dump file access

   ipnano = 128;                                  // Start with unused value
   thread_data.ipnano = ipnano;

   thread_params = (struct thread_param *)param;  // Convert void pointer to struct pointer
   thd_sock = thread_params->thread_sock;         // Make a local copy of socket
   thread_data.thd_sock = thd_sock;

   // Create a dummy event to fill the command and reply arrays

   dummy_event = CreateEvent(NULL,FALSE,FALSE,NULL);
   thread_data.dummy_event = dummy_event;

   ThreadWaitObjects[THREADTERMEVENT] = ThreadTermEvent;       // Add ThreadTermEvent to wait objects
   ThreadSockEvents = WSACreateEvent();                        // Create an event object for listen socket events

   ThreadWaitObjects[MULTICONTERMEVENT] = dummy_event;         // Add MultiConTerm to wait objects

   ThreadWaitObjects[THREADSOCKEVENTS] = ThreadSockEvents;     // Place event in main object array 
   WSAEventSelect(thd_sock,ThreadSockEvents,FD_READ|FD_CLOSE); // Add socket events to wait objects

   thread_data.ThreadSockEvents = ThreadSockEvents;

   nothreadobj = 3;                                            // Enable these 2 wait objects



   // send IDENTITYREQ to socket

   ((struct sig_head *)send_buf)->sig_len = htons(SL_IDREQ);
   ((struct sig_head *)send_buf)->sig_num = IDENTITYREQ;
   ((struct sig_head *)send_buf)->sig_ver = 0;

   if(!makeitsend(thd_sock,send_buf,SL_IDREQ+sizeof(struct sig_head),"IDENTITYREQ",ipnano))
      kill_thread(8,&thread_data);              // Clean up and die
   while(true)
   {
      // Wait for events in wait object array 

      result = WaitForMultipleObjects(nothreadobj,ThreadWaitObjects,FALSE,timeout);

      #ifdef DEBUGPRINT
         sprintf(LogMsg,"IPNA %d :WMO returned %d\r\n",ipnano,result);
         DoMsg(LogMsg);
//		 sprintf(LogMsg,"WAIT_FAILED = %d, WAIT_ABANDONED = %d, WAIT_OBJECT_0 = %d, WAIT_TIMEOUT = %d\n",
//			             WAIT_FAILED,WAIT_ABANDONED,WAIT_OBJECT_0,WAIT_TIMEOUT);
		 DoMsg(LogMsg);
      #endif

      switch(result)
      {
         case (WAIT_OBJECT_0 + THREADTERMEVENT):
         {
            #ifdef DEBUGPRINT
               sprintf(LogMsg,"IPNA %d received THREADTERMEVENT\r\n",ipnano);
               DoMsg(LogMsg);
            #endif 
 
//          ResetEvent(ThreadTermEvent);                 // OK, I am doing it 
            kill_thread(0,&thread_data);                 // Clean up and die

            break;
         }

         case (WAIT_OBJECT_0 + THREADSOCKEVENTS):
         {
            #ifdef DEBUGPRINT
               sprintf(LogMsg,"IPNA %d received THREADSOCKETEVENT\r\n",ipnano);
               DoMsg(LogMsg);
            #endif

            if(WSAEnumNetworkEvents(thd_sock,ThreadSockEvents,&sock_events) == SOCKET_ERROR)
            {
               #ifdef DEBUGPRINT
                  sprintf(LogMsg,"IPNA %d :WSAEnumNetworkEvents failed with error %d\r\n",ipnano,WSAGetLastError());
                  DoMsg(LogMsg);
               #endif

			   kill_thread(5,&thread_data);              // Clean up and die
            }

            if(sock_events.lNetworkEvents & FD_CLOSE)
            {

               // Socket closed event

               // Clean up global variables and events and exit thread

               #ifdef DEBUGPRINT
                  sprintf(LogMsg,"IPNA %d received FD_CLOSE event\r\n",ipnano);
                  DoMsg(LogMsg);
               #endif

			   kill_thread(0,&thread_data);              // Clean up and die
            }          
            else if(sock_events.lNetworkEvents & FD_READ)
            {
               #ifdef DEBUGPRINT
                  sprintf(LogMsg,"IPNA %d received FD_READ event\r\n",ipnano);
                  DoMsg(LogMsg);
               #endif

               // receive the signal header from IPNA 
 
               if(!recvitall(thd_sock,rec_buf,sizeof(struct sig_head),"Signal Header",0))
                 kill_thread(5,&thread_data);              // Clean up and die

			   signal = ((struct sig_head *)rec_buf)->sig_num;
               switch(signal)
               {
                  // Signal from either IPNAs or command handler(s)

                  case IDENTITYCONF:
                  {
                     #ifdef DEBUGPRINT
                        sprintf(LogMsg,"IPNA %d received IDENTITYCONF\r\n",ipnano);
                        DoMsg(LogMsg);
                     #endif

                     sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);
                     if(sig_len != SL_IDENTITYCONF)
                     {
                        // The signal length received is not correct, throw it away and die

                        kill_thread(102,&thread_data);              // Clean up and die
                     }

					 ipna_version = ((struct sig_head *)rec_buf)->sig_ver; // Get IPNA version

                     #ifdef DEBUGPRINT
                       if(ipna_version == CAN_HEARTBEAT)
                       {
                           sprintf(LogMsg,"IPNA %d can heartbeat\r\n",ipnano);
                           DoMsg(LogMsg);
                       }
                       else
                       {
                           sprintf(LogMsg,"IPNA %d cannot heartbeat\r\n",ipnano);
                           DoMsg(LogMsg);
                       }
                     #endif       

                     // read identity from signal

                     result = recv(thd_sock,&ch,1,0);

                     if(result == SOCKET_ERROR || result != 1)
                     {
                        #ifdef DEBUGPRINT
                           sprintf(LogMsg,"Read of id failed with WSA error %d\r\n",WSAGetLastError());
                           DoMsg(LogMsg); 
                        #endif

                        kill_thread(103,&thread_data);              // Clean up and die
                     }
 
                     ipnano = (int)ch & 255;
                     thread_data.ipnano = ipnano;

                     if(ipnano == ID_COMMAND)               // Test for command handler
                     {
                        // ID from IDENTITYCONF signal indicates connection from command handler

                        ResetEvent(MultiCmdTermEvent);      // Reset event in case it's still active

                        ThreadWaitObjects[MULTICONTERMEVENT] = MultiCmdTermEvent;   // Add MultiCmdTerm to wait objects
                        command_connected++;               // Indicate command handler connected
                        if(command_connected > 1)
                           SetEvent(MultiCmdTermEvent);    // Order termination of all command threads 

                        // Since I'm a cmd handler, I won't receive command events

                        ThreadWaitObjects[COMMANDEVENT] = dummy_event;
                        thread_data.Command_Event = dummy_event;
                        ThreadWaitObjects[REPLYEVENT] = ipna_replied_event; 
                        nothreadobj = 5;                    // Make events visable to WaitForMultipleObjects
       
                        // WaitMultipleObjects gets upset if object doesn't have valid handle
                        // so go through the connected matrix and set unused positions to a dummy
                        // event which is never signalled. New IPNA connections will enter these
                        // events when they connect, and remove them when disconnecting. 

                        EnterCriticalSection(&ipnasec);
                        for(i=0;i<MAX_IPNAS;i++)
                        {
                           Replied[i] = false;              // Clear any outstanding IPNA replies
                        }
                        LeaveCriticalSection(&ipnasec);

                     }
                     else if((ipnano >=0) && (ipnano < MAX_IPNAS)) // Test valid IPNA id range
                     {
                        // ID from IDENTITYCONF signal indicates an IPNA

                        EnterCriticalSection(&ipnasec);
                        ResetEvent(MultiConTermEvent[ipnano]); // Reset event in case it's still active
                        ThreadWaitObjects[MULTICONTERMEVENT] = MultiConTermEvent[ipnano];   // Add MultiConTerm to wait objects

                        connected[ipnano]++;                   // Indicate IPNA connected
                        if(connected[ipnano] >1)               // Check for multiple connections
                        {
                           SetEvent(MultiConTermEvent[ipnano]);     // Kill multiple connections
                        }

                        Command_Data[ipnano] = 0;           // Clear command data
                        Reply_Data[ipnano] = 0;             // Clear reply data
                        Replied[ipnano] = false;            // Clear replied indicator

                        LeaveCriticalSection(&ipnasec);

                        // Create command and reply events (auto reset)


                        Command_Events[ipnano] = CreateEvent(NULL,FALSE,FALSE,NULL);
                        ThreadWaitObjects[COMMANDEVENT] = Command_Events[ipnano];   // Add command event to wait objects

                        thread_data.Command_Event = Command_Events[ipnano];

                        ThreadWaitObjects[REPLYEVENT] = dummy_event; // IPNAs don't handle this

                        nothreadobj = 5;                    // Make these objects visable in WaitForMultipleObjects

                        // Send DUMPAVAILREQ to IPNA

                        ((struct sig_head *)send_buf)->sig_len = htons(SL_DAVAIL);
                        ((struct sig_head *)send_buf)->sig_num = DUMPAVAILREQ;
                        ((struct sig_head *)send_buf)->sig_ver = 0;

                        if(!makeitsend(thd_sock,send_buf,SL_DAVAIL+sizeof(struct sig_head),"DUMPAVAILREQ",ipnano))
                           kill_thread(8,&thread_data);              // Clean up and die
                     }
                     else
                     {
                        #ifdef DEBUGPRINT
                           sprintf(LogMsg,"Invalid ID\r\n");
                           DoMsg(LogMsg); 
                        #endif
                        kill_thread(104,&thread_data);              // Clean up and die
                     }

                     break;
                  }

                 // Signals from IPNAs only

                  case DUMPAVAILCONF:
                  {
                     #ifdef DEBUGPRINT
                         sprintf(LogMsg,"IPNA %d received DUMPAVAILCONF\r\n",ipnano);
                         DoMsg(LogMsg);
                     #endif

                     sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);
                     if(sig_len != SL_DACONF)
                     {
                        // The signal length received is not correct, throw it away and die

                        kill_thread(105,&thread_data);              // Clean up and die

                     }
                     if((ipnano >=0) && (ipnano < MAX_IPNAS)) // If valid IPNA id range
                     {

                        // Check fetch dump avail result and proceed if dump is avail

                        result = recv(thd_sock,&ch,1,0);
                        dumpavail = (int)ch;
                        if(result == SOCKET_ERROR || result != 1)
                        {
                           #ifdef DEBUGPRINT
                              sprintf(LogMsg,"Read of result of DUMPAVAILCONF failed with WSA error %d\r\n",WSAGetLastError());
                              DoMsg(LogMsg);
                           #endif

                           kill_thread(106,&thread_data);              // Clean up and die
                        }
                        else if( dumpavail == DUMPAVAIL)
                        {
                           if(!CreateDirectory(LOGPATH,0))
                           {
                              if((result=GetLastError()) != ERROR_ALREADY_EXISTS)
                              {
                                 #ifdef DEBUGPRINT
                                    sprintf(LogMsg,"Create directory %s failed with error %d\r\n",LOGPATH,GetLastError());
                                    DoMsg(LogMsg);
                                 #endif

                                 kill_thread(107,&thread_data);    // Clean up and die 
                              }
                           }
                           // Create unique filename based on ipnano, date and time

                           GetLocalTime(&st);                      // Get local time

                           sprintf(filenam,"%sipna%02d-%02d%02d%02d-%02d%02d%02d.oselog",LOGPATH,ipnano,st.wYear,st.wMonth,
                                  st.wDay,st.wHour,st.wMinute,st.wSecond);

                           EnterCriticalSection(&critsec); // Avoid problems with directory lists

                           hLogFile = CreateFile(filenam,GENERIC_WRITE,0,0,CREATE_NEW,0,0);
                           if(hLogFile == INVALID_HANDLE_VALUE)
                           {
                              if((result = GetLastError()) == ERROR_FILE_EXISTS)
                              {
                                 #ifdef DEBUGPRINT
                                    sprintf(LogMsg,"File %s already exists!\r\n",filenam);
                                    DoMsg(LogMsg);
                                 #endif

                                 LeaveCriticalSection(&critsec);
                                 kill_thread(108,&thread_data);              // Clean up and die
                              }
                              #ifdef DEBUGPRINT
                                 sprintf(LogMsg,"Creation of file %s filed with error %d\r\n",filenam,result);
                                 DoMsg(LogMsg);
                              #endif

                              LeaveCriticalSection(&critsec);
                              kill_thread(109,&thread_data);              // Clean up and die
                           }
                           LeaveCriticalSection(&critsec);

                           // Send FETCHDUMPREQ

                           ((struct sig_head *)send_buf)->sig_len = htons(SL_FDUMP);
                           ((struct sig_head *)send_buf)->sig_num = FETCHDUMPREQ;
                           ((struct sig_head *)send_buf)->sig_ver = 0;

                           if(!makeitsend(thd_sock,send_buf,SL_FDUMP+sizeof(struct sig_head),"FETCHDUMPREQ",ipnano))
                              kill_thread(8,&thread_data);              // Clean up and die 

                        }
						else if((dumpavail != DUMPAVAIL) && (ipna_version == CAN_HEARTBEAT))
						{
						   // No dump is available and IPNA can use heartbeat mechanism

						   timeout = HEARTBEAT_TIMEOUT;  // Set WaitForMultipleObjects timeout
						   hbts = true;                  // Enable heartbeat supervision

						   // Send heartbeat signal to ipna

                           ((struct sig_head *)send_buf)->sig_len = htons(SL_HBREQ);
                           ((struct sig_head *)send_buf)->sig_num = HEARTBEATREQ;
                           ((struct sig_head *)send_buf)->sig_ver = 0;

                           if(!makeitsend(thd_sock,send_buf,SL_HBREQ+sizeof(struct sig_head),"HEARTBEATREQ",ipnano))
                              kill_thread(8,&thread_data);              // Clean up and die

						}

                        if(ipna_version != CAN_HEARTBEAT)
                          timeout = INFINITE;             // Set WMO timeout to infinite
                        
                     }
                     else 
                     {
                        #ifdef DEBUGPRINT
                           sprintf(LogMsg,"DUMPAVAILCONF received by command handler!\r\n");
                           DoMsg(LogMsg);
                        #endif

                        kill_thread(110,&thread_data);              // Clean up and die
                     }
                     break;
                  }

                  case SENDDUMPDATA:
                  {
                     #ifdef DEBUGPRINT
                        sprintf(LogMsg,"IPNA %d received SENDDUMPDATA\r\n",ipnano);
                        DoMsg(LogMsg);
                     #endif

                     if((ipnano >=0) && (ipnano < MAX_IPNAS)) // If valid IPNA id range
                     {
                        data_buf = (char *)malloc(DBUFSIZE);   // Do this to reduce run time memory usage

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

                       
                           if(!recvitall(thd_sock,data_buf,nget,"Dump data",0))
                           {
                              free(data_buf);        // free_data_buf
                              CloseHandle(hLogFile);    // Close file
                              kill_thread(111,&thread_data);              // Clean up and die
                           }
                           else
                           {
                              // Write data to file and check success
                              if(!WriteFile(hLogFile,data_buf,nget,&nwrote,0))
                              {
                                 #ifdef DEBUGPRINT
                                    sprintf(LogMsg,"Write to file %s failed with error %d\r\n",filenam,GetLastError());
                                    DoMsg(LogMsg);
                                 #endif

                                 CloseHandle(hLogFile); // Close file
                                 free(data_buf);        // free_data_buf
                                 kill_thread(112,&thread_data);              // Clean up and die
                              }
                           }
                        }

                        free(data_buf);           // De-allocate buffer memory
                        CloseHandle(hLogFile);    // Close file

                        //  Perform dump file maintenance - remove old files if more than 64 dump files
     
                        EnterCriticalSection(&critsec);
                        if(!SetCurrentDirectory(LOGPATH))  // Change to the directory containing the logs
                        {
                           #ifdef DEBUGPRINT
                              sprintf(LogMsg,"SetCurrentDirectory to %s failed with error %d\n",LOGPATH,GetLastError());
                              DoMsg(LogMsg);
                           #endif
                        }
                        else
                        {
                           numfile = 0;
                           filenam[0] = '\0';  
                           fileHandle = FindFirstFile("*.oselog",&findData); // Find first osdump file in LOGPATH
                           while(fileHandle != INVALID_HANDLE_VALUE)         // Continue for all osdump files in LOGPATH
                           {
                               numfile++;                                    // One more file found
                               if(file_older(filenam,findData.cFileName))    // Compare file names to get oldest
                                  strcpy(filenam,findData.cFileName);        // Found file becomes oldest file                               }

                               if(!FindNextFile(fileHandle,&findData))       // Find next osedump file in LOGPATH 
                                  break;                                     // Done if no other files found
                           }
                           if(numfile>63)
                           {
                              DeleteFile(filenam);                           // Delete oldest file found if more than 63 of them
                              #ifdef DEBUGPRINT
                                 sprintf(LogMsg,"Deleting file %s \n",filenam);
                                 DoMsg(LogMsg);
                              #endif
                           }
                           FindClose(fileHandle);
                        }  
                        LeaveCriticalSection(&critsec);

						if(ipna_version == CAN_HEARTBEAT)
						{
						   // IPNA can use heartbeat mechanism

						   timeout = HEARTBEAT_TIMEOUT;  // Set WaitForMultipleObjects timeout
						   hbts = true;                  // Enable heartbeat supervision

						   // Send heartbeat signal to ipna

                           ((struct sig_head *)send_buf)->sig_len = htons(SL_HBREQ);
                           ((struct sig_head *)send_buf)->sig_num = HEARTBEATREQ;
                           ((struct sig_head *)send_buf)->sig_ver = 0;

                           if(!makeitsend(thd_sock,send_buf,SL_HBREQ+sizeof(struct sig_head),"HEARTBEATREQ",ipnano))
                              kill_thread(8,&thread_data);              // Clean up and die
						}
                     }
                     else 
                     {
                        #ifdef DEBUGPRINT
                           sprintf(LogMsg,"SENDDUMPDTA received by command handler!\r\n");
                           DoMsg(LogMsg);
                        #endif

                        kill_thread(113,&thread_data);              // Clean up and die
                     }
                     break;
                  }

				  case HEARTBEATCONF:
				  {
                     #ifdef DEBUGPRINT
                        sprintf(LogMsg,"IPNA %d received HEARTBEATCONF\r\n",ipnano);
                        DoMsg(LogMsg);
                     #endif

                     sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);
                     if(sig_len != SL_HBCONF)
                     {
                        // The signal length received is not correct, throw it away and die

                        kill_thread(1135,&thread_data);              // Clean up and die
                     } 

                     hbts = false;     // Let WMO timeout so new heartbeat request can be sent 
                     break;
				  }

                  case SETIPNASTATECONF:
                  {
                     #ifdef DEBUGPRINT
                        sprintf(LogMsg,"IPNA %d received SETIPNASTATECONF\r\n",ipnano);
                        DoMsg(LogMsg);
                     #endif

                     sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);
                     if(sig_len != SL_SSCONF)
                     {
                        // The signal length received is not correct, throw it away and die

                        kill_thread(114,&thread_data);              // Clean up and die
                     }
                     if((ipnano >=0) && (ipnano < MAX_IPNAS)) // If valid IPNA id range
                     {
                        result = recv(thd_sock,&ch,1,0);   // Receive result code 
                        code = (int)ch;
                        if(result == SOCKET_ERROR || result != 1)
                        {
                           #ifdef DEBUGPRINT
                              sprintf(LogMsg,"Read of result of SETIPNASTATECONF failed with WSA error %d\r\n",WSAGetLastError());
                              DoMsg(LogMsg);
                           #endif

                           kill_thread(115,&thread_data);              // Clean up and die
                        }
                        else
                        {
                           // Send reply

                           Reply_Data[ipnano] = code; 
                           Replied[ipnano]=TRUE;          // Indicate reply waiting 
                           SetEvent(ipna_replied_event);  // Inform command thread of reply
                        }
                     }
                     else 
                     {
                        #ifdef DEBUGPRINT
                           sprintf(LogMsg,"SETIPNASTATECONF received by command handler!\r\n");
                           DoMsg(LogMsg);
                        #endif

                        kill_thread(116,&thread_data);              // Clean up and die
                     }
                     break;
                  }

// -------------------------------------------------------------------------------------
                  // Signals from command handler(s) only

// -------------------------------------------------------------------------------------

                  // Set IPNA state

                  case CMDSSREQ:
                  {
                     #ifdef DEBUGPRINT
                        sprintf(LogMsg,"IPNA %d received CMDSSREQ\r\n",ipnano);
                        DoMsg(LogMsg);
                     #endif

                     sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);
                     if(sig_len != SL_CMDSSREQ)
                     {
                        // The signal length received is not correct, throw it away and die

                        kill_thread(117,&thread_data);              // Clean up and die

                     } 
                     if(ipnano == ID_COMMAND)               // Test for command handler, ignore if not
                     {
                        // Read dest ipna, state, sub-state from signal

                        result = recv(thd_sock,&ch,1,0);   // Receive destination IPNA from signal
                        if(result == SOCKET_ERROR || result != 1)
                        {
                           #ifdef DEBUGPRINT
                              sprintf(LogMsg,"Read of CMDSSREQ signal data failed with WSA error %d\r\n",WSAGetLastError());
                              DoMsg(LogMsg);
                           #endif

                           kill_thread(118,&thread_data);              // Clean up and die
                        }
                        dest_ipna = (int)ch;

                        result = recv(thd_sock,&ch,1,0);  // Receive state from signal
                        if(result == SOCKET_ERROR || result != 1)
                        {
                           #ifdef DEBUGPRINT
                              sprintf(LogMsg,"Read of CMDSSREQ signal data failed with WSA error %d\r\n",WSAGetLastError());
                              DoMsg(LogMsg);
                           #endif

                           kill_thread(119,&thread_data);              // Clean up and die
                        }
                        state = (int)ch;

                        result = recv(thd_sock,&ch,1,0);  // Receive sub-state from signal
                        if(result == SOCKET_ERROR || result != 1)
                        {
                           #ifdef DEBUGPRINT
                              sprintf(LogMsg,"Read of CMDSSREQ signal data failed with WSA error %d\r\n",WSAGetLastError());
                              DoMsg(LogMsg);
                           #endif

                           kill_thread(120,&thread_data);              // Clean up and die
                        }
                        substate = (int)ch;
 
                        if(connected[dest_ipna]==0)  // Send failure if ipna not connected
                        {
                          // Send CMSSCONF back to command handler with result from Reply_Data[i]

                          ((struct cmdssconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDSSCONF);
                          ((struct cmdssconf_sig *)send_buf)->sh.sig_num = CMDSSCONF;
                          ((struct cmdssconf_sig *)send_buf)->sh.sig_ver = 0;
                          ((struct cmdssconf_sig *)send_buf)->result = CMDSS_NOTCON;

                          if(!makeitsend(thd_sock,send_buf,SL_CMDSSCONF+sizeof(struct sig_head),"CMDSSCONF",ipnano))
                             kill_thread(8,&thread_data);              // Clean up and die  
                        }

                        // set Command_Data[dest ipnano]
                        Command_Data[dest_ipna] = 0;

                        // This is a bit simple as both IPNA_STATE_SEP and IPNA_SUBSTATE_LOCK are 1 and their opposites 
                        // are 0. STATE_SEP and SUBSTATE_LOCK are bit positions. Woe betide he who changes this

                        if(state == IPNA_STATE_SEP)
                           Command_Data[dest_ipna] |= STATE_SEP;

                        if(substate == IPNA_SUBSTATE_LOCK)
                           Command_Data[dest_ipna] |= SUBSTATE_LOCK;

                        // Set Command_Event[dest ipnano]

                        SetEvent(Command_Events[dest_ipna]);  // Tell thread for destination IPNA to send SETIPNASTATEREQ signal
                     }
                     else 
                     {
                        #ifdef DEBUGPRINT
                           sprintf(LogMsg,"CMDSSREQ received by ipna handler!\r\n");
                           DoMsg(LogMsg);
                        #endif

                        kill_thread(121,&thread_data);              // Clean up and die
                     }

                     break;
                  }

// -------------------------------------------------------------------------------------

                  // List connected IPNAs

                  case CMDLISTREQ:
                  {
                     #ifdef DEBUGPRINT
                        sprintf(LogMsg,"IPNA %d received CMDLISTREQ\r\n",ipnano);
                        DoMsg(LogMsg);
                     #endif

                     sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);
                     if(sig_len != SL_CMDLIST)
                     {
                        // The signal length received is not correct, throw it away and die

                        kill_thread(122,&thread_data);              // Clean up and die

                     }
                     if(ipnano == ID_COMMAND)               // Test for command handler
                     {
                        // Send CMDLISTCONF back to command handler with list of connected IPNAs

                        ((struct cmdlistconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDLISTCONF);
                        ((struct cmdlistconf_sig *)send_buf)->sh.sig_num = CMDLISTCONF;
                        ((struct cmdlistconf_sig *)send_buf)->sh.sig_ver = 0;
                        listp = &((struct cmdlistconf_sig *)send_buf)->list[0]; // Pointer to start of list
                        for(i=0; i<2; i++)
						{
							if (connectToCPon14000(i) == true)
								*listp++ = 1;				// Return element of connected array
							else
								*listp++ = 0;
						}
						for(i=2; i<MAX_IPNAS; i++)
						{
							*listp++ = (char) connected[i];	// Return element of connected array
						}

                        if(!makeitsend(thd_sock,send_buf,SL_CMDLISTCONF+sizeof(struct sig_head),"CMDLISTCONF",ipnano))
                           kill_thread(8,&thread_data);              // Clean up and die

                     }
                     else 
                     {
                        #ifdef DEBUGPRINT
                           sprintf(LogMsg,"CMDLISTREQ received by ipna handler!\r\n");
                           DoMsg(LogMsg);
                        #endif

                     }

                     kill_thread(253,&thread_data);              // Clean up and die
                     break;
                  }

// -------------------------------------------------------------------------------------

                  // Handle OSEDUMP commands

                  case CMDOSDREQ:
                  {
                     #ifdef DEBUGPRINT
                        sprintf(LogMsg,"IPNA %d received CMDOSDREQ\r\n",ipnano);
                        DoMsg(LogMsg);
                     #endif

                     if(ipnano == ID_COMMAND)               // Test for command handler
                     {
                        result = recv(thd_sock,&ch,1,0);   // Receive dump sub-command from signal
                        if(result == SOCKET_ERROR || result != 1)
                        {
                           #ifdef DEBUGPRINT
                              sprintf(LogMsg,"Read of CMDOSDREQ signal data failed with WSA error %d\r\n",WSAGetLastError());
                              DoMsg(LogMsg);
                           #endif

                           kill_thread(124,&thread_data);              // Clean up and die
                        }
                        switch(ch)
                        {
                           case CMDOSD_DIR:
                           {
                              #ifdef DEBUGPRINT
                                 sprintf(LogMsg,"IPNA %d received CMDOSD_DIR\r\n",ipnano);
                                 DoMsg(LogMsg);
                              #endif
                              sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);
                              if(sig_len != SL_CMDOSD_DIR)
                              {
                                 // The signal length received is not correct, throw it away and die

                                 kill_thread(125,&thread_data);              // Clean up and die
                              }
                              // Send CMDOSDCONF back to command handler with dump directory in a string

                              ((struct cmdosdconf_sig *)send_buf)->sh.sig_num = CMDOSDCONF;
                              ((struct cmdosdconf_sig *)send_buf)->sh.sig_ver = 0;
                              ((struct cmdosdconf_sig *)send_buf)->cmd = CMDOSD_DIR;  // Indicate which sub-command rely comes from
                              ((struct cmdosdconf_sig *)send_buf)->result = 0;        // No error

                              // The directory where ose dumps are placed is in the constant LOGPATH

                              path_len = strlen(LOGPATH);     // Length of the LOGPATH 
                              strcpy(&((struct cmdosdconf_sig *)send_buf)->data,LOGPATH); 

                              ((struct cmdosdconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDOSD_DIRR+path_len);

                              if(!makeitsend(thd_sock,send_buf,SL_CMDOSD_DIRR+sizeof(struct sig_head)+path_len,"CMDOSDCONF",ipnano))
                                 kill_thread(8,&thread_data);              // Clean up and die
                              break;
                           }

// -------------------------------------------------------------------------------------

                           case CMDOSD_LIST:
                           {
                              #ifdef DEBUGPRINT
                                 sprintf(LogMsg,"IPNA %d received CMDOSD_LIST\r\n",ipnano);
                                 DoMsg(LogMsg);
                              #endif

                              sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);
                              if(sig_len != SL_CMDOSD_LIST)
                              {
                                 // The signal length received is not correct, throw it away and die

                                 kill_thread(126,&thread_data);              // Clean up and die
                              }
                              // Search for osedump files in LOGPATH and return in CMDOSDCONF signal

                              ((struct cmdosdconf_sig *)send_buf)->sh.sig_num = CMDOSDCONF;
                              ((struct cmdosdconf_sig *)send_buf)->sh.sig_ver = 0;
                              ((struct cmdosdconf_sig *)send_buf)->cmd = CMDOSD_LIST;  // Indicate which sub-command rely comes from
                              ((struct cmdosdconf_sig *)send_buf)->result = CMDOSD_SUCCESS; // No error

                              if(!SetCurrentDirectory(LOGPATH))                       // Change to the directory containing the logs
                              {
                                 ((struct cmdosdconf_sig *)send_buf)->result = CMDOSD_FAIL; // Return error value
                                 ((struct cmdosdconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDOSD_LISTR); // Signal length
                                 if(!makeitsend(thd_sock,send_buf,SL_CMDOSD_LISTR+sizeof(struct sig_head),"CMDOSDCONF",ipnano))
                                    kill_thread(8,&thread_data);              // Clean up and die
                              }
                              else
                              {
                                 EnterCriticalSection(&critsec);    // Prevent ipna threads creating new files until list finished
                                 listp = &((struct cmdosdconf_sig *)send_buf)->data;// Pointer to start of file list data
                                 *listp = '\0';                                     // Empty string
                                 fileHandle = FindFirstFile("*.oselog",&findData);  // Search cwd for first osedump file
                                 while(fileHandle != INVALID_HANDLE_VALUE)
                                 {
                                    strcat(listp,findData.cFileName);               // Add found filename to list in signal data
                                    if(!FindNextFile(fileHandle,&findData))         // Look for the next file
                                       break;                                       // End while if no more files found
                                    else
                                    {
                                       strcat(listp,"\r\n");                        // Add a new line seperator and get the next name
                                    }
                                 }
                                 strcat(listp,"\r\n");
                                 LeaveCriticalSection(&critsec);
                                 FindClose(fileHandle);

                                 remain = SL_CMDOSD_LISTR+strlen(listp)+1;   // Signal length + length of list string (including terminating 0)
                                 ((struct cmdosdconf_sig *)send_buf)->sh.sig_len = htons(remain); // Signal length

                                 if(!makeitsend(thd_sock,send_buf,remain+sizeof(struct sig_head),"CMDOSDCONF",ipnano))      // Send the CMDOSDCONF signal
                                     kill_thread(8,&thread_data);              // Clean up and die 
                              }
                              break;
                           }

// -------------------------------------------------------------------------------------

                           case CMDOSD_GET:
                           {
                              #ifdef DEBUGPRINT
                                 sprintf(LogMsg,"IPNA %d received CMDOSD_GET\r\n",ipnano);
                                 DoMsg(LogMsg);
                              #endif

                              sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);
                   
                              // Open the file in the signal and send it back in the reply
                              // N.B. Dump files are about 8K in length and will fit into
                              // the 64k send buffer

                              ((struct cmdosdconf_sig *)send_buf)->sh.sig_num = CMDOSDCONF;
                              ((struct cmdosdconf_sig *)send_buf)->sh.sig_ver = 0;
                              ((struct cmdosdconf_sig *)send_buf)->cmd = CMDOSD_GET;  // Indicate which sub-command rely comes from
                              ((struct cmdosdconf_sig *)send_buf)->result = CMDOSD_SUCCESS; // No error

  
                              if(!SetCurrentDirectory(LOGPATH))                       // Change to the directory containing the logs
                              {
                                 ((struct cmdosdconf_sig *)send_buf)->result = CMDOSD_FAIL;       // Return error value
                                 ((struct cmdosdconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDOSD_GETR); // Signal length

                                 if(!makeitsend(thd_sock,send_buf,SL_CMDOSD_GETR+sizeof(struct sig_head),"CMDOSDCONF",ipnano))
                                    kill_thread(8,&thread_data);              // Clean up and die
                              }
                              else
                              {

                                 // Receive rest of signal -> filename

                                 recvitall(thd_sock,filenam,sig_len - SL_CMDOSD_GET ,"CMDOSD get filename",ipnano);
 
                                 filenam[sig_len - SL_CMDOSD_GET] = '\0';  // Terminate string
                                 strcpy(filepath,LOGPATH);    // Full path to file starts with log directory
                                 strcat(filepath,filenam);    // Add file name

                                 #ifdef DEBUGPRINT
                                    sprintf(LogMsg,"IPNA %d getting file %s\r\n",ipnano,filenam);
                                    DoMsg(LogMsg);
                                    sprintf(LogMsg,"IPNA %d file with path %s\r\n",ipnano,filepath);
                                    DoMsg(LogMsg);
                                 #endif

                                 if((fileHandle = CreateFile(filepath,GENERIC_READ,0,0,OPEN_EXISTING,0,0)) == INVALID_HANDLE_VALUE)
                                 {
                                    #ifdef DEBUGPRINT
                                       sprintf(LogMsg,"IPNA %d Open of file %s failed with error %d\r\n",ipnano,filepath,GetLastError());
                                       DoMsg(LogMsg);
                                    #endif
                                    CloseHandle(fileHandle);
                                    ((struct cmdosdconf_sig *)send_buf)->result = CMDOSD_FAIL;       // File doesn't exist, return error value
                                    ((struct cmdosdconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDOSD_GETR); // Signal length
                                    if(!makeitsend(thd_sock,send_buf,SL_CMDOSD_GETR+sizeof(struct sig_head),"CMDOSDCONF",ipnano))
                                       kill_thread(8,&thread_data);              // Clean up and die
                                 }
                                 else
                                 {
                                    listp = &((struct cmdosdconf_sig *)send_buf)->data;     // Pointer to start of data in return signal

                                    // Read whole file into send buffer, ask for bufsize -sig length up to now

                                    if(!ReadFile(fileHandle,listp,SENDBUFSIZE-SL_CMDOSD_GETR-sizeof(struct sig_head),&lresult,0))
                                    {
                                       CloseHandle(fileHandle);
                                       ((struct cmdosdconf_sig *)send_buf)->result = CMDOSD_FAIL;       // Read file failed, return error value
                                       ((struct cmdosdconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDOSD_GETR); // Signal length

                                       if(!makeitsend(thd_sock,send_buf,SL_CMDOSD_GETR+sizeof(struct sig_head),"CMDOSDCONF",ipnano))
                                          kill_thread(8,&thread_data);              // Clean up and die
                                    }
                                    else
                                    {
                                       CloseHandle(fileHandle);                        
                                       ((struct cmdosdconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDOSD_GETR +(int)lresult); // Signal length

                                       if(!makeitsend(thd_sock,send_buf,SL_CMDOSD_GETR+sizeof(struct sig_head) +(int)lresult,"CMDOSDCONF",ipnano))
                                          kill_thread(8,&thread_data);              // Clean up and die  
                                    }
                                 }
                              }
                              break;
                           }

                           default:
                           {
                             #ifdef DEBUGPRINT
                                sprintf(LogMsg,"IPNA %d didn't handle signal %d in CMDOSD\r\n",ipnano,(int)ch);
                                DoMsg(LogMsg);
                             #endif
                             break;
                           }
                        }
                     }
                     else 
                     {
                        #ifdef DEBUGPRINT
                           sprintf(LogMsg,"CMDLISTREQ received by ipna handler!\r\n");
                           DoMsg(LogMsg);
                        #endif

                        kill_thread(127,&thread_data);              // Clean up and die
                     }
                     kill_thread(254,&thread_data);              // Clean up and die
                     break;
                  }
				  
// -------------------------------------------------------------------------------------

                  // Prepare for Function Change in IPNA

                  case CMDFCPREPREQ:
                  {
                     #ifdef DEBUGPRINT
                        sprintf(LogMsg,"IPNA %d received CMDFCPREPREQ\r\n",ipnano);
                        DoMsg(LogMsg);
                     #endif

                     sig_len = ntohs(((struct sig_head *)rec_buf)->sig_len);
                     if(sig_len != SL_CMDFCPREPREQ)
                     {
                        // The signal length received is not correct, throw it away and die

                        kill_thread(117,&thread_data);      // Clean up and die

                     } 
                     if(ipnano == ID_COMMAND)               // Test for command handler, ignore if not
                     {
                        // Read dest ipna from signal

                        result = recv(thd_sock,&ch,1,0);    // Receive destination IPNA from signal
                        if(result == SOCKET_ERROR || result != 1)
                        {
                           #ifdef DEBUGPRINT
                              sprintf(LogMsg,"Read of CMDFCPREPREQ signal data failed with WSA error %d\r\n",WSAGetLastError());
                              DoMsg(LogMsg);
                           #endif

                           kill_thread(118,&thread_data);   // Clean up and die
                        }
                        dest_ipna = (int)ch;


						result = prepareFunctionChange(dest_ipna);
				
							
                        // Send CMSSCONF back to command handler with result from Reply_Data[i]

                        ((struct cmdssconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDFCPREPCONF);
                        ((struct cmdssconf_sig *)send_buf)->sh.sig_num = CMDFCPREPCONF;
                        ((struct cmdssconf_sig *)send_buf)->sh.sig_ver = 0;
                        ((struct cmdssconf_sig *)send_buf)->result = result;

                        if (!makeitsend(thd_sock,
										send_buf,
										SL_CMDFCPREPCONF+sizeof(struct sig_head),
										"CMDFCPREPCONF",
										ipnano))
                            kill_thread(8,&thread_data);            // Clean up and die  
                     }
                     else 
                     {
                        #ifdef DEBUGPRINT
                           sprintf(LogMsg,"CMDFCPREPREQ received by ipna handler!\r\n");
                           DoMsg(LogMsg);
                        #endif

                        kill_thread(121,&thread_data);              // Clean up and die
                     }

                     break;
                  }

// -------------------------------------------------------------------------------------

				  // Command Debug request 

                  case CMDDEBUGREQ:
                  {
                     #ifdef DEBUGPRINT
                        sprintf(LogMsg,"IPNA %d received CMDDEBUGREQ\r\n",ipnano);
                        DoMsg(LogMsg);
                     #endif


                     if(ipnano == ID_COMMAND)               // Test for command handler
                     {
                        result = recv(thd_sock,&ch,1,0);   // Receive dump sub-command from signal
                        if(result == SOCKET_ERROR || result != 1)
                        {
                           #ifdef DEBUGPRINT
                              sprintf(LogMsg,"Read of CMDDEBUG signal data failed with WSA error %d\r\n",WSAGetLastError());
                              DoMsg(LogMsg);
                           #endif

                           kill_thread(128,&thread_data);              // Clean up and die
                        }
                        switch(ch)
                        {
                           case CMDDEBUG_START:
                           { 
                              #ifdef DEBUGPRINT 
                                 sprintf(LogMsg,"IPNA %d received debug start command %d\r\n",ipnano,(int)ch); 
                                 DoMsg(LogMsg); 
                              #endif 

                              ((struct cmddebugconf_sig *)send_buf)->result = CMDDEBUG_OK; // Succeeds unless otherwise set

                              if(!debug_file_active)
                              {
                                 // Check if directory exits, create if not

                                 if(!CreateDirectory(DEBUG_FILE_PATH,0))
                                 {
                                    if(GetLastError() == ERROR_ALREADY_EXISTS)
                                    {
                                       #ifdef DEBUGPRINT
                                          sprintf(LogMsg,"Directory %s exists\r\n",DEBUG_FILE_PATH);
                                          DoMsg(LogMsg);
                                       #endif
                                    }
                                    else
                                    {
                                       #ifdef DEBUGPRINT
                                          sprintf(LogMsg,"Create directory %s failed with error %d\r\n",DEBUG_FILE_PATH,GetLastError());
                                          DoMsg(LogMsg);
                                       #endif

                                      ((struct cmddebugconf_sig *)send_buf)->result = CMDDEBUG_FAIL;
                                    }
                                 }
                                 else
                                 {
                                    #ifdef DEBUGPRINT
                                       sprintf(LogMsg,"Directory %s exists\r\n",DEBUG_FILE_PATH);
                                       DoMsg(LogMsg);
                                    #endif
                                 }
                                 // Open debug file, append if exists, create if not
 
                                 debug_file_handle = CreateFile(DEBUG_FILE_NAME,GENERIC_WRITE,0,0,OPEN_ALWAYS,0,0);
                                 if(debug_file_handle == INVALID_HANDLE_VALUE)
                                 {
                                    if((result = GetLastError()) == ERROR_FILE_EXISTS)
                                    {
                                       debug_file_handle = CreateFile(DEBUG_FILE_NAME,GENERIC_WRITE,0,0,OPEN_EXISTING,0,0);
                                       if(debug_file_handle != INVALID_HANDLE_VALUE)
                                          debug_file_active = true;
                                    }
                                    else
                                    {
                                       #ifdef DEBUGPRINT
                                          sprintf(LogMsg,"Open of file %s failed with error %d\r\n",DEBUG_FILE_NAME,GetLastError());
                                          DoMsg(LogMsg);
                                       #endif

                                       ((struct cmddebugconf_sig *)send_buf)->result = CMDDEBUG_FAIL;

                                    }
                                 }
                                 else
                                    debug_file_active = true;
                              }

                              // Send CMSSCONF back to command handler with result from Reply_Data[i]

                              ((struct cmddebugconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDDEBUGCONF);
                              ((struct cmddebugconf_sig *)send_buf)->sh.sig_num = CMDDEBUGCONF;
                              ((struct cmddebugconf_sig *)send_buf)->sh.sig_ver = 0;
                              ((struct cmddebugconf_sig *)send_buf)->cmd = CMDDEBUG_START;

                              if(!makeitsend(thd_sock,send_buf,SL_CMDDEBUGCONF+sizeof(struct sig_head),"CMDDEBUGCONF",ipnano))
                                 kill_thread(8,&thread_data);              // Clean up and die
                              break;
                           }

                           case CMDDEBUG_STOP:
                           { 
                              #ifdef DEBUGPRINT 
                                 sprintf(LogMsg,"IPNA %d received debug stop command %d\r\n",ipnano,(int)ch); 
                                 DoMsg(LogMsg); 
                              #endif 

                              CloseHandle(debug_file_handle);
                              debug_file_active = false;
                              ((struct cmddebugconf_sig *)send_buf)->result = CMDDEBUG_OK;

                              // Send CMSSCONF back to command handler with result from Reply_Data[i]

                              ((struct cmddebugconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDDEBUGCONF);
                              ((struct cmddebugconf_sig *)send_buf)->sh.sig_num = CMDDEBUGCONF;
                              ((struct cmddebugconf_sig *)send_buf)->sh.sig_ver = 0;
                              ((struct cmddebugconf_sig *)send_buf)->cmd = CMDDEBUG_STOP;

                              if(!makeitsend(thd_sock,send_buf,SL_CMDDEBUGCONF+sizeof(struct sig_head),"CMDDEBUGCONF",ipnano))
                                 kill_thread(8,&thread_data);              // Clean up and die

                              break;
                           }

                           case CMDDEBUG_GET:
                           {
                              #ifdef DEBUGPRINT 
                                 sprintf(LogMsg,"IPNA %d received debug get command %d\r\n",ipnano,(int)ch); 
                                 DoMsg(LogMsg); 
                              #endif 

                              // Set up header

                              ((struct cmddebugconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDDEBUGCONF);
                              ((struct cmddebugconf_sig *)send_buf)->sh.sig_num = CMDDEBUGCONF;
                              ((struct cmddebugconf_sig *)send_buf)->sh.sig_ver = 0;
                              ((struct cmddebugconf_sig *)send_buf)->cmd = CMDDEBUG_GET;

                              if(debug_file_active) // Fail if debug file open 
                              {
                                 ((struct cmddebugconf_sig *)send_buf)->result = CMDDEBUG_FAIL;

                                 // Send CMSSCONF back to command handler with result from Reply_Data[i]
 
                                 if(!makeitsend(thd_sock,send_buf,SL_CMDDEBUGCONF+sizeof(struct sig_head),"CMDDEBUGCONF",ipnano))
                                    kill_thread(8,&thread_data);              // Clean up and die
                              }
                              else
                              {
                                 // Open debug file, append if exists, create if not
 
                                 debug_file_handle = CreateFile(DEBUG_FILE_NAME,GENERIC_READ,0,0,OPEN_EXISTING,0,0);
                                 if(debug_file_handle == INVALID_HANDLE_VALUE)
                                 {
                                    #ifdef DEBUGPRINT
                                       sprintf(LogMsg,"Open of file %s failed with error %d\r\n",DEBUG_FILE_NAME,GetLastError());
                                       DoMsg(LogMsg);
                                    #endif

                                    ((struct cmddebugconf_sig *)send_buf)->result = CMDDEBUG_FAIL;

                                    // Send CMSSCONF back to command handler with result from Reply_Data[i]

                                    if(!makeitsend(thd_sock,send_buf,SL_CMDDEBUGCONF+sizeof(struct sig_head),"CMDDEBUGCONF",ipnano))
                                       kill_thread(8,&thread_data);              // Clean up and die
                                 }
                                 else
                                 {
                                    listp = &((struct cmddebugconf_sig *)send_buf)->data;    // Set a pointer to the start of data in signal

                                    // Read the file into the buffer upto the size of remaining buffer

                                    if(!ReadFile(debug_file_handle,listp,sizeof(send_buf)-sizeof(struct sig_head) - SL_CMDDEBUGCONF,&nread,0))
                                    {
                                       #ifdef DEBUGPRINT
                                          sprintf(LogMsg,"Read from file %s failed with error %d\r\n",DEBUG_FILE_NAME,GetLastError());
                                          DoMsg(LogMsg);
                                       #endif

                                       ((struct cmddebugconf_sig *)send_buf)->result = CMDDEBUG_FAIL;

                                       // Send CMSSCONF back to command handler with result from Reply_Data[i]

                                       if(!makeitsend(thd_sock,send_buf,SL_CMDDEBUGCONF+sizeof(struct sig_head),"CMDDEBUGCONF",ipnano))
                                          kill_thread(8,&thread_data);              // Clean up and die  
                                    }
                                    else
                                    {
                                       ((struct cmddebugconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDDEBUGCONF+(int)nread); // Add number of data read to length

                                       // Send CMSSCONF back to command handler with result from Reply_Data[i]

                                       if(!makeitsend(thd_sock,send_buf,SL_CMDDEBUGCONF+sizeof(struct sig_head)+(int)nread,"CMDDEBUGCONF",ipnano))
                                          kill_thread(8,&thread_data);              // Clean up and die
                                    }
                                    CloseHandle(debug_file_handle);
                                 }
                              }
                              break;
                           }

                           case CMDDEBUG_DELETE:
                           {
                              #ifdef DEBUGPRINT 
                                 sprintf(LogMsg,"IPNA %d received debug delete command %d\r\n",ipnano,(int)ch); 
                                 DoMsg(LogMsg); 
                              #endif 

                              // Set up header

                              ((struct cmddebugconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDDEBUGCONF);
                              ((struct cmddebugconf_sig *)send_buf)->sh.sig_num = CMDDEBUGCONF;
                              ((struct cmddebugconf_sig *)send_buf)->sh.sig_ver = 0;
                              ((struct cmddebugconf_sig *)send_buf)->cmd = CMDDEBUG_DELETE;

                              if(debug_file_active) // Fail if debug file open 
                              {
                                 ((struct cmddebugconf_sig *)send_buf)->result = CMDDEBUG_FAIL;

                                 // Send CMSSCONF back to command handler with result from Reply_Data[i]
 
                                 if(!makeitsend(thd_sock,send_buf,SL_CMDDEBUGCONF+sizeof(struct sig_head),"CMDDEBUGCONF",ipnano))
                                    kill_thread(8,&thread_data);              // Clean up and die 
                              }
                              else
                              {
                                 if(!DeleteFile(DEBUG_FILE_NAME))
                                    ((struct cmddebugconf_sig *)send_buf)->result = CMDDEBUG_FAIL;
                                 else 
                                    ((struct cmddebugconf_sig *)send_buf)->result = CMDDEBUG_OK;

                                 // Send CMSSCONF back to command handler with result from Reply_Data[i]
 
                                 if(!makeitsend(thd_sock,send_buf,SL_CMDDEBUGCONF+sizeof(struct sig_head),"CMDDEBUGCONF",ipnano))
                                    kill_thread(8,&thread_data);              // Clean up and die
                              }
                              break;
                           }

 
                           default:
                           {
                              #ifdef DEBUGPRINT
                                 sprintf(LogMsg,"IPNA %d received unhandled sub-command %d\r\n",ipnano,(int)ch);
                                 DoMsg(LogMsg);
                              #endif

                              ((struct cmddebugconf_sig *)send_buf)->result = CMDDEBUG_FAIL;

                              // Send CMSSCONF back to command handler with result from Reply_Data[i]

                              ((struct cmddebugconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDDEBUGCONF);
                              ((struct cmddebugconf_sig *)send_buf)->sh.sig_num = CMDDEBUGCONF;
                              ((struct cmddebugconf_sig *)send_buf)->sh.sig_ver = 0;
                              ((struct cmddebugconf_sig *)send_buf)->cmd = CMDDEBUG_UNKNOWN;

                              if(!makeitsend(thd_sock,send_buf,SL_CMDDEBUGCONF+sizeof(struct sig_head),"CMDDEBUGCONF",ipnano))
                                 kill_thread(8,&thread_data);              // Clean up and die
                              break;
                           }
                        }
                     }
                     else
                     {
                        #ifdef DEBUGPRINT
                           sprintf(LogMsg,"\r\n   Debug command received by IPNA handler \r\n");
                           DoMsg(LogMsg);
                        #endif
                     }

                     kill_thread(255,&thread_data);              // Clean up and die
                     break;
                  }
                  default:
                  {
                     #ifdef DEBUGPRINT
                        sprintf(LogMsg,"IPNA %d didn't handle signal %d \r\n",ipnano,(int)ch);
                        DoMsg(LogMsg);
                     #endif
                     break;
                  }
               }
            }

            else
            {
               #ifdef DEBUGPRINT
                  sprintf(LogMsg,"IPNA %d didn't handle socket event %d\r\n",ipnano,sock_events.lNetworkEvents);
                  DoMsg(LogMsg);
               #endif
            }

            break;
         }

         // This thread has received a command from the command handler thread

         case (WAIT_OBJECT_0 + COMMANDEVENT):
         {
            #ifdef DEBUGPRINT
               sprintf(LogMsg,"IPNA %d received COMMANDEVENT\r\n",ipnano);
               DoMsg(LogMsg);
            #endif

            // The only command sent to IPNAs so far is set state with a state and sub-state value

            if((ipnano >= 0) && (ipnano < MAX_IPNAS)) // If valid IPNA id range
            {

               // Send SETIPNASTATEREQ with state, sub-state from command

               ((struct ss_sig *)send_buf)->sh.sig_len = htons(SL_SSREQ);
               ((struct ss_sig *)send_buf)->sh.sig_num = SETIPNASTATEREQ;
               ((struct ss_sig *)send_buf)->sh.sig_ver = 0;

               if(Command_Data[ipnano] & STATE_SEP)
                  ((struct ss_sig *)send_buf)->state = IPNA_STATE_SEP;
               else
                  ((struct ss_sig *)send_buf)->state = IPNA_STATE_NORM;

               // The sub-state is locked if state=sep, open if norm 

               if(Command_Data[ipnano] & STATE_SEP)
                  ((struct ss_sig *)send_buf)->substate = IPNA_SUBSTATE_LOCK;
               else
                  ((struct ss_sig *)send_buf)->substate = IPNA_SUBSTATE_OPEN;

               if(!makeitsend(thd_sock,send_buf,SL_SSREQ+sizeof(struct sig_head),"SETIPNASTATEREQ",ipnano))
                  kill_thread(8,&thread_data);              // Clean up and die
               Command_Data[ipnano] = 0;
            }
            else 
            {
               #ifdef DEBUGPRINT
                  sprintf(LogMsg,"Command event received by the command handler!\r\n");
                  DoMsg(LogMsg);
               #endif

               kill_thread(129,&thread_data);              // Clean up and die
            }
            break;
         }


         // Command thread handles IPNA replies 

         case (WAIT_OBJECT_0 + REPLYEVENT):
         {
            #ifdef DEBUGPRINT
               sprintf(LogMsg,"IPNA %d received reply event\r\n",ipnano);
               DoMsg(LogMsg);
            #endif

             // At the moment, there is only one command generating a reply event

            if(ipnano == ID_COMMAND)               // Test for command handler
            {
               for(i=0;i<MAX_IPNAS;i++)
               {
                  if(Replied[i])  // Test if IPNA has a reply
                  {
                     // Send CMSSCONF back to command handler with result from Reply_Data[i]

                     ((struct cmdssconf_sig *)send_buf)->sh.sig_len = htons(SL_CMDSSCONF);
                     ((struct cmdssconf_sig *)send_buf)->sh.sig_num = CMDSSCONF;
                     ((struct cmdssconf_sig *)send_buf)->sh.sig_ver = 0;
                     ((struct cmdssconf_sig *)send_buf)->result = Reply_Data[i];

                     if(!makeitsend(thd_sock,send_buf,SL_CMDSSCONF+sizeof(struct sig_head),"CMDSSCONF",ipnano))
                        kill_thread(8,&thread_data);              // Clean up and die 
                     Replied[i] = FALSE;          // Handled a reply
                  }
               }
               ResetEvent(ipna_replied_event); // Probably safe to reset this now
            }
            else 
            {
               #ifdef DEBUGPRINT
                  sprintf(LogMsg,"Reply event received by ipna handler!\r\n");
                  DoMsg(LogMsg);
               #endif

               kill_thread(130,&thread_data);              // Clean up and die
            }
            break;
         }

         case (WAIT_OBJECT_0 + MULTICONTERMEVENT):
         {
            #ifdef DEBUGPRINT
               sprintf(LogMsg,"ipna %d : Received multicontermevent\r\n",ipnano);
               DoMsg(LogMsg);
            #endif


            if((ipnano >=0) && (ipnano < MAX_IPNAS))    // If valid IPNA id range
            {
//             ResetEvent(MultiConTermEvent[ipnano]);   // multicon term is done
               kill_thread(888,&thread_data);           // just kill thread 
            }
            else if(ipnano == ID_COMMAND)               // Test for command handler
            {
//             ResetEvent(MultiCmdTermEvent);           // multicmd term is done
               kill_thread(889,&thread_data);           // just kill thread 
            }


            break;
         }

         case WAIT_FAILED:
         {
            #ifdef DEBUGPRINT
               sprintf(LogMsg,"IPNA %d : WMO returned WAIT_FAILED\r\n",ipnano);
               DoMsg(LogMsg);
 
               sprintf(LogMsg,"Thread waitmulti failed, waiting for %d objects,with error %d\r\n",nothreadobj,GetLastError());
               DoMsg(LogMsg); 
            #endif

            kill_thread(2,&thread_data);              // Clean up and die
            break;     
         }

         case WAIT_ABANDONED:
         {
            #ifdef DEBUGPRINT
               sprintf(LogMsg,"IPNA %d : WMO returned WAIT_ABANDONED\r\n",ipnano);
               DoMsg(LogMsg);
            #endif
            kill_thread(3,&thread_data);              // Clean up and die
            break;
         }

         case WAIT_TIMEOUT:
         {
            if(hbts)          // An IPNA capable of using heartbeats has timeout - kill it
            {
               #ifdef DEBUGPRINT
                  sprintf(LogMsg,"IPNA %d : Heartbeat timeout\r\n",ipnano);
                  DoMsg(LogMsg);
               #endif
               kill_thread(3,&thread_data);              // Clean up and die
            }
            else if(ipna_version == CAN_HEARTBEAT)
            {
               // IPNA can use heartbeat mechanism

			   timeout = HEARTBEAT_TIMEOUT;  // Set WaitForMultipleObjects timeout
			   hbts = true;                  // Enable heartbeat supervision

			   // Send heartbeat signal to ipna

               ((struct sig_head *)send_buf)->sig_len = htons(SL_HBREQ);
               ((struct sig_head *)send_buf)->sig_num = HEARTBEATREQ;
               ((struct sig_head *)send_buf)->sig_ver = 0;

               if(!makeitsend(thd_sock,send_buf,SL_HBREQ+sizeof(struct sig_head),"HEARTBEATREQ",ipnano))
                  kill_thread(8,&thread_data);              // Clean up and die
            }
            else
            {
               #ifdef DEBUGPRINT
                  sprintf(LogMsg,"IPNA %d : WMO returned WAIT_TIMEOUT\r\n",ipnano);
                  DoMsg(LogMsg);
               #endif

               kill_thread(4,&thread_data);              // Clean up and die
            }

            break;
         }

         default:
         {
            #ifdef DEBUGPRINT
               sprintf(LogMsg,"IPNA %d didn't handle wait object %d\r\n",ipnano,result);
               DoMsg(LogMsg);
            #endif
            kill_thread(5,&thread_data);              // Clean up and die
            break;

         }

      }
   }
   return 0;   // Needed due to non-void declaration
}

//******************************************************************************
// kill_thread
//
// Clean up globals, events and die
//
// INPUT: -
// OUTPUT:- 
// OTHER:- 
//******************************************************************************

void kill_thread(int err_code,struct thread_data *thread_data)
{
   char ch;


   EnterCriticalSection(&connsec);
   if(curr_cons>0)
	   curr_cons--;
   LeaveCriticalSection(&connsec);

   DeleteCriticalSection(&critsec);

   #ifdef DEBUGPRINT
      sprintf(LogMsg,"Thread for ipna %d terminated with code %d\r\n",thread_data->ipnano,err_code);
      DoMsg(LogMsg);
   #endif
 
   if((thread_data->ipnano >=0) && (thread_data->ipnano  < MAX_IPNAS))   // If valid IPNA id range
   { 
      EnterCriticalSection(&ipnasec); 

      connected[thread_data->ipnano] = 0;

      Command_Events[thread_data->ipnano ] = 0;  
      CloseHandle(thread_data->Command_Event);         // Remove event

      LeaveCriticalSection(&ipnasec);
   }  
   else if(thread_data->ipnano == ID_COMMAND)          // ID for command handler
   {  
      command_connected = 0;                           // Indicate command handler not connected
   } 
 
   CloseHandle(thread_data->dummy_event);

   shutdown(thread_data->thd_sock,1);                  // Half close socket
   while(recv(thread_data->thd_sock,&ch,1,0) == 1)     // Call recv until it returns 0 or error
     ;                                    // to clear unreceived data on socket 
 
   if(closesocket(thread_data->thd_sock) == SOCKET_ERROR)  // Close control socket
   {
      #ifdef DEBUGPRINT
         sprintf(LogMsg,"Closesocket failed during threadkill with WSA error %d\r\n",WSAGetLastError());
         DoMsg(LogMsg);
      #endif
   }

   WSACloseEvent(thread_data->ThreadSockEvents);       // Close event handle

#ifdef _DEBUG
   sprintf(LogMsg,"Thread for IPNA %d has gone to a better place\r\n",thread_data->ipnano);
   DoMsg(LogMsg);
#endif

   EXIT_THREAD(err_code);                 // Exit thread.
}

//******************************************************************************
// makeitsend
//
// Send a signal and make sure it's all sent
//
//******************************************************************************

bool makeitsend(SOCKET its_sock,char *its_buf,int its_len, char *its_name,int ipna)
{
    int result;
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
		if(result == SOCKET_ERROR)                    // Handle inet errors
		{
			#ifdef DEBUGPRINT
				sprintf(LogMsg,"Send of %s signal failed with WSA error %d\r\n",its_name,WSAGetLastError());
				DoMsg(LogMsg);
			#endif
			return(false);
		}
		its_len -= result;
		aktuell = its_len;
		datap += result;
	}


#ifdef DEBUGPRINT
    sprintf(LogMsg,"IPNA %d sent %d bytes of the %d byte %s signal\r\n",ipna,result,its_len,its_name);
    DoMsg(LogMsg);
#endif

    memset(its_buf,0,4); // clear the signal header part of the send buffer
    return(true);
}

//******************************************************************************
// recvitall
//
// Receive multiple bytes from a socket and ensure it is all received
//
//******************************************************************************

bool recvitall(SOCKET its_sock,char *its_buf,int its_len,char *its_name,int ipna)
{
   int result,remain;
   int retrycnt = 0;
   char *datap;
#ifdef DEBUGPRINT
   int i;
#endif

   result = recv(its_sock,its_buf,its_len,0); // Try to read all bytes asked for
   if(result == SOCKET_ERROR)
   {
      // Keep trying if socket blocks, exit otherwise

      if((result=WSAGetLastError()) == WSAEWOULDBLOCK)
      {
         if(retrycnt++ > 400) return(false);     // Stop it locking up if something goes wrong
		 Sleep(10);
      }
      else
      {
         #ifdef DEBUGPRINT
            sprintf(LogMsg,"Receive of %s failed with WSA error %d\r\n",its_name,WSAGetLastError());
            DoMsg(LogMsg);
         #endif 
         return(false);
      }
   }
   else if(result < its_len)
   {
      datap = its_buf + result;                // Make a pointer to place in  buf not yet received 
      remain = its_len - result;               // remainder
      while(remain>0)                          // loop until nothing remains
      {
         result = recv(its_sock,datap,remain,0); // Try to receive remainder of signal
         if(result == SOCKET_ERROR)
         {
            // Keep trying if socket blocks
            if((result=WSAGetLastError()) == WSAEWOULDBLOCK)
            {
               if(retrycnt++ > 400)            // Stop it locking up if something goes wrong
               {
				   #ifdef DEBUGPRINT
                     sprintf(LogMsg,"receiveitall exceeded maximum retry count with %d bytes remaining\r\n",remain);
                     DoMsg(LogMsg);
                   #endif 
				   return(false);
			   }
			   else
			       Sleep(10);
            }
            else
            {
               #ifdef DEBUGPRINT
                  sprintf(LogMsg,"Receive of %s failed with WSA error %d\r\n",its_name,WSAGetLastError());
                  DoMsg(LogMsg);
 
                  sprintf(LogMsg,"Asked for %d, got %d\r\ndata: ",its_len,its_len-remain);
                  DoMsg(LogMsg);
                  for(i=0;i<its_len-remain;i++)
                  {
                     sprintf(LogMsg,"%0X ",its_buf[i]);
                     DoMsg(LogMsg);
                  }

                  sprintf(LogMsg,"\r\n");
                  DoMsg(LogMsg);
               #endif
               return(false);
            }
         }
         else
            remain-=result;
      }
   }
   return(true);
}


//******************************************************************************
//
// find_older
//
// Compare two osedump file names and decide if the second is older 
// than the first. These files are created with names based on the date and
// time.
//
//******************************************************************************

bool file_older(char *file1, char *file2)
{
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
      #ifdef DEBUGPRINT
         sprintf(LogMsg,"file_older : ipna not found in %s\r\n",file1);
         DoMsg(LogMsg);
      #endif
      return false;
   }
   else
   {
      if((result = sscanf(thing,"ipna%2d-%4d%2d%2d-%2d%2d%2d.oselog",&f[0],&f[1],&f[2],&f[3],&f[4],
           &f[5],&f[6])) != 7)  // Will return different number if name doesn't match format
      {
         if((result = sscanf(thing,"Ipna%2d-%4d%2d%2d-%2d%2d%2d.oselog",&f[0],&f[1],&f[2],&f[3],&f[4],
              &f[5],&f[6])) != 7)  // Will return different number if name doesn't match format
         {
            #ifdef DEBUGPRINT
               sprintf(LogMsg,"file_older : sscanf on file %s returned %d match(es)\r\n",file1,result);
               DoMsg(LogMsg);
            #endif
          
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
      #ifdef DEBUGPRINT
         sprintf(LogMsg,"file_older : ipna not found in %s\r\n",file2);
         DoMsg(LogMsg);
      #endif
      return false;
   }
   else
   {
      if((result = sscanf(thing,"ipna%2d-%4d%2d%2d-%2d%2d%2d.oselog",&h[0],&h[1],&h[2],&h[3],&h[4],
           &h[5],&h[6])) != 7)  // Will return different number if name doesn't match format
      {
         #ifdef DEBUGPRINT
            sprintf(LogMsg,"file_older : sscanf on file %s returned %d match(es)\r\n",file2,result);
            DoMsg(LogMsg);
         #endif
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


//******************************************************************************
//  connectToCPon14000()

// return values:
// true: connection successful to port 14000
// false: connection failed.
//******************************************************************************
bool connectToCPon14000(short net)
{
	extern u_long		link_IP[2];
	bool				res = false;
	int					result;
	WSAEVENT			hand;
	SOCKET				CPconnectSock = INVALID_SOCKET;
	HANDLE				CPconnectHand = INVALID_HANDLE_VALUE;
	struct sockaddr_in	CPaddr;
    CPaddr.sin_family = AF_INET;
    CPaddr.sin_port = htons(14000);
	int waitTime = 2000 + (1000 * (char)connected[net]);
	if (waitTime > 3000) waitTime = 3000;

	if (link_IP[net&1] != 0)
	{
	  CPaddr.sin_addr.s_addr = link_IP[net&1];

	// Create connection sockets to the CP.

	  if ((CPconnectSock = socket(AF_INET, SOCK_STREAM, 0)) != INVALID_SOCKET)
	  {

	    hand = WSACreateEvent();
	    WSAEventSelect(CPconnectSock, (HANDLE)hand, FD_CONNECT);
	    CPconnectHand = (HANDLE)hand;

	    if (CPconnectHand != INVALID_HANDLE_VALUE)
		{
		  result = connect(CPconnectSock,(struct sockaddr *) &CPaddr,sizeof(CPaddr));
		  switch (result)
		  {
		  case 0:
			res = true;
			break;
		  case SOCKET_ERROR:
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
		      DWORD rc = WaitForSingleObject(CPconnectHand, waitTime);
		      if (rc == WAIT_OBJECT_0)
			  {
		        res = true;
			  }
			}
			break;
		  default:;
		  }
		}
	  }
	}

	if (CPconnectHand != INVALID_HANDLE_VALUE) CloseHandle(CPconnectHand);
	if (CPconnectSock != INVALID_SOCKET) closesocket(CPconnectSock);

	return res;
}