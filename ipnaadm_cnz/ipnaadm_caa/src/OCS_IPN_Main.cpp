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
// OCS_IPN_Main.cpp
//
// DESCRIPTION 
//
// DOCUMENT NO
// 2/190 55-CAA 109 0392
//
// AUTHOR 
// 0000803 EPA/D/U Michael Bentley
//
//******************************************************************************
// *** Revision history ***
// 0000922 MJB Created 
//******************************************************************************

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "OCS_IPN_Main.h"

#ifdef _DEBUG
   #define DEBUGPRINT
#endif

#ifdef DEBUGPRINT
    #define DEBUGONSTART       // Uncomment this to start server with debugging enabled
#endif

#ifdef INC_SYSLOG
   #include "CPS_syslog.h"
#endif

//******************************************************************************
// OCS_IPN_Main
//
//
// INPUT:  -
// OUTPUT:  
// OTHER:  -
//******************************************************************************

DWORD OCS_IPN_Main(LPDWORD param)
{

   //***************************************************************************
   // Local variables   
   //***************************************************************************

   WORD             wVersionRequested;       // Winsocket version available
   DWORD            result;                  // Result from function
   WSADATA          wsaData;
   DWORD            tid;                     // Place for thread id 

   SOCKADDR_IN      s_listen;                // Listening socket address 
   int              s_ctrllen;               // Length of Ctl sock addr returned by accept
 
   struct servent   *servp;                  // Pointer to service entry
   short            list_port;               // Port for listen socket 

   WSANETWORKEVENTS list_events;             // Listen socket events
 
   struct thread_param thread_params;		 // Parameters for thread

   HANDLE           hThread;
   struct hostent*	p;

   struct hostent	*hostp;					 // Variables for getting local host IP
   struct in_addr	*ptr; 
   char				local_host[256];
   u_long			local_host_IP = 0;

   struct sockaddr_in socketAddr;			 // Variables for checking IPNA calls 
   int				i;
   char*			siteName;
   char*			clientIP_Address;
   struct hostent*	clientInfo;
   int				Ipna_num;
   u_long			IPNA_0, IPNA_1, current_IP;
   int				handle_ref[4];
   bool             classicAPZ; 
   bool             multiCP;  
   bool             bind_failed = false;  


#ifdef DEBUGONSTART
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
         }
      }
      else
         debug_file_active = true;
   }
#endif


   list_sock = INVALID_SOCKET;


   // Start winsock2

   wVersionRequested = MAKEWORD(2, 2);
   if((result = WSAStartup(wVersionRequested, &wsaData)) != 0 )
   {
      terminateMain("WSAStartup", result);
	  return 1;
   }


   // Check up the IP addresses for CP devices

   link_IP[0] = 0;
   link_IP[1] = 0;
   handle_ref[0] = -1;
   handle_ref[1] = -1;
   handle_ref[2] = -1;
   handle_ref[3] = -1;
   classicAPZ = false;
   multiCP = false;

   ACS_CS_API_NS::CS_API_Result returnValue;
 
   returnValue = ACS_CS_API_NetworkElement::isMultipleCPSystem(multiCP);

   if (returnValue == ACS_CS_API_NS::Result_Success)
   {
      if (multiCP == false)
      {
         ACS_PHA_Tables apconfTable("ACS/CXC1371147");
         ACS_PHA_Parameter<int> cpType("ACS_APCONFBIN_CpAndProtocolType");

         // Get CP-type from PHA

         if (cpType.get(apconfTable) == ACS_PHA_PARAM_RC_OK)
         {
	        if (cpType.data()<2)
            { 
	  	       classicAPZ = true;
			} 
         }
	  }
   }

   if (classicAPZ == true)
   {
	  p = gethostbyname(CP_EX_low);
	  if (p != (struct hostent*) NULL)
	  {
		 memcpy((char *) &socketAddr.sin_addr.s_addr, p->h_addr_list[0], p->h_length);
		 link_IP[0] = socketAddr.sin_addr.s_addr;
	  }
	  p = gethostbyname(CP_EX_high);
	  if (p != (struct hostent*) NULL)
	  {
		 memcpy((char *) &socketAddr.sin_addr.s_addr, p->h_addr_list[0], p->h_length);
		 link_IP[1] = socketAddr.sin_addr.s_addr;
	  }
   }


 // Check the IP address if local host

   gethostname(local_host,sizeof(local_host));
   hostp = gethostbyname(local_host);
   if((ptr = (struct in_addr *)hostp->h_addr_list[0]) != NULL)
   {
		local_host_IP = ptr->s_addr;
   }

   // Test for the existence of directory for osedumps. Create if not there

   if (!CreateDirectory(LOGPATH_1ST,0))
   {
      if((result=GetLastError()) != ERROR_ALREADY_EXISTS)
      {
          sprintf(LogMsg,"Could not create directory %s\r\n",LOGPATH_1ST);
          terminateMain(LogMsg,result);
		  return 2;
      }
   }

   if (!CreateDirectory(LOGPATH,0))
   {
      if((result=GetLastError()) != ERROR_ALREADY_EXISTS)
      {
          sprintf(LogMsg,"Could not create directory %s\r\n",LOGPATH);
          terminateMain(LogMsg,result);
		  return 3;
      }
   }


