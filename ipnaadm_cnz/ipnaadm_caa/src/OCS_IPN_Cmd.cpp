
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
// OCS_IPN_cmd_main.cpp
//
// DESCRIPTION 
//
// DOCUMENT NO
// 8/190 55-CAA 109 0392
//
// AUTHOR 
// 0000926 EPA/D/U Michael Bentley
//
//******************************************************************************
// *** Revision history ***
// 0000926 MJB Created 
//******************************************************************************

#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#include <string.h>
#include <clusapi.h>

#include "OCS_IPN_Cmd.h"
#include "OCS_IPN_Global.h"


// #define DEBUGPRINT

//******************************************************************************
// OCS_IPN_cmd
//
// IPNA Adminstration command handler
//
// INPUT:  -
// OUTPUT:  
// OTHER:  -
//******************************************************************************



int main(int argc, char *argv[])
{

   WORD                wVersionRequested;       // Winsocket version available
   int                 result;                  // Result from function
   char                subcmd;                  // Sub-command
   WSADATA             wsaData;

   char                *datap;                  // Pointer to start of received data
   char                ch;

   int                 i,fnlen;
   int				   texit=0;
   unsigned int        ndata,k;
   char                state;                   // Signal parameters
   int				   ipnano;

   char                lstring[65535];          // String
   char                *pldir;
   int                 nrec,rest;  
   unsigned int        slen;

   char                cmd[100];                // Place to build a command
   STARTUPINFO         startupinfo;
   PROCESS_INFORMATION procinfo;

   char                myarg[6][100];           // Place to put converted characters
   int                 retrycnt;                // Counter for number of retries of recv


   unsigned short		node;
   unsigned short		ActiveNode;
   char					file_remote[80];
   char					file[32] = BOOTFILE;
   file[21] = 0;



   // Connect to valid winsock version  

   wVersionRequested = MAKEWORD(2,2);      
   
   // Start winsock2

   if((result = WSAStartup(wVersionRequested,&wsaData)) != 0 )
   {
      printf("WSAStartup failed with error %d\n",result);
      exit(-1);
   }

 
   // Parse command line
   // Try each command in both cases

   for(i=0;i<argc;i++)
   {
      strcpy(myarg[i],argv[i]);   // Copy the command line arguements so they can be modified
      CharLower(myarg[i]);        // Convert each argument to lower case
   }

   if(argc==1) goto synterr; // There are no arguements, just print help


   if(argc==2)				 // Test single argument commands
   {
      if((strcmp(myarg[1],"-help")==0) ||
         (strcmp(myarg[1],"-h")==0)    ||
         (strcmp(myarg[1],"/?")==0)      )
      {
         usage();			// Help command - print help message
         exit(2);
      }
      else if(strcmp(myarg[1],"-list") == 0)
      {
         server_connect();                    // Connect to the IPNAADM server

         // list command (no args) 
         // Send CMDOSDLIST

         ((struct cmdssreq_sig *)send_buf)->sh.sig_len = htons(SL_CMDLIST);
         ((struct cmdssreq_sig *)send_buf)->sh.sig_num = CMDLISTREQ;
         ((struct cmdssreq_sig *)send_buf)->sh.sig_ver = 0;
         if(!makeitsend(cmd_sock,send_buf,SL_CMDLIST+sizeof(struct sig_head),"CMDLISTREQ"))
            goto senderr;					  // Exit on error

      }
      else if(strcmp(myarg[1],"-osdump") == 0) 
      {
         server_connect();                    // Connect to the IPNAADM server
   
         //  send CMDOSDREQ

         ((struct cmdosdreq_sig *)send_buf)->sh.sig_len = htons(SL_CMDOSD_DIR);
         ((struct cmdosdreq_sig *)send_buf)->sh.sig_num = CMDOSDREQ;
         ((struct cmdosdreq_sig *)send_buf)->sh.sig_ver = 0;
         ((struct cmdosdreq_sig *)send_buf)->cmd = CMDOSD_DIR;

         if(!makeitsend(cmd_sock,send_buf,SL_CMDOSD_DIR+sizeof(struct sig_head),"CMDOSDREQ -dir"))
            goto senderr;                     // Exit on error


      }
      else if(strcmp(myarg[1],"-fcend") == 0) 
	  {

		  ActiveNode = checkNodeState(node);
		  if (node==0)			// Means 'We are now in the A-node'
			sprintf(file_remote, "\\\\192.168.202.2\\C$\\tftpboot\\boot.ipn \0");
		  else					// Means 'We are now in the B-node'
			sprintf(file_remote, "\\\\192.168.202.1\\C$\\tftpboot\\boot.ipn \0");

		  if (ActiveNode == 0)		// In Passive Node
			  for (i=0; i<4; i++)
			  {
				  file[20] = 48+i;
				  file_remote[36] = 48+i;
				  if (CopyFile(file_remote, file, false))
					 printf("copy %s %s, successful\n", file_remote, file);
				  else
					 printf("copy %s %s, failed\n", file_remote, file);
			  }
		  else
			  for (i=0; i<4; i++)
			  {
				  file[20] = 48+i;
				  file_remote[36] = 48+i;
				  if (CopyFile(file, file_remote, false))
					 printf("copy %s %s, successful\n", file, file_remote);
				  else
					 printf("copy %s %s, failed\n", file, file_remote);
			  }
		  exit(0);
	  }

      else
		  goto synterr;    // Command not recognised - print usage

 
   }
   else if(argc==3)
   {
      if((strcmp(myarg[1],"-osdump") == 0) && (strcmp(myarg[2],"-list") == 0))
      {
         server_connect();                    // Connect to the IPNAADM server
      
         // Handle the list sub-command, ask server for a list of dump files

         ((struct cmdosdreq_sig *)send_buf)->sh.sig_len = htons(SL_CMDOSD_LIST);
         ((struct cmdosdreq_sig *)send_buf)->sh.sig_num = CMDOSDREQ;
         ((struct cmdosdreq_sig *)send_buf)->sh.sig_ver = 0;
         ((struct cmdosdreq_sig *)send_buf)->cmd = CMDOSD_LIST;
 
         if(!makeitsend(cmd_sock,send_buf,SL_CMDOSD_LIST+sizeof(struct sig_head),"CMDOSDREQ -list"))
           goto senderr;                      // Exit on error


      }
      else if(((strcmp(myarg[1],"-debug")) == 0) && ((strcmp(myarg[2],"-start") == 0)))
      {
         server_connect();                    // Connect to the IPNAADM server
  
         //  send CMDOSDREQ(dir)
 
         ((struct cmddebugreq_sig *)send_buf)->sh.sig_len = htons(SL_CMDDEBUGREQ);
         ((struct cmddebugreq_sig *)send_buf)->sh.sig_num = CMDDEBUGREQ;
         ((struct cmddebugreq_sig *)send_buf)->sh.sig_ver = 0;
         ((struct cmddebugreq_sig *)send_buf)->cmd = CMDDEBUG_START;
 
         if(!makeitsend(cmd_sock,send_buf,SL_CMDDEBUGREQ+sizeof(struct sig_head),"DEBUG file start"))
            goto senderr;                     // Exit on error
 

      }
      else if(((strcmp(myarg[1],"-debug")) == 0) && ((strcmp(myarg[2],"-stop") == 0)))
      {
         server_connect();                    // Connect to the IPNAADM server
  
         //  send CMDDEBUGREQ
 
         ((struct cmddebugreq_sig *)send_buf)->sh.sig_len = htons(SL_CMDDEBUGREQ);
         ((struct cmddebugreq_sig *)send_buf)->sh.sig_num = CMDDEBUGREQ;
         ((struct cmddebugreq_sig *)send_buf)->sh.sig_ver = 0;
         ((struct cmddebugreq_sig *)send_buf)->cmd = CMDDEBUG_STOP;
 
         if(!makeitsend(cmd_sock,send_buf,SL_CMDDEBUGREQ+sizeof(struct sig_head),"DEBUG file stop"))  // Send the signal
            goto senderr;                     // Exit on error
 

      }
      else if(((strcmp(myarg[1],"-debug")) == 0) && ((strcmp(myarg[2],"-get") == 0)))
      {
         server_connect();                    // Connect to the IPNAADM server
  
         //  send CMDDEBUGREQ
 
         ((struct cmddebugreq_sig *)send_buf)->sh.sig_len = htons(SL_CMDDEBUGREQ);
         ((struct cmddebugreq_sig *)send_buf)->sh.sig_num = CMDDEBUGREQ;
         ((struct cmddebugreq_sig *)send_buf)->sh.sig_ver = 0;
         ((struct cmddebugreq_sig *)send_buf)->cmd = CMDDEBUG_GET;
 
         if(!makeitsend(cmd_sock,send_buf,SL_CMDDEBUGREQ+sizeof(struct sig_head),"DEBUG file get"))
            goto senderr;                     // Exit on error
 

      }
      else if(((strcmp(myarg[1],"-debug")) == 0) && ((strcmp(myarg[2],"-delete") == 0)))
      {
         server_connect();                    // Connect to the IPNAADM server
  
         //  send CMDDEBUGREQ
 
         ((struct cmddebugreq_sig *)send_buf)->sh.sig_len = htons(SL_CMDDEBUGREQ);
         ((struct cmddebugreq_sig *)send_buf)->sh.sig_num = CMDDEBUGREQ;
         ((struct cmddebugreq_sig *)send_buf)->sh.sig_ver = 0;
         ((struct cmddebugreq_sig *)send_buf)->cmd = CMDDEBUG_DELETE;
 
         if(!makeitsend(cmd_sock,send_buf,SL_CMDDEBUGREQ+sizeof(struct sig_head),"DEBUG file delete"))
            goto senderr;                    // Exit on error
 
      }
      else     // Command not recognised - print usage
		  goto synterr;

   }
   else if(argc==4)
   {
      if((strcmp(myarg[1],"-osdump") == 0) && (strcmp(myarg[2],"-get") == 0))
      {
         server_connect();                    // Connect to the IPNAADM server

         // Get <filename> - send CMDOSDREQ(get,filename)

         fnlen = strlen(myarg[3]);            // Length of filename
 
         ((struct cmdosdreq_sig *)send_buf)->sh.sig_len = htons(SL_CMDOSD_GET+fnlen);
         ((struct cmdosdreq_sig *)send_buf)->sh.sig_num = CMDOSDREQ;
         ((struct cmdosdreq_sig *)send_buf)->sh.sig_ver = 0;
         ((struct cmdosdreq_sig *)send_buf)->cmd = CMDOSD_GET;

         datap = &((struct cmdosdreq_sig *)send_buf)->filenam; // Pointer to filename in signal
         strcpy(datap,myarg[3]);                               // Copy filename into signal

         #ifdef DEBUGPRINT
            printf("Getting file %s\n",myarg[3]);
            print_signal();
         #endif

         if(!makeitsend(cmd_sock,send_buf,SL_CMDOSD_GET+sizeof(struct sig_head)+fnlen,"CMDOSDREQ -get"))
            goto senderr;                // Exit on error

      }

      else if((strcmp(myarg[1],"-fcprep") == 0) && (strcmp(myarg[2],"-ipnano") == 0))
      {
		 result = sscanf(myarg[3],"%d",&ipnano);
	     if ((result != 1) || (ipnano > 63))
         {
            printf("Error in ipna number argument %s\n",myarg[3]); // sscanf didn't match 1 number
            usage();
            my_exit(3, cmd_sock);                                 // Exit on error
         }

		 server_connect();                    // Connect to the IPNAADM server

         ((struct cmdfcprepreq_sig *)send_buf)->sh.sig_len = htons(SL_CMDFCPREPREQ);
         ((struct cmdfcprepreq_sig *)send_buf)->sh.sig_num = CMDFCPREPREQ;
         ((struct cmdfcprepreq_sig *)send_buf)->sh.sig_ver = 0;
         ((struct cmdfcprepreq_sig *)send_buf)->ipnano = ipnano;
 
         if(!makeitsend(cmd_sock,send_buf,SL_CMDFCPREPREQ+sizeof(struct sig_head),"FC Preparation"))
            goto senderr;                    // Exit on error

	  }

      else if((strcmp(myarg[1],"-fcrollb") == 0) && (strcmp(myarg[2],"-ipnano") == 0))
      {
		 result = sscanf(myarg[3],"%d",&ipnano);
	     if ((result != 1) || (ipnano > 63))
         {									// sscanf didn't match 1 number
            printf("Error in ipna number argument %s\n",myarg[3]);
            usage();
            my_exit(3, cmd_sock);           // Exit on error
         }

		 switch (rollBackFunctionChange(ipnano))
		 {
			case CMDFCPREP_SUCCESS:
				printf("\n\nCommand succeeded\n");
				texit = 0;
                break;
			case CMDFCPREP_NOROLLBACK:
				printf("\n\nRollback not possible\n");
				texit = 13;
                break;
            default:
                printf("\n\nCommand returned unknown value %d\n",
					    ((struct cmdfcprepconf_sig *)rec_buf)->result);
				goto senderr;
                break;
		 }
		 exit(texit);

	  }
	  else     // Command not recognised - print usage
		  goto synterr;

   }
   else if(argc==5)
   {
      if(strcmp(myarg[1], "-state") == 0)
      {
         // State command args:state<norm(al),sep(erated)> ipna<0..63>

         // Check argument to state - allow both cases, only look at 1st few chars

         if(strncmp(myarg[2], "normal", 4) == 0) 
         {
            state = CMDSS_NORM;
         }
         else if(strncmp(myarg[2], "separated", 3) == 0) 
         {
            state = CMDSS_SEP;
         }
         else
         {
             printf("Error in state command argument %s\n",myarg[2]);
             usage();
             my_exit(3, cmd_sock);    
         }

         // Check for ipna number argument next

         if(strcmp(myarg[3], "-ipnano") == 0)
         {
			result = sscanf(myarg[4],"%d",&ipnano);
			if ((result != 1) || (ipnano > 63))
            {
               printf("Error in ipna number argument %s\n",myarg[4]); // sscanf didn't match 1 number
               usage();
               my_exit(3, cmd_sock);                                  // Exit on error
            }
         }
         else
			 goto synterr;

         server_connect();                    // Connect to the IPNAADM server

         // Send CMDSSREQ

         ((struct cmdssreq_sig *)send_buf)->sh.sig_len = htons(SL_CMDSSREQ);
         ((struct cmdssreq_sig *)send_buf)->sh.sig_num = CMDSSREQ;
         ((struct cmdssreq_sig *)send_buf)->sh.sig_ver = 0;
         ((struct cmdssreq_sig *)send_buf)->ipnano = ipnano;
         ((struct cmdssreq_sig *)send_buf)->state = state;
         ((struct cmdssreq_sig *)send_buf)->substate = CMDSS_OPEN;

         if(!makeitsend(cmd_sock,send_buf,SL_CMDSSREQ+sizeof(struct sig_head),"CMDSSREQ"))
			goto senderr;                                    // Exit on error

      }
      else
		  goto synterr;

   }
   else
	   goto synterr;


   // Enter loop waiting for network events, the first should be the IDENTITYREQ signal

   while(true)
   {
      result = WaitForSingleObject(CmdEvent,CmdTimeout);  // Just wait for network events

      #ifdef DEBUGPRINT
         printf("WSO returned %d\n",result);
      #endif

      if(result == WAIT_OBJECT_0)
      {
         #ifdef DEBUGPRINT
            printf("Handling network event\n");
         #endif

         if(WSAEnumNetworkEvents(cmd_sock,CmdEvent,&cmd_events) == SOCKET_ERROR)
         {
            #ifdef DEBUGPRINT
               printf("WSAEnumNetworkEvents failed with error %d\n",WSAGetLastError());
            #endif
			my_exit(19,cmd_sock);
         }

         if(cmd_events.lNetworkEvents & FD_READ)     // There is something to read
         {
            if(!recvitall(cmd_sock,rec_buf,sizeof(sig_head),"Signal Header"))
            {
               printf("Read of signal header failed with WSA error %d\n",WSAGetLastError());
			   my_exit(18,cmd_sock);
            } 

            switch(((struct sig_head *)rec_buf)->sig_num)
            {
               case CMDSSCONF:
               {
                  #ifdef DEBUGPRINT
                     printf("Received CMDSSCONF\n");
                  #endif

                  // Print result of set state 

                  result = recv(cmd_sock,&ch,1,0);   // Receive dump sub-command from signal
                  if(result == SOCKET_ERROR || result != 1)
                  {
                     printf("Receive of dump sub-command failed with error %d\n",WSAGetLastError());
                     my_exit(17,cmd_sock);
                  }
                  switch(ch)
                  {
                     case CMDSS_SUCCESS:
                        printf("\n\nCommand succeeded\n");
						texit = 0;
                        break;

                     case CMDSS_UNKNOWN:
                        printf("\n\nIPNA reports unknown state\n");
						texit = 4;
                        break;

                     case CMDSS_EX_CP:
                        printf("\n\nIPNA reports order received in EX-CP state\n");
						texit = 5;
                        break;

                     case CMDSS_NOTCON:
                        printf("\n\nIPNA is not connected\n");
						texit = 6;
                        break;

                     default:
                        printf("\n\nCommand returned unknown value %d\n",((struct cmdssconf_sig *)rec_buf)->result);
						texit = 16;
                        break;
                  }
                  my_exit(texit,cmd_sock);                        // End of command
                  break;
               }

               case CMDFCPREPCONF:
               {
                  #ifdef DEBUGPRINT
                     printf("Received CMDFCPREPCONF\n");
                  #endif

                  // Print result of function change preparation 

                  result = recv(cmd_sock,&ch,1,0);   // Receive result from signal
                  if(result == SOCKET_ERROR || result != 1)
                  {
                     printf("Receive of FC preparation conf, failed with error %d\n",WSAGetLastError());
                     my_exit(14,cmd_sock);
                  }

                  switch(ch)
                  {
                     case CMDFCPREP_SUCCESS:
                        printf("\n\nCommand succeeded\n");
						texit = 0;
                        break;

                     case CMDFCPREP_NOCHANGE:
                        printf("\n\nNo Function Change files loaded\n");
						texit = 12;
                        break;

                     default:
                        printf("\n\nCommand returned unknown value %d\n",((struct cmdfcprepconf_sig *)rec_buf)->result);
						texit = 16;
                        break;
                  }
                  my_exit(texit, cmd_sock);              // End of command
                  break;
               }

               case CMDLISTCONF:
               {
                  #ifdef DEBUGPRINT
                     printf("Received CMDLISTCONF\n");
                  #endif

                  printf("\n\nConnected IPNAs are :\n\n");
                  for(i=0; i<MAX_IPNAS; i++)
                  {
                     result = recv(cmd_sock,&ch,1,0); // Read one connection list entry
					 if(result == SOCKET_ERROR || result != 1)
					 {
                        printf("Receive of dump sub-command failed with error %d\n",WSAGetLastError());
                        my_exit(17, cmd_sock);
					 }
                     if(ch >0)
                     {
                        for(int m=0;m<ch;m++)          // Might have multiple connections !
                        {
                           printf("    ipna%02d ",i);
                           if((i>0) && (i%8 == 0)) printf("\n");   // Just 8 per line
                        }
                     }
                  }
                  printf("\n\nendlist\n");             // Finish line
                  exit(0);                           // End of command
                  break;
               }

               case CMDOSDCONF:
               {
                  #ifdef DEBUGPRINT
                     printf("Received CMDOSDCONF\n");
                  #endif

                  result = recv(cmd_sock,&ch,1,0);   // Receive sub-command from socket
                  if(result == SOCKET_ERROR || result != 1)
                  {
                     printf("Receive of dump sub-command failed with error %d\n",WSAGetLastError());
                     my_exit(17,cmd_sock);
                  }
                  switch(ch)                         // Test which sub-command made reply
                  {
                     case CMDOSD_DIR:
                     {
                        #ifdef DEBUGPRINT
                           printf("Received CMDOSD_DIR\n");
                        #endif

                        // I'm on the local machine, so pop up an explorer window in the directory
                        // returned by signal

                        // Length of directory string from signal length - signal header - result - command

                        result = recv(cmd_sock,&ch,1,0);                   // Receive and discard result
                        if(result == SOCKET_ERROR || result != 1)
                        {
                          printf("Receive of CMDOSD_DIR result failed with error %d\n",WSAGetLastError());
                          my_exit(8,cmd_sock);
                        }
                        slen = ntohs(((struct sig_head *)rec_buf)->sig_len) - 2;

                        // recv won't necessarily read all that it is asked to - loop until it does

                        nrec = 0;
                        pldir = lstring;                                   // Set pointer to start of string

                        if(!recvitall(cmd_sock,pldir,slen,"CMDOSD_DIR path"))
                        {
                           goto senderr;                                   // Exit on error
                        }

                        pldir[slen] = '\0';                                // Terminate string
                        sprintf(cmd,"C:\\Winnt\\explorer.exe %s",lstring); // Build the explorer command

                        #ifdef DEBUGPRINT
                           printf("Starting explorer in %s\n",cmd);
                        #endif

                        GetStartupInfo(&startupinfo);                     // Fill in the statup info structure needed for ...
                        if(!CreateProcess(0,cmd,0,0,FALSE,CREATE_NEW_CONSOLE,0,0,&startupinfo,&procinfo))  // Start explorer in log directory
                        {
                           printf("\n\nStart of explorer in directory %s failed with error %d\n",lstring,GetLastError());
						   my_exit(7, cmd_sock);
                        }
                   
                        break;
                     }
                     case CMDOSD_LIST:
                     {
                        #ifdef DEBUGPRINT
                           printf("Received CMDOSD_LIST\n");
                        #endif

                        result = recv(cmd_sock,&ch,1,0);                   // Read command result
                        if(result == SOCKET_ERROR || result != 1)
                        {
                           printf("Receive of CMDOSD_LIST result failed with error %d\n",WSAGetLastError());
                           my_exit(8,cmd_sock);
                        }
                        if(ch == CMDOSD_SUCCESS)
                        {
                           // If result !=0, there are dump files available and data contains a single
                           // string which is a list of file names separated by <CR><LF>

                           memset(lstring,0,sizeof(lstring));              // Clear the string (terminates its too)

                           slen = ntohs(((struct sig_head *)rec_buf)->sig_len)  - SL_CMDOSD_LISTR;
                           printf("\n\nList of available dump files:\n\n");
 

                           if(!recvitall(cmd_sock,lstring,slen,"CMDOSD_LIST"))
                           {
                              goto senderr;                               // Exit on error
                           }
 
                           printf("%s\n",lstring);                        // Print the received string
 
                        }
                        else
                        {
                           printf("\n\nThere are no dump files available\n");
                        }
                        break;
                     }
                     case CMDOSD_GET:
                     {
                        #ifdef DEBUGPRINT
                           printf("Received CMDOSD_GET\n");
                        #endif
 
                        result = recv(cmd_sock,&ch,1,0);                   // Read command result
                        if(result == SOCKET_ERROR || result != 1)
                        {
                           printf("Receive of CMDOSD_GET result failed with error %d\n",WSAGetLastError());
                           my_exit(8, cmd_sock);
                        }
                        if(ch == CMDOSD_SUCCESS)   
                        { 
                           // If result !=0, there was a dump file of the name requested by
                           // the get sub-command. The dump files are about 8K long so I'll
                           // read the whole signal and print out one char at a time 

                           // sig_len is a 16 bit value so watch byte order

                           ndata = ntohs(((struct cmdosdconf_sig *)rec_buf)->sh.sig_len)
                                  - SL_CMDOSD_GETR;

                           retrycnt = 0;

                           for(k=0;k<ndata;k++)
                           {
                              result = recv(cmd_sock,&ch,1,0); 
                              if(result == SOCKET_ERROR)
                              {
                                 if((result=WSAGetLastError()) == WSAEWOULDBLOCK)
                                 {
                                    if(retrycnt++ > MAXRETRY)
                                       break;  // Break out of for loop, too many retries
                                    else
                                       k--;    // Data not ready, re-do 
                                 }
                                 else
                                 {
                                    printf("recv died during osdump get with error %d\n",WSAGetLastError());
                                    break;
                                 }
                              }
                              else
                                 putchar(ch);  // Print the char if received ok
                           }
                           printf("\n");
                        } 
                        else
                        { 
                           printf("The primary dump:\n\nFile not found\n");
                        }
                        break;
                     }
                     default: 
                     {
                         #ifdef DEBUGPRINT
                            printf("Received unhandled sub-command %d\n",(int)ch);
                         #endif
                     }
                  }
                  my_exit(0,cmd_sock);   

                  break;
               }
               case CMDDEBUGCONF:
               {
                  result = recv(cmd_sock,&subcmd,1,0);   // Receive sub-command from socket
                  if(result == SOCKET_ERROR || result != 1)
                  {
                     printf("Receive of debug sub-command failed with error %d\n",WSAGetLastError());
                     my_exit(-1,cmd_sock);
                  }
                
                  result = recv(cmd_sock,&ch,1,0);   // Receive dump sub-command from signal
                  if(result == SOCKET_ERROR || result != 1)
                  {
                     printf("Receive of debug command result failed with error %d\n",WSAGetLastError());
                     my_exit(-1,cmd_sock);
                  }

                  #ifdef DEBUGPRINT
                     printf("debug sub-command = %d, result = %d\n",subcmd,ch);
                  #endif

                  switch(subcmd)
                  {
                     case CMDDEBUG_GET:
                     {
                        #ifdef DEBUGPRINT
                           printf("Received debug get with %d data\n",ntohs(((struct sig_head *)rec_buf)->sig_len));
                        #endif

                        if(ch == CMDDEBUG_FAIL)
                           printf("\n    Debug command failed\n");
                        else
                        {
                           // Get has returned the contents of the debug file
                           // This is how much data we're expecting
 
                           slen = ntohs(((struct sig_head *)rec_buf)->sig_len);
                           rest = slen -SL_CMDDEBUGCONF;

 
                           // Read 256 byte chunks of data and dump into file

                           while(rest !=0)
                           {
                              if(rest > 256)
                                 nrec = 256;
                              else
                                 nrec = rest;



                              if(!recvitall(cmd_sock,lstring,nrec,"Dump data"))
                              {
                                  printf("recv died during debug file get\n");
                                  exit(10);
                              }
                              else
                              {
                                  lstring[nrec] = '\0';  // Turn buffer into string
                                  printf("%s",lstring); // Print the buffer
                              }
                              if(rest > 256)
                                 rest -= 256;
                              else
                                 rest = 0;
                           }
                        }
                        break;
                     }
                     case CMDDEBUG_START:
                     case CMDDEBUG_STOP:
                     case CMDDEBUG_DELETE:
                     {
                        if(result == CMDDEBUG_OK)
                           printf("\n    Debug command executed succesfully\n");
                        else if(result == CMDDEBUG_FAIL)
                           printf("\n    Debug command failed\n");
                        else
                           printf("\n    Debug command returned unexpected value %d\n",result);
                        break;
                     }


                     default:
                     {
                        printf("\n    Debug command returned unexpected value %d\n",subcmd);
                        break;
                     }
                  }

                  my_exit(0,cmd_sock); 

                  break;
               }
               default:
               {
                   #ifdef DEBUGPRINT
                      printf("Received unhandled signal %d\n",((struct sig_head *)rec_buf)->sig_num);
                   #endif
               }
            }
         }
         else if(cmd_events.lNetworkEvents & FD_CLOSE)    // Socket was closed, connection lost
         {
            printf("Connection to IPNAADM server lost, exiting\n");
            my_exit(15,cmd_sock);                        // Exit on error

         }
         else
         {
             #ifdef DEBUGPRINT
                printf("Received unhandled network event %d\n",cmd_events.lNetworkEvents);
             #endif
         }
      }
      else if(result == WAIT_TIMEOUT)
      {
         printf("\n\n  Error waiting for network events\n");
         my_exit(20,cmd_sock);                                    // Exit on error

      }
      else
      {
          #ifdef DEBUGPRINT
             printf("WaitForSingleObject returned unhandled event %d\n",result);
          #endif
      }

   }
   return(0);

synterr:
   printf("Syntax error in command :");
   for(i=0;i<argc;i++) printf("%s ",myarg[i]);
   printf("\n");
   usage();
   my_exit(2, cmd_sock);  

senderr:
   printf("Server communication failure\n");
   usage();
   my_exit(21, cmd_sock);

}


