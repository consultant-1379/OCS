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
// OCS_IPN_Service.cpp
//
// DESCRIPTION 
// This file implements the parts of the ipnaadm service that communicates with
// the Windows Service Control Manager. This code starts ipnaadmMain,
//
// DOCUMENT NO
// 4/190 89-CAA 109 0392
//
// AUTHOR 
// 000803 EPA/D/U Michael Bentley
//
//******************************************************************************
// === Revision history ===
// 0000803 MJB Created. 
//******************************************************************************

//******************************************************************************
//ipnaadmService.cpp
//******************************************************************************
//#define _WIN32_WINNT 0x400

//#include <windows.h>
//#include <winbase.h>
//#include <process.h>  //_beginthreadex, _endthreadex
#include <stdio.h>
#include <iostream>
#include <stddef.h>   //errno

#include "OCS_IPN_Global.h"
#include "OCS_IPN_Service.h"



#ifdef INC_SYSLOG

   #include "CPS_syslog.h"

   #ifdef _DEBUG
      CPS_syslog syslog("ipnaadm", SYSLOG_EVENTLOG, CPS_syslog::LVLDEBUG, true);
   #endif
#endif

//==============================================================================
// initService
//
// Initializes the service by starting the working thread (ipnaadmMain).
//
// INPUT:  -
// OUTPUT: -
// OTHER:  -
//==============================================================================

void initService(void)
{
   DWORD id;

   // Start the thread for ipnaadmMain
   
   hIpnaAdmMainThread = CREATE_THREAD(SECURITY_ATTRIBUTES, 
                                    THREAD_STACK_SIZE, 
                                    (LPTHREAD_START_ROUTINE)OCS_IPN_Main,
                                    0, //No arguments
                                    0, //Start active (not suspended)
                                    &id);

   if(hIpnaAdmMainThread == 0)
   {
   
      // No thread was created, terminate the service, terminateService will 
      // not return.
      
      terminateService(errno);
   }
   
   // Create an event that is used when the working threads are stopped.
   // This is a manual reset event and the initial state of the event is not 
   // signalled. This event has no name in the global name space.
   
   if(!(hMainObjectArray[SERVICE_ABORT_EVENT] = CreateEvent(SECURITY_ATTRIBUTES, 
                                                              TRUE,
                                                              FALSE,
                                                              NULL)) )
   {
      // It was not possible to create the event. Terminate execution and 
      // release resources, terminateService will not return.
      
      terminateService(GetLastError());
   }

   // Set serviceState to indicate that the service is up and running.
   
   serviceState = SERVICE_RUNNING;

} 



//==============================================================================
// stopService
//
// Stops the service by sending an event to OCS_IPN_Main ordering it to abort.
// OCS_IPN_Main will close its thread. This will cause the main thread to be
// signalled, and the serviceMain function to terminate.
//
// INPUT:  -
// OUTPUT: -
// OTHER:  An event is set to order OCS_IPN_Main to abort.
//==============================================================================

void stopService(void)
{
   // Stop the working threads by setting an event that will abort them
   // in a controlled way. The main thread handle will be signalled when it has
   // ended. This will terminate the serviceMain function.
   
   if(!SetEvent(hMainObjectArray[SERVICE_ABORT_EVENT]))
   {
      // It was not possible to set the event. Terminate in a non 
      // controlled way, terminateService will not return.
      
      terminateService(GetLastError());
   }

   // Wait for the termination of the service via the servicaMain function 
   // (the main threads handle is signalled) or a timeout. If the timeout 
   // occurs will the service terminate it self.

   durationTime = STOP_SUPERVISION_TIME;

   if(!SetWaitableTimer(hStopSupervisionTimer,(LARGE_INTEGER*)&durationTime, 
                           0,NULL,NULL,FALSE))
   {
      // It was not possible to set the timer. Terminate in a non 
      // controlled way, terminateService will not return.
   
      terminateService(GetLastError());
   }
} 