// Create listening socket

   if ((list_sock = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,NULL,0,NULL)
      ) == INVALID_SOCKET)
   {
      terminateMain("WSASocket",WSAGetLastError());
	  return 4;
   }
   

// Get port number for this service from operating system

   if ((servp=getservbyname("ipnaadm","tcp")) == NULL)// Entry for ipnaadm isn't in 
   {                                                  // the service file
      list_port = htons(ADM_PORT_NUMBER);             // Use this as default
   }
   else                                               // Entry for ipnaadm service exits 
   {
      list_port = servp->s_port;                      // Use OS port number
   }
 

// Specify a local endpoint address for listening socket

   s_listen.sin_family      = PF_INET;
   s_listen.sin_addr.s_addr = INADDR_ANY;
   s_listen.sin_port        = list_port;


// Bind the control socket to local endpoint address

   if(bind(list_sock,(struct sockaddr *)&s_listen,sizeof(struct sockaddr)) == SOCKET_ERROR)
   {
      if (classicAPZ == true) 
	  {
         terminateMain("bind",WSAGetLastError());
	     return 5;
	  }
	  bind_failed = true;
   }


// Create events in main object array. Index 0 in array is dedicated to
// abort event created in service.

   if((hMainObjectArray[SERVICE_ABORT_EVENT] = CreateEvent(NULL,TRUE,FALSE,NULL)) == NULL)
   {
      terminateMain("CreateEvent,MainObjectArray",GetLastError());
   }

   hListEvent = WSACreateEvent();                    // Create an event object for listen socket events
   hMainObjectArray[LISTSOCK_EVENTS] = hListEvent;   // Place event in main object array 


// Set an event on new connection, close of socket or read for listen socket

   if(WSAEventSelect(list_sock,hListEvent,FD_ACCEPT|FD_CLOSE) == SOCKET_ERROR)
   {
      terminateMain("WSAEventSelect",GetLastError());  
	  return 6;
   }


// Create an event to terminate all threads

   if((ThreadTermEvent=CreateEvent(NULL,TRUE,FALSE,NULL)) == NULL) // Manual reset, not signalled
   {
      terminateMain("CreateEvent,ThreadTermEvent",GetLastError()); // Terminate if event create fails
	  return 7;
   }


// Listen for new connections
 
   if(listen(list_sock,LIST_BACKLOG) == SOCKET_ERROR) // Allow backlog of connections
   {
      terminateMain("listen",WSAGetLastError());
	  return 8;
   }


// Create an event for IPNAs to reply to the command handler

   ipna_replied_event = CreateEvent(NULL,TRUE,FALSE,NULL);  // Manual reset


// Listen for new connections
 
   if(listen(list_sock,LIST_BACKLOG) == SOCKET_ERROR) // Allow backlog of connections
   {
      if (classicAPZ == true)
	  {
         terminateMain("listen",WSAGetLastError());
	     return 9;
	  }
	  bind_failed = true;
   }
  