//******************************************************************************
//
// usage
//
// Print a help message
// 
// 
//****************************************************************************** 

void usage(void)
{
   printf("\n\n\
Usage 'ipnaadm -command', where command is one of :\n\
\n\
   -state <norm(al)|sep(arated) -ipnano <0..63>\n\
    (order a state change on a connected IPNA)\n\
   -list\n\
    (List the connected IPNAs)\n\
   -osdump -subcommand\n\
       where subcommand is one of :\n\
                <none>           start an explorer window in the directory containing the OSE dumps\n\
                -list            list the available dump files\n\
             or -get <filename>  get a particular dump file and send to STDOUT\n\
                                 (no path needed)\n\
   -fcprep   -ipnano <0..63>     prepare IPNA function change configuration files in AP\n\
   -fcrollb  -ipnano <0..63>     rollback IPNA function change configuration files in AP\n\
   -fcend                        end IPNA function change (equalize Passive with Active node)\n\
\n\
   -help\n\
       Print this message\n\
\n\
");

}

//******************************************************************************
//
// server connect
//
// Connect to the ipnaadm server
// 
// 
//******************************************************************************
 
void server_connect()
{
   struct in_addr      *ptr;                    // Pointer used in gethostbyname
   SOCKADDR_IN         s_cmd;                   // Command socket address 
   short               cmd_port;                // Port for command socket 
   struct servent      *servp;                  // Pointer to service entry
   struct hostent      *hostp;                  // Pointer to host entry
   char                local_host[256];
   int                 result;
   bool                wait_ident;

   // Create command socket

   if((cmd_sock = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,NULL,0,NULL)) == INVALID_SOCKET)
   {
      printf("Socket creation failed with WSA error %d\n",WSAGetLastError());
      exit(-1);
   }
   
   // Get port number for this service from operating system

   if((servp=getservbyname("ipnaadm","tcp")) == NULL) // Entry for ipnaadm isn't in 
   {                                                  // the service file
      cmd_port = htons(ADM_PORT_NUMBER);              // Use this as default
   }
   else                                               // Entry for ipnaadm service exits 
   {
      cmd_port = servp->s_port;                       // Use OS port number
   }
 
   // Try to get local host ip address

   gethostname(local_host,sizeof(local_host));
   hostp = gethostbyname(local_host);



   if((ptr = (struct in_addr *)hostp->h_addr_list[0]) == NULL)
   {
       printf("Can't get IP address of local host, aborting\n");
       my_exit(-1,cmd_sock);                                    // Exit on error

   }