//==============================================================================
// sendStatusToSCM
//
// This function reports the status of the service to the Service Control 
// Manager(SCM). The SCM is functionality inside Windows that handles services,
//
// INPUT:  
// dwCurrentState  - The current state of the service
//
//                   SERVICE_STOPPED          - The service is not running
//                   SERVICE_START_PENDING    - The service is starting
//                   SERVICE_STOP_PENDING     - The service is stopping
//                   SERVICE_RUNNING          - The service is running
//                   SERVICE_CONTINUE_PENDING - The service continue is pending
//                   SERVICE_PAUSE_PENDING    - The service pause is pending
//                   SERVICE_PAUSED           - The service is paused
//
// dwWin32ExitCode - Specifies an Win32 error code that the service uses to 
//                   report an error that occurs when it is starting or 
//                   stopping.
//                   In case of no fault use NO_ERROR.
//                   Not used if dwServiceSpecificExitCode is not 0.
//
// dwServiceSpecificExitCode - Specifies a service specific error code that the 
//                             service returns when an error occurs while the 
//                             service is starting or stopping.
//                             Use 0 or user defined service exit code.
//
// dwCheckPoint - Specifies a value that the service increments periodically to
//                report its progress during a lengthy start, stop, pause, 
//                or continue operation. (See Windows SERVICE_STATUS.)
//
// dwWaitHint   - Specifies an estimate of the amount of time, in milliseconds,
//                that the service expects a pending start, stop, pause, 
//                or continue operation to take before the service makes its 
//                next call to the SetServiceStatus function.
//                (See Windows SERVICE_STATUS.)
//
// OUTPUT: -
// OTHER:  -
//==============================================================================

void sendStatusToSCM(DWORD dwCurrentState,
                     DWORD dwWin32ExitCode, 
                     DWORD dwServiceSpecificExitCode,
                     DWORD dwCheckPoint,
                     DWORD dwWaitHint)
{
   SERVICE_STATUS serviceStatus;

   // Fill in all of the SERVICE_STATUS fields
   // ----------------------------------------
    
   serviceStatus.dwCurrentState = dwCurrentState;
   serviceStatus.dwCheckPoint = dwCheckPoint;
   serviceStatus.dwWaitHint = dwWaitHint;

   // Type this service as a service running in it´s own process, 
   // not interacting with the desktop (SERVICE_WIN32_OWN_PROCESS).
    
   serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

   // If in the process of something, do not accept control events, 
   // else accept stop and shutdown.
    
   if((dwCurrentState == SERVICE_START_PENDING) |
      (dwCurrentState == SERVICE_STOP_PENDING))
   {
      serviceStatus.dwControlsAccepted = 0;
   }
   else
   {
      serviceStatus.dwControlsAccepted =  
            SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
   }

   // If a specific exit code is defined, set up the win32 service exit code 
   // to this specific exit code. Else use the Win32 exit code.
    
   if(dwServiceSpecificExitCode == 0)
      serviceStatus.dwWin32ExitCode = dwWin32ExitCode;
   else
      serviceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;

   serviceStatus.dwServiceSpecificExitCode = dwServiceSpecificExitCode;

   // Pass the status record to the SCM
    
   if(SetServiceStatus(serviceStatusHandle_ipnaadm, &serviceStatus)) 
   {
      // Status successfully set.
        
      return;
   }
   else
   {
      // Some error has occured. Report the error and stop the service.
      // There is some fault in the cooperation with the Service Control 
      // Manager. serviceStatusHandle_ipnaadm is set to 0 to avoid cyclic calls to
      // sendStatusToSCM. The terminateService function will not return.
        
      serviceStatusHandle_ipnaadm = 0;
      terminateService(GetLastError());    
   }
} 


//==============================================================================
// serviceCtrlHandler
//
// Dispatches orders received from the Service Control Manager. This function is
// called by the Windows Service Control Manager.
//
// INPUT: controlCode - An order from the Service Control Manager.
//                      SERVICE_CONTROL_STOP,     SERVICE_CONTROL_PAUSE, 
//                      SERVICE_CONTROL_CONTINUE, SERVICE_CONTROL_INTERROGATE, 
//                      SERVICE_CONTROL_SHUTDOWN
// OUTPUT: -
// OTHER:  -
//==============================================================================