// Create an event to terminate multiple command connection threads

   if((MultiCmdTermEvent=CreateEvent(NULL,TRUE,FALSE,NULL)) == NULL) // Manual reset, not signalled
   {
      terminateMain("CreateEvent,ThreadTermEvent",GetLastError()); // Terminate if event create fails
	  return 10;
   }

   command_connected = 0;

   for (i=0; i<MAX_IPNAS; i++)                        // Initialize connection array
   {
	  connected[i] = 0;
      MultiConTermEvent[i] = CreateEvent(NULL,TRUE,FALSE,NULL);  // Manual reset
	  // Create an event for IPNAs to terminate when one ipna has multiple connections
   }


// Here the JTP interface is initiated

   HANDLE temphandl = hMainObjectArray[LISTSOCK_EVENTS];		// Temporarily shut out
   hMainObjectArray[LISTSOCK_EVENTS] = INVALID_HANDLE_VALUE;	// link calls

   int				NoOfFds;
   HANDLE*			Fd;
   ACS_JTP_Service	S(IPNAADMJTPNAME);
   ACS_JTP_Job		J;

   while (S.jidrepreq() == false)
   {
	  if (WaitForMultipleObjects((NO_OF_EVENTS_MAIN-4),
								 hMainObjectArray,
								 FALSE,
								 5000) == (WAIT_OBJECT_0 + SERVICE_ABORT_EVENT))
	  {
		  terminateMain("Service Aborted",0);
          return 11;
	  }
   }

   S.getHandles(NoOfFds, Fd);
   hMainObjectArray[JTP_EVENTS]   = Fd[0];
   hMainObjectArray[JTP_EVENTS+1] = Fd[1];
   hMainObjectArray[JTP_EVENTS+2] = Fd[2];
   hMainObjectArray[JTP_EVENTS+3] = Fd[3];

   hMainObjectArray[LISTSOCK_EVENTS] = temphandl;


// Initialize critical sessions

   no_conns = curr_cons = 0;            // Reset connection count
   InitializeCriticalSection(&connsec); // A critical section is required as curr_cons
										// changed in threads and main
   InitializeCriticalSection(&ipnasec); // Critical section for managing ipna multiple
										// connections

// Initialize my thread handles

   for (foo=0; foo<50; foo++) hThreads[foo] = NULL;
   foo = 0;