//  printf("\n%s:\n\n\n",inet_ntoa(*ptr ));
// exit(0);

   // Specify a local endpoint address for command socket

   s_cmd.sin_family          =     PF_INET;
   s_cmd.sin_addr.s_addr     =     ptr->s_addr;
   s_cmd.sin_port            =     cmd_port;

 

   // connect to server

   if(connect(cmd_sock,(struct sockaddr *) &s_cmd, sizeof(s_cmd)) == SOCKET_ERROR )
   {
      if((result = WSAGetLastError()) == WSAECONNREFUSED)
         printf("Connection refused\n");
      else
         printf("Connect failed with WSA error %d\n",result);
      my_exit(-1,cmd_sock);                                    // Exit on error

   }

   // create socket events

   CmdEvent = WSACreateEvent();                       // Create an event object for control socket events

   WSAEventSelect(cmd_sock,CmdEvent,FD_READ|FD_CLOSE);

   // Enter loop waiting for network events until received the IDENTITYREQ signal

   wait_ident = true;

   while(wait_ident == true)
   {
      if(WaitForSingleObject(CmdEvent,INFINITE) == WAIT_OBJECT_0)     // Just wait for network events
      {
         WSAEnumNetworkEvents(cmd_sock,CmdEvent,&cmd_events);
         WSAResetEvent(CmdEvent);                    // WSA events are manually reset, so doit
         if(cmd_events.lNetworkEvents & FD_READ)     // There is something to read
         {
            result = recv(cmd_sock,rec_buf,sizeof(struct sig_head),0); // Read whole signal
            if(result == SOCKET_ERROR)
            {
               printf("Read of signal header failed with WSA error %d\n",WSAGetLastError());
               my_exit(-1,cmd_sock);                                    // Exit on error

            }
            if(result < sizeof(struct sig_head))
            {
               printf("Didn't read full signal header\n");
               my_exit(-1,cmd_sock);                                    // Exit on error

            }


            if(((struct sig_head *)rec_buf)->sig_num == IDENTITYREQ )
            {
              
                // Send IDENTITYCONF

                ((struct idconf_sig *)send_buf)->sh.sig_len = htons(SL_IDENTITYCONF);
                ((struct idconf_sig *)send_buf)->sh.sig_num = IDENTITYCONF;
                ((struct idconf_sig *)send_buf)->sh.sig_ver = 0;
                ((struct idconf_sig *)send_buf)->id         = ID_COMMAND;

                if(!makeitsend(cmd_sock,send_buf,SL_IDENTITYCONF+sizeof(struct sig_head),"IDENTITYCONF"))
                my_exit(-1,cmd_sock);                                              // Exit on error


                else
                   wait_ident = false;   // Break out of the while loop
            }
         }
      }
   }
}