void serviceCtrlHandler(DWORD order) 
{
   // Check what type of order that is received from the Windows Service
   // Control Manager.
    
   switch(order)
   {
      // There is no START option because
      // ServiceMain gets called on a start
        

      // Stop the service
        
      case SERVICE_CONTROL_STOP:
      {
         // Tell the SCM what's happening
            
         serviceState = SERVICE_STOP_PENDING;
         sendStatusToSCM(serviceState, NO_ERROR, 0, 1, SERVICE_STOP_TIME);
         stopService();                               // Stop the service

         // Return to avoid a faulty state being sent to the SCM.
         // (We don't know if we will return or if terminateService will 
         // execute first.)
            
         return;
      }

      // Update current status
        
      case SERVICE_CONTROL_INTERROGATE:
      {

         // it will fall to bottom and send status
            
         break;
      }

      // Do nothing in a shutdown. Could do cleanup
      // here but it must be very quick.
        
      case SERVICE_CONTROL_SHUTDOWN:
      {
         // Terminate, the process will exit and all resources will be 
         // returned. The dll-counters will not be handled properly, but 
         // this is not a problem because the system is being shut down.
         // The terminateService function will not return.
            
         terminateService(ADM_ERR_SERVICE_SHUTDOWN);
      }

      default:
         break;
   }

   // Return current status to the Service Control Manager
    
   sendStatusToSCM(serviceState,
                   NO_ERROR,
                   0,
                   SERVICE_NOTHING_PENDING,
                   SERVICE_NOTHING_PENDING);
} 


//==============================================================================
// terminateService
//
// Handle the termination of the service. 
//
// If an internal error has occured the process will terminate without 
// returning any resources. But this will be handled properly by the operating 
// system. As this is an exceptional case it is acceptable.
//
// This routine is also called after a normal SERVICE_CONTROL_STOP order is
// recived by the service. This is done when service main has done the 
// necessary cleanup.
//
// INPUT:  error code - Generated internally or Windows error codes.
// OUTPUT: The error code is passed on to the Service Control Manager.
// OTHER:  -
//==============================================================================

void terminateService(DWORD result)
{
   // Send a message to the scm to tell about stopage. Do not call 
   // sendStatusToSCM if there is no handle to the Service Control Manager.
    
   serviceState = SERVICE_STOPPED;

   if(serviceStatusHandle_ipnaadm) 
   {
      sendStatusToSCM(serviceState,
                      result,
                      0,
                      SERVICE_NOTHING_PENDING,
                      SERVICE_NOTHING_PENDING);
   }

   // Do not need to close serviceStatusHandle_ipnaadm.
    
   exit(result);                                      // Exit from the service.

} 


//==============================================================================
// serviceMain
//
// ServiceMain is called when the Service Control Manager wants to start the 
// service. When it returns, the service has stopped.
//
// It therefor waits for the main thread to be signalled (ended) before 
// returning via the terminateService function. The main thread is signalled 
// both when a normal SERVICE_CONTROL_STOP is executed and an error has occured,
// in the main thread.
//
// INPUT:  -
// OUTPUT: -
// OTHER:  -
//==============================================================================