// Loop until main is terminated. Controlled termination occurs following  
// reception of abort event from Service Control Manager.

   while (true)
   {
      // This function is central in main thread. It holds the array of object 
      // handles. The objects are events,timers and threads.

      // Check for any of the event objects in the array.

      result = WaitForMultipleObjects(((NO_OF_EVENTS_MAIN-4) + NoOfFds),
									  hMainObjectArray,
									  FALSE,
									  INFINITE);
      switch(result)
      {
         // Abort event from service

        case (WAIT_OBJECT_0 + SERVICE_ABORT_EVENT):
        {
            terminateMain("Service Aborted",0);
            return 0;
        }

        case (WAIT_OBJECT_0 + LISTSOCK_EVENTS):
        {
            WSAEnumNetworkEvents(list_sock,hListEvent,&list_events);
            WSAResetEvent(hListEvent);                    // WSA events are manually reset, so doit

            if(list_events.lNetworkEvents & FD_ACCEPT)    // Connection event
            {
				// Accept connection.

				s_ctrllen = sizeof(struct sockaddr);
				for (i=0; i<10; i++)					  // Blocking accept could be delayed
				{
					Sleep(25);
					if((ctrl_sock = accept(list_sock, (SOCKADDR *)&s_control, &s_ctrllen)) 
										  != INVALID_SOCKET) break;
				}

				if (ctrl_sock == INVALID_SOCKET)
				{
				#ifdef DEBUGPRINT
					sprintf(LogMsg,"accept failed with wsa error %d\r\n",WSAGetLastError());
					DoMsg(LogMsg);
				#endif
				}
				else
				{
					clientIP_Address = (char*) &s_control.sin_addr.s_addr;
					current_IP = s_control.sin_addr.s_addr;
					clientInfo = gethostbyaddr(clientIP_Address,
											   sizeof(u_long),	
											   AF_INET);
					if (clientInfo != NULL)
					{
						siteName = clientInfo->h_name;
						if (strcmp(siteName, CP_EX_low) == 0)
							link_IP[0] = current_IP;
						else
							if (strcmp(siteName, CP_EX_high) == 0)
								link_IP[1] = current_IP;
							else
								if (strcmp(siteName, CP_SB_low) == 0)
									link_IP[0] = current_IP;
								else
									if (strcmp(siteName, CP_SB_high) == 0)
										link_IP[1] = current_IP;
					}


				// Find out from which IPNA the call might come

					IPNA_0 = link_IP[0];					// Get IP addr. for IPNA-0
					IPNA_1 = link_IP[1];					// Get IP addr. for IPNA-1
					Ipna_num = 4;							// Non existing IPNA
					if (current_IP == IPNA_0)
						Ipna_num = 0;
					else									// IPNA-0
						if (current_IP == IPNA_1)
							Ipna_num = 1;
						else								// IPNA-1
							if (current_IP == (IPNA_0 + 0x02000000))
								Ipna_num = 2;
							else							// IPNA-2
								if (current_IP == (IPNA_1 + 0x02000000))
									Ipna_num = 3;			// IPNA-3
								else
								{
									if (current_IP != local_host_IP) break;
								}


			// Start the thread for handling connections

					thread_params.thread_sock = ctrl_sock; // Pass the new socket to thread

					hThread = CREATE_THREAD(SECURITY_ATTRIBUTES, 
											THREAD_STACK_SIZE, 
											(LPTHREAD_START_ROUTINE)OCS_IPN_Thread,
											&thread_params, 
											0,   
											&tid);

					no_conns++;                         // One more connection
					EnterCriticalSection(&connsec);     // Avoid problems with directory lists
					curr_cons++;                        // One more current connection
					LeaveCriticalSection(&connsec);

					if (hThread)
					{
						while ((hThreads[foo] != NULL) &&
							   (hThreads[foo] != INVALID_HANDLE_VALUE))
						{
							bool found = false;
							for (i=0; i<4; i++) if (handle_ref[i]==foo) found = true;
							if (found == false)
							{
								TerminateThread(hThreads[foo], 0);
								CloseHandle(hThreads[foo]);
								break;
							}
							if (++foo>49) foo = 0;
						}

						hThreads[foo] = hThread;
						if (Ipna_num < 4) handle_ref[Ipna_num] = foo;
						if (++foo>49) foo = 0;
					}
				}
            }
            else if(list_events.lNetworkEvents & FD_CLOSE)
            {
               #ifdef DEBUGPRINT
                  sprintf(LogMsg,"Listening socket has closed !\r\n");
                  DoMsg(LogMsg);
               #endif
            }
            else
            {
               #ifdef DEBUGPRINT
                  sprintf(LogMsg,"Received event %d\n",list_events.lNetworkEvents);
                  DoMsg(LogMsg);
               #endif
            }
            break;
        }


         // JTP event, or Unexpected event - terminate main

        case (WAIT_OBJECT_0 + JTP_EVENTS):
        case (WAIT_OBJECT_0 + JTP_EVENTS + 1):
        case (WAIT_OBJECT_0 + JTP_EVENTS + 2):
        case (WAIT_OBJECT_0 + JTP_EVENTS + 3):
		{
            WSAEnumNetworkEvents(list_sock,hListEvent,&list_events);
            WSAResetEvent(hListEvent);     // WSA events are manually reset, so doit

			bool signaled = false;
			for (i=0; i<NoOfFds; i++)
				if (Fd[i] != INVALID_HANDLE_VALUE)
					if (WaitForSingleObject(Fd[i],0)==WAIT_OBJECT_0)
					{
						signaled = true;
						break;
					}
			if (signaled)
			{
				S.accept(&J, 0);
				if (J.State() == ACS_JTP_Job::StateConnected)
				{
					ushort action, ipna, Len, res;
					char * Msg;
					if (J.jinitind(action, ipna, Len, Msg) == true)
					{
						if (J.jinitrsp(action, ipna, 0) == true)
						{
							if (action == PREPARE_FC)
								res = (ushort)prepareFunctionChange(ipna&63);
							J.jresultreq(action, res, 0, 0, Msg);
						}
					}
				}
			}
			break;
		}


// Otherwise branch
	default:
		{
			terminateMain("Unexpected event",result);
			return 12;
		}	// default:
      }		// switch (result)
   }		// while (true)

   return 0;// Threads are declared with types - must return at the end
}