//******************************************************************************
// makeitsend
//
// Send a signal and make sure it's all sent
//
//******************************************************************************

bool makeitsend(SOCKET its_sock,char *its_buf,int its_len, char *its_name)
{
   int result,remain;
   char *datap;

   result = send(its_sock,its_buf,its_len,0); // First go at sending entire signal
   if(result == SOCKET_ERROR)                 // Handle inet errors
   {
      printf("Send of %s signal failed with WSA error %d\n",its_name,WSAGetLastError());
      return(false);
   }
   else if(result < its_len)
   {
      datap = its_buf + result;                // Make a pointer to place in send buf not yet sent 
      remain = its_len - result;               // remainder
      while(remain>0)                          // loop until nothing remains
      {
         result = send(its_sock,datap,remain,0); // Try to send remainder of signal
         if(result == SOCKET_ERROR)
         {
            printf("Send of %s signal failed with WSA error %d\n",its_name,WSAGetLastError());
            return(false);
         }
         remain-=result;
      }
   }
   return(true);
}

//******************************************************************************
// recvitall
//
// Receive multiple bytes from a socket and ensure it is all received
//
//******************************************************************************

bool recvitall(SOCKET its_sock, char *its_buf, int its_len, char *its_name)
{
    int	 result, aktuell, fel;
    char *datap;
 

	datap = its_buf;
    aktuell = its_len;

	while (aktuell)
	{
		if (aktuell > 1000)
		{
			aktuell = 1000;
		}
		for (int i=0; i<50; i++)
		{
			if ((result = recv(its_sock, datap, aktuell, 0)) != SOCKET_ERROR) break;
			fel = WSAGetLastError();

			if ((fel == WSAEWOULDBLOCK)| (fel == WSAEWOULDBLOCK))
			{
				Sleep(50);
				continue;
			}
			else
			{
				printf("Receive of %s failed with WSA error %d\n", its_name, fel);
				return(false);
			}
		}
		its_len -= result;
		aktuell  = its_len;
		datap   += result;
		if (aktuell < 1) aktuell = 0;
	}
	return(true);
}