void serviceMain(DWORD argc, LPTSTR *argv) 
{
   DWORD result;

   // Call registration function to register the service.
    
   serviceStatusHandle_ipnaadm = RegisterServiceCtrlHandler(SERVICE_NAME,
                                  (LPHANDLER_FUNCTION)serviceCtrlHandler);

   if(!serviceStatusHandle_ipnaadm)
   {
      // It was not possible to register the service.
      // Terminate execution, terminateService will not return.
        
      terminateService(GetLastError());
   }

   // Notify SCM of progress
    
   serviceState = SERVICE_START_PENDING;
   sendStatusToSCM(serviceState, NO_ERROR, 0, 1, SERVICE_START_TIME_1);
    
   // Start the service itself, that is OCS_IPN_Main
    
   initService();

   // Create a timer that is used for supervision of the abort order sent to
   // the main thread.
    
   hStopSupervisionTimer = CreateWaitableTimer(SECURITY_ATTRIBUTES, TRUE, NULL);

   // Check if it was possible to create the timer. If not terminate the
   // service.
    
   if(hStopSupervisionTimer == NULL) 
   {
      terminateService(GetLastError());
   }

   // The service is now running. Notify SCM of progress
    
   sendStatusToSCM(serviceState, NO_ERROR, 0, SERVICE_NOTHING_PENDING,
                   SERVICE_NOTHING_PENDING);

   // Wait for the main thread to stop execute or a timeout on the 
   // SERVICE_CONTROL_STOP order. If this happens will the service terminate.
    
   hTerminateArr[WAIT_MAIN_THREAD] = hIpnaAdmMainThread;
   hTerminateArr[WAIT_STOP_SUPERVISION] = hStopSupervisionTimer;

   result = WaitForMultipleObjects(NO_OF_TERMINATION_EVENTS, 
                                   &hTerminateArr[0],
                                   FALSE,
                                   INFINITE);

   switch(result)
   {
      // The main thread has stopped executing. This is a normal case if it
      // was an stop order (SERVICE_CONTROL_STOP) that caused the service to
      // stop, or an error if it was an unexpected stoppage. What situation
      // it is, can be decided by the threads exit code.
        
      case (WAIT_MAIN_THREAD + WAIT_OBJECT_0):
      {
         // Get exit code from the main thread if we have a handle to it
            
         if(hIpnaAdmMainThread)
         {
            if(!GetExitCodeThread(hIpnaAdmMainThread, &result))
            {
               // Terminate if GetExitCodeThread did not succeed
                    
               terminateService(GetLastError());
            }

            // Check why the main thread has stopped
                
            if(result == NO_ERROR)
            {
               // It was an expeced stop of the main thread.
               // Close down everything in a controlled way
                
               CloseHandle(hIpnaAdmMainThread);
               CloseHandle(hStopSupervisionTimer);
               CloseHandle(hMainObjectArray[SERVICE_ABORT_EVENT]);
                                        
               hIpnaAdmMainThread = 0;
               hStopSupervisionTimer = 0;
               hMainObjectArray[SERVICE_ABORT_EVENT] = 0;

               terminateService(NO_ERROR);
            }
            else
            {
               // It was an unexpected stoppage of the main thread,
               // terminte the service.
                    
               terminateService(result);
            }
         }

         // Got no handle to the main thread, return an internal error code.
            
         terminateService(ADM_ERR_NO_HANDLE_EXIST);
      }

      // The stop order has timed out, normal stop of the service failed.
        
      case(WAIT_STOP_SUPERVISION + WAIT_OBJECT_0):
      {
         terminateService(ADM_ERR_TIMEOUT_DURING_STOP);
      }

      case(WAIT_FAILED):
      {
         terminateService(GetLastError());
      }

      default:
      {
         // It was an unexpected stoppage of the main thread, 
         // terminate the service.
            
         terminateService(ADM_ERR_UNEXPECTED_ERROR);
      }
   }
} 


//==============================================================================
// installService
//
// This function installs the service in the Windows NT Service Manager
//
// INPUT:  -
// OUTPUT: -
// OTHER:  -
//==============================================================================

void installService(void)
{
   char      filePath[512];
   char      buffer[512];
   SC_HANDLE SC_Manager;
   SC_HANDLE service;

   // Get the path to the service and add " before and after the string in
   // case the path contains spaces.
    
   GetModuleFileName(NULL, buffer, 512);
   strcpy(filePath,"\"");
   strcat(filePath,buffer);
   strcat(filePath,"\"");

   // Get a handle to the Service Control Manager.
   // Exit if not possible to get a handle.
    
   SC_Manager = OpenSCManager(NULL,
                              NULL,
                              SC_MANAGER_ALL_ACCESS);

   if(SC_Manager == NULL) exit(GetLastError());

    
   // Register our service.
   // Exit if not possible to get a handle.
    
   service = CreateService(SC_Manager,
                           SERVICE_NAME,
                           SERVICE_NAME,
                           SERVICE_ALL_ACCESS,
                           SERVICE_WIN32_OWN_PROCESS,
                           SERVICE_AUTO_START,
                           SERVICE_ERROR_IGNORE,
                           filePath,
                           NULL,
                           NULL,
                           "dummy",
                           NULL,
                           NULL);

   if(service == NULL)
   {
      CloseServiceHandle(SC_Manager);
      exit(GetLastError());
   }

   
   // Return used handles
  
   CloseServiceHandle(SC_Manager);
   CloseServiceHandle(service);

}