//******************************************************************************
//  put_buffer()
//******************************************************************************

void put_buffer(char* buf, DWORD len, char* bootcontents_out)
{
	DWORD totlen;

	totlen = strlen(bootcontents_out);
	strncpy(bootcontents_out+totlen, buf, len);
	bootcontents_out[totlen+len] = 0;
}


//******************************************************************************
//  prepareFunctionChange()

// return values:
// CMDFCPREP_SUCCESS(0) : Operation succeeded
// CMDFCPREP_NOCHANGE(1): No Function Change files loaded
// CMDFCPREP_FAULT(2)   : Other fault
//******************************************************************************
DWORD prepareFunctionChange(char ipnx)
{
	WIN32_FIND_DATA	lpFindFileData;
	bool			equality;
	DWORD			pos;
	DWORD			highest;
	HANDLE			search_handle;
	HANDLE			open_handle;

	char*			p1;
	char*			p2;
	char*			p3;
	char*			p4;
	char*			ptr;

	DWORD			index = 0;
	DWORD			nbRead;
	DWORD			nbWritten;
	DWORD			len;
	DWORD			ind;
	bool			changed1 = false;
	bool			changed2 = false;
	char			filecontents[16][120];
	char			module[16][32];
	char			module1[32];
	char			filename[80];
	char			prefix[80];
	char			oneline[120];
	char			ffile[80] = TFTPBOOTDIR;
	char			ffile_backup[80];
	char			ffile_oldbackup[80];
	sprintf(ffile_backup,    "%s%d\0", BACKUPFILE,    ipnx);
	sprintf(ffile_oldbackup, "%s%d\0", OLDBACKUPFILE, ipnx);
	char			bootcontents[2048];
	char			bootcontents_out[2048];
	bootcontents_out[0] = '\0';


	// First scan through all newrev_*.ver files and store their contents in PM

	search_handle = FindFirstFile(ffile, &lpFindFileData);

	while (search_handle != INVALID_HANDLE_VALUE)
	{
		strcpy(filename, (char*) &lpFindFileData.cFileName);
		ptr = strchr(filename, 95);				// Underscore
		if (ptr != NULL)
		{
			strncpy(prefix, filename, (ptr-filename));
			prefix[ptr-filename] = '\0';
			if (strcmp(prefix, "newrev") == 0)
			{
				strcpy(filename, ++ptr);
				ptr = strchr(filename, 46);		// Dot
				if (ptr != NULL)
				{
					strncpy(prefix, filename, (ptr-filename));
					prefix[ptr-filename] = '\0';
					strcpy(&module[index][0], prefix);
					strcpy(filename, ++ptr);
					if (strcmp(filename, "ver") == 0)
					{
						strcpy(ffile, TFTPBOOTPATH);
						strcat(ffile, (char *) &lpFindFileData.cFileName);
						open_handle = CreateFile((LPCTSTR) ffile,
												 GENERIC_READ,
												 FILE_SHARE_READ,
												 NULL,
												 OPEN_EXISTING,
												 FILE_ATTRIBUTE_READONLY,
												 NULL);
						if (ReadFile(open_handle,
									 (LPVOID) filecontents[index],
									 119,
									 &nbRead,
									 NULL))
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
		if (FindNextFile(search_handle, &lpFindFileData) == false)
			search_handle = INVALID_HANDLE_VALUE;
	}


// Now go into the boot file and edit the lines with new module versions

	sprintf(ffile, "%s%d\0", BOOTFILE, ipnx);

	open_handle = CreateFile((LPCTSTR) ffile,
							 GENERIC_READ,
							 FILE_SHARE_READ,
							 NULL,
							 OPEN_EXISTING,
							 FILE_ATTRIBUTE_NORMAL,
							 NULL);
	if (open_handle != INVALID_HANDLE_VALUE)
	{
		if (ReadFile(open_handle,
					 (LPVOID) &bootcontents,
					 2048,
					 &nbRead,
					 NULL))
		{
			if (nbRead > 0)
			{
				ptr = strchr(bootcontents, 13);		// Line feed
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
							if (strcmp(module1, module[ind]) == 0)
							{
								if (strcmp(oneline, filecontents[ind]) != 0)
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
								filecontents[ind][len] = 13;
								filecontents[ind][len+1] = 10;
								put_buffer(filecontents[ind], len+2, bootcontents_out);
								break;
							}
						}
						len = 0;
					}
					oneline[len] = 13;			// Add some line feed signs
					oneline[len+1] = 10;
					put_buffer(oneline, len+2, bootcontents_out);
					ptr = ptr + 2;
					strcpy(bootcontents, ptr);
					ptr = strchr(bootcontents, 13);
				}
			}
		}
		CloseHandle(open_handle);
	}


// If the boot file is to be been changed, push the existing files and create a new boot.ipnX
 
	if (changed1)
	{
		DeleteFile((LPCTSTR) ffile_oldbackup);
		MoveFile(ffile_backup, ffile_oldbackup);
		MoveFile(ffile, ffile_backup);

		open_handle = CreateFile((LPCTSTR) ffile,
								 GENERIC_READ|GENERIC_WRITE,
								 FILE_SHARE_READ|FILE_SHARE_WRITE,
								 NULL,
								 CREATE_ALWAYS,
								 FILE_ATTRIBUTE_NORMAL,
								 NULL);
		if (open_handle == INVALID_HANDLE_VALUE)
		{
			CopyFile(ffile_backup, ffile, false);
		}
		else
		{
			WriteFile(open_handle,
					  (LPVOID) &bootcontents_out,
					  strlen(bootcontents_out),
					  &nbWritten,
					  NULL);
			CloseHandle(open_handle);
		}
	}


// Now scan through the directory again to check all *.babsc.* files and compare
// with the contents of the boot.ipnX file. If there is mismatch between *.babsc.*
// files and module revisions given in boot.ipnX, update boot.ipnX with the existing
// *.babsc.* files (highest revision). An IPNA function change must be possible.

// First scan through the existing *.basc.* files. Store in Primary Memory.

	index = 0;
	bootcontents_out[0] = '\0';
	strcpy(ffile, TFTPBOOTDIR);
	search_handle = FindFirstFile(ffile, &lpFindFileData);

	while (search_handle != INVALID_HANDLE_VALUE)
	{
		strcpy(filename, (char*) &lpFindFileData.cFileName);
		p1 = strchr(filename, 46);				// Dot
		p2 = strchr((char*)(p1+1), 46);			// Dot
		if ((p1 != NULL) && (p2 != NULL))
		{
			strncpy(prefix, (char*)(p1+1), p2-p1-1);
			prefix[p2-p1-1] = '\0';
			if (strcmp(prefix, "babsc") == 0)
			{
				strncpy(prefix, filename, p1-filename);
				strcpy(module[index], prefix);
				module[index][p1-filename] = '\0';
				strcpy(filecontents[index], filename);
				filecontents[index][strlen(filename)] = '\0';
				index++;
			}
		}

		if (FindNextFile(search_handle, &lpFindFileData) == false)
			search_handle = INVALID_HANDLE_VALUE;
	}


// Now we go into the boot file and update according to 

	sprintf(ffile, "%s%d\0", BOOTFILE, ipnx);
	open_handle = CreateFile((LPCTSTR) ffile,
							 GENERIC_READ|GENERIC_WRITE,
							 FILE_SHARE_READ|FILE_SHARE_WRITE,
							 NULL,
							 OPEN_EXISTING,
							 FILE_ATTRIBUTE_NORMAL,
							 NULL);
	if (open_handle != INVALID_HANDLE_VALUE)
	{
		if (ReadFile(open_handle,
					 (LPVOID) &bootcontents,
					 2048,
					 &nbRead,
					 NULL))
		{
			if (nbRead > 0)
			{
				ptr = strchr(bootcontents, 13);		// Line feed
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
						for (ind=0; ind<index; ind++)
						{
							if (strcmp(module1, module[ind]) == 0)
							{
								strncpy(prefix, p1+1, (p3-p1-1));
								prefix[p3-p1-1] = '\0';
								if (strcmp(prefix, filecontents[ind]) == 0)
								{
									equality = true;
									break;
								}
								if (highest == -1)
									highest = ind;
								else
								{
									for (pos=0; pos<strlen(filecontents[ind]); pos++)
									{
										if (filecontents[ind][pos] > filecontents[highest][pos])
										{
											highest = ind;
											break;
										}
									}
								}
							}
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
								strcpy(prefix, _strupr(prefix));
								strncpy(p4-5, prefix, 5);
							}
						}
					}
					len = strlen(oneline);
					oneline[len] = 13;				// Add some line feed
					oneline[len+1] = 10;
					put_buffer(oneline, len+2, bootcontents_out);
					ptr = ptr + 2;
					strcpy(bootcontents, ptr);
					ptr = strchr(bootcontents, 13);
				}
			}
		}
	}
	if (changed2)
	{
		SetFilePointer(open_handle, 0, 0, FILE_BEGIN);
		WriteFile(open_handle,
				  (LPVOID) &bootcontents_out,
				  strlen(bootcontents_out),
				  &nbWritten,
				  NULL);
	}

	if (open_handle != INVALID_HANDLE_VALUE) CloseHandle(open_handle);


	if (changed1)
		return CMDFCPREP_SUCCESS;
	else
		return CMDFCPREP_NOCHANGE;
}