//******************************************************************************
// my_exit
//
// Clean up and die
//
// 
//******************************************************************************

void my_exit(int err_code, SOCKET my_sock)
{
  char ch;


  shutdown(my_sock,1);                  // Half close socket
  while(recv(my_sock,&ch,1,0) == 1)     // Call recv until it returns 0 or error
     ;                                  // to clear unreceived data on socket 
 
  closesocket(my_sock);                 // Close control socket

  exit(err_code);                       // Exit
}


//******************************************************************************
//  rollBackFunctionChange()
// return values:
// CMDFCPREP_SUCCESS(0)   : Operation succeeded
// CMDFCPREP_NOCHANGE(1)  : No Function Change files loaded
// CMDFCPREP_FAULT(2)     : Other fault
// CMDFCPREP_NOROLLBACK(3): No rollback possible to perform
//******************************************************************************
DWORD rollBackFunctionChange(char ipnx)
{
	WIN32_FIND_DATA	dummy;
	char ffile[80];
	char ffile_backup[80];
	char ffile_oldbackup[80];
	char ffile_temp[80];

	sprintf(ffile,           "%s%d\0", BOOTFILE,      ipnx);
	sprintf(ffile_backup,    "%s%d\0", BACKUPFILE,    ipnx);
	sprintf(ffile_oldbackup, "%s%d\0", OLDBACKUPFILE, ipnx);
	sprintf(ffile_temp,      "%s%d\0", TEMPBOOTFILE,  ipnx);


	if (FindFirstFile((LPCTSTR)ffile_backup, &dummy) == INVALID_HANDLE_VALUE)
	{
		if (!MoveFile(ffile_oldbackup, ffile_backup)) return CMDFCPREP_NOROLLBACK;
	}

	if (FindFirstFile((LPCTSTR)ffile, &dummy) != INVALID_HANDLE_VALUE)
	{
		if (!CopyFile(ffile, ffile_temp, false)) return CMDFCPREP_NOROLLBACK;

		if (!DeleteFile(ffile))
		{
			DeleteFile(ffile_temp);
			return CMDFCPREP_NOROLLBACK;
		}
	}

	if (!MoveFile(ffile_backup, ffile))
	{
		MoveFile(ffile_temp, ffile);
		return CMDFCPREP_NOROLLBACK;
	}

	if (!MoveFile(ffile_oldbackup, ffile_backup))
		DeleteFile(ffile_oldbackup);

	DeleteFile(ffile_temp);
	return CMDFCPREP_SUCCESS;
}