//==============================================================================
// removeService
//
// This function removes the service from the Windows NT Service Manager.
//
// INPUT:  -
// OUTPUT: -
// OTHER:  -
//==============================================================================

void removeService(void)
{
   SC_HANDLE      SC_Manager;
   SC_HANDLE      service;
   SERVICE_STATUS serviceStatus;
   DWORD          timeout = 0;

   // Get a handle to the Service Control Manager.
   // Exit if not possible to get a handle.
    
   SC_Manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);

   if(SC_Manager == NULL) 
      exit(GetLastError());

   // Get a handle to our service.
   // Exit if not possible to get a handle.
    
   service = OpenService(SC_Manager,SERVICE_NAME,SERVICE_ALL_ACCESS);

   if(service == NULL)
   {
      CloseServiceHandle(SC_Manager);
      exit(GetLastError());
   }

   // Order the service to stop. This function will return an error code if
   // the service already is stopped. Therefor is the error code not checked.
    
   ControlService(service,SERVICE_CONTROL_STOP,&serviceStatus);
    
   // Wait for the service to stop
   // If the QueryServiceStatus does not succeed remove the service anyway.
    
   while(QueryServiceStatus(service, &serviceStatus))
   {
      // If the service is stopped, break the while loop and remove the 
      // service.
        
      if(serviceStatus.dwCurrentState == SERVICE_STOPPED)
      {
         break;
      }

      // If the service is being stopped, wait until it is stopped or a
      // timeout uccures. If a timout occurs break the while loop and remove 
      // the service anyway.
        
      else if(serviceStatus.dwCurrentState == SERVICE_STOP_PENDING)
      {
         Sleep(500);

         if(++timeout == TIMEOUT_DURING_SERVICE_REMOVE) 
            break;
      }

      // The service is in another state than SERVICE_STOPPED or 
      // SERVICE_STOP_PENDING even though a stop order is sent. Send an extra 
      // stop order, then break the while loop and remove the service anyway.
        
      else
      {
         ControlService(service,SERVICE_CONTROL_STOP,&serviceStatus);
         break;
      }
   }

   // Delete the service and return used handles.
    
   DeleteService(service);

   CloseServiceHandle(SC_Manager);
   CloseServiceHandle(service);

} 


//==============================================================================
// main
//
// Starts the serviceMain function.
//
// INPUT:  Takes one argument, either - install, installs the service with SCM.
//                             or     - remove,  remove the service from SCM.
//                             or     - console, run as consle app (only for debug)
// OUTPUT: -
// OTHER:  -
//==============================================================================

void main(int argc, char *argv[])
{
   SERVICE_TABLE_ENTRY serviceTable[] = 
   { 
      { SERVICE_NAME,(LPSERVICE_MAIN_FUNCTION)serviceMain },
      { NULL, NULL }
   };
    
   // Check if any arguments was given
   
   if(argc == 2)
   {
      // Install the service if the given argument was "install"
        
      if(strcmp(argv[1], "install") == 0)
      {
         installService();
      }
        
      // Remove the service if the given argument was "remove"
        
      else if(strcmp(argv[1], "remove") == 0)
      {
         removeService();
      }

      // Run as console app (only available in debug mode)
        
#ifdef INC_SYSLOG
      else if(strcmp(argv[1], "console") == 0)
      {
         syslog.setOutputFlag(SYSLOG_STDERR);
         OCS_IPN_Main(NULL);
      }
#endif
   }
   else
   {
      // Register the service in the Service Control Manager.
        
      if(!StartServiceCtrlDispatcher(serviceTable))
      {
         // If the service code is executed as an ordinary program, print
         // error information.
            
		  std::cout << std::endl 
					<< "This is a service (not an ordinary program) that must be "
					<< "installed with a special installation program." 
					<< std::endl
					<< "Error code: " << GetLastError() << std::endl;
      }
   }
} 