//******************************************************************************
// terminateMain
//
// terminate current file transfer. 
//
// INPUT:  exit code. 
// OUTPUT: -
// OTHER:  -
//******************************************************************************


void terminateMain(char *why, DWORD exitCode)
{
	int i;
	
	SetEvent(ThreadTermEvent);          // Kill running threads

	for (i=0; i<50; i++)
		if ((hThreads[i] != NULL) &&
			(hThreads[i] != INVALID_HANDLE_VALUE))
		{
			TerminateThread(hThreads[i], 0);
			CloseHandle(hThreads[i]);
		}


	WSACloseEvent(hListEvent);

	closesocket(list_sock);

	WSACleanup();


	CloseHandle(ipna_replied_event);
	CloseHandle(ThreadTermEvent);
	CloseHandle(MultiCmdTermEvent);

	for(i=0; i<MAX_IPNAS; i++)
		CloseHandle(MultiConTermEvent[i]);

	CloseHandle(debug_file_handle);

	DeleteCriticalSection(&connsec);
	DeleteCriticalSection(&ipnasec);

	EXIT_THREAD(exitCode);
}


//******************************************************************************
//
//   cleanup
//
//  clean up the things 
//
//******************************************************************************

void cleanup()
{

   char ch;
 
   // Check Tracking and undo done things
 
   if(Tracking[ListenSocket])
   {

      shutdown(list_sock,1);              // Half close socket
      while(recv(list_sock,&ch,1,0) == 1) // Call recv until it returns 0 or error
         ;                               // to clear unreceived data on socket 
 
      if(closesocket(list_sock) == SOCKET_ERROR)  // Close control socket
      {
         #ifdef DEBUGPRINT
            sprintf(LogMsg,"Closesocket failed with WSA error %d\n",WSAGetLastError());
            DoMsg(LogMsg);
         #endif
      }
    }

   // Check Tracking and undo done things

}


//******************************************************************************
//
// DoMsg
//
// Handle error messages 
//
//******************************************************************************

void DoMsg(char *LogMsg)
{
   DWORD nwrite;
/*
Debug logging to be done either to a console specially opened when main thread
starts, to a file or to a debug console when started in the debugger with
the console option. Use a special ftp command to enable debugging.
 
 
*/

   // Send output to debug file, if active, else check for debugging mode
 


#ifdef INC_SYSLOG
   if(syslog.getOutputFlag() == SYSLOG_STDERR)  // Set by service in console mode
   {
      printf("%s",LogMsg);               // In console mode, just print it
   } 
#endif
 
   // Send message to file 
 
   if(debug_file_active)
   {
      WriteFile(debug_file_handle,LogMsg,strlen(LogMsg),&nwrite,0); 
   }


}