//******************************************************************************
// checkNodeState(unsigned short &node)
//
// Check whether I am executing in Active node
//
// 
//******************************************************************************

unsigned short checkNodeState(unsigned short &node)
{
	HCLUSTER	clusterH = 0;
	HGROUP		groupH = 0;
	
    clusterH=OpenCluster(NULL);
	if (clusterH==0)  // unable to open cluster
	{
		if(clusterH) CloseCluster(clusterH);
		if(groupH) CloseClusterGroup(groupH);
		return 1; //We regard ourself as active.
	}

	groupH=OpenClusterGroup(clusterH, L"Cluster Group");
	if (groupH==0)   // unable to open clustergroup
	{ 
		if(clusterH) CloseCluster(clusterH);
		if(groupH) CloseClusterGroup(groupH);
		return 1; //We regard ourself as active.
	}
    
	unsigned int nodeState;
	DWORD	namelen=64;
	LPWSTR	currentNode=new WCHAR[namelen];
	GetComputerNameW(currentNode,&namelen);
	
	namelen=64;
	LPWSTR	activeNode=new WCHAR[namelen];
	DWORD	status=GetClusterGroupState(groupH,activeNode,&namelen);
	if (status==ClusterGroupStateUnknown)   
	{				// Unable to get clustergroup status
		if(clusterH) CloseCluster(clusterH);
		if(groupH) CloseClusterGroup(groupH);
		delete[] activeNode;
		delete[] currentNode;
		return 1; //We regard ourselves as active.
	}
	unsigned short p;
	node = 0;
	for (p=0; p<namelen; p++)
		if ((currentNode[p]==66)||(currentNode[p]==98)) node = 1;

	if (wcscmp(activeNode,currentNode)==0)
	{
		// This node is active
		nodeState = 1;
	}
	else
	{
		// This node is Not active
		nodeState = 0;
	}

	if(clusterH) CloseCluster(clusterH);
	if(groupH) CloseClusterGroup(groupH);
	delete[] activeNode;
	delete[] currentNode;
	
	return nodeState;
}


//******************************************************************************
// print_signal
//
// print the signal pointed at by signal
//
// 
//******************************************************************************

void print_signal(void)
{
   int sig_len;
   int i;

   sig_len = ntohs(((struct cmdosdreq_sig *)send_buf)->sh.sig_len);

   printf("Signal header : length %d, number %d, version %d\n",sig_len,
           ((struct cmdosdreq_sig *)send_buf)->sh.sig_num,((struct cmdosdreq_sig *)send_buf)->sh.sig_ver);
   printf("Data :\n");
   for(i=0;i<sig_len;i++)
   {
       printf("%0X ",send_buf[i]);
       if((i>0) && (i%16 == 0))
         printf("\n");    // new line after 16 chars
 
   } 
   printf("\n\n");

}