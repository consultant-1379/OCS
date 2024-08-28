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
// OCS_IPN_Service.h
//
// DESCRIPTION 
// Header file for OCS_IPN_Service.cpp.
//
// DOCUMENT NO
// 5/190 89-CAA 109 0392
//
// AUTHOR 
// 000803 EPA/D/U Michael Bentley
//
//******************************************************************************
// === Revision history ===
// 000803 MJB Created 
//
//******************************************************************************

#ifndef OCS_IPN_Service
#define OCS_IPN_Service

//******************************************************************************
// OCS_IPN_Service prototypes
//******************************************************************************

//==============================================================================
// initService
//
// Initializes the service by starting the working thread 
//
// INPUT:  -
// OUTPUT: -
// OTHER:  -
//==============================================================================

void initService(void);

//==============================================================================
// installService
//
// This function installs the service in the Windows NT Service Manager
//
// INPUT:  -
// OUTPUT: -
// OTHER:  -
//==============================================================================

void installService(void);

//==============================================================================
// main
//
// Starts the serviceMain function.
//
// INPUT:  Takes one argument, either - install, installs the service with SCM.
//                             or     - remove,  remove the service from SCM.
// OUTPUT: -
// OTHER:  -
//==============================================================================

void main(int argc, char *argv[]);

//==============================================================================
// removeService
//
// This function removes the service from the Windows NT Service Manager.
//
// INPUT:  -
// OUTPUT: -
// OTHER:  -
//==============================================================================

void removeService(void);

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
                                DWORD dwWaitHint);

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

void serviceCtrlHandler(DWORD controlCode);

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

void serviceMain(DWORD argc, LPTSTR *argv);

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

void stopService(void);

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

void terminateService(DWORD error);


//******************************************************************************



#define ADJUST_TIME_NS_MS                       10000

#define SERVICE_NOTHING_PENDING                 0
#define SERVICE_PAUSE_TIME                      1000   //1s
#define SERVICE_RESUME_TIME                     1000   //1s
#define SERVICE_START_TIME_1                    5000   //5s
#define SERVICE_STOP_TIME                       5000   //5s

#define STOP_SUPERVISION_TIME (-(SERVICE_STOP_TIME-1000)*ADJUST_TIME_NS_MS)//4s

#define TIMEOUT_DURING_SERVICE_REMOVE (SERVICE_STOP_TIME*2/500) //10s


//==============================================================================
// Variables used to control the service
//==============================================================================

//******************************************************************************
// The name of the service
//******************************************************************************

char *SERVICE_NAME = "OCS_IPN_ipnaadm";


//******************************************************************************
// Handle used to communicate status info with the Service Control Manager(SCM)
// created by RegisterServiceCtrlHandler in ServiceMain function.
//******************************************************************************

SERVICE_STATUS_HANDLE serviceStatusHandle_ipnaadm;

//******************************************************************************
// Flag holding current state of the service
//
// States currently used: 
// ----------------------
// SERVICE_STOPPED          - The service is not running
// SERVICE_START_PENDING    - The service is starting
// SERVICE_STOP_PENDING     - The service is stopping
// SERVICE_RUNNING          - The service is running
// SERVICE_CONTINUE_PENDING - The service continue is pending
// SERVICE_PAUSE_PENDING    - The service pause is pending
// SERVICE_PAUSED           - The service is paused
//
// For more information see Windows SERVICE_STATUS
//******************************************************************************

BYTE serviceState;

//******************************************************************************
// Handle array used in serviceMain. Used for the events that can cause the
// service to stop.
//******************************************************************************

#define NO_OF_TERMINATION_EVENTS        2

#define WAIT_MAIN_THREAD                0
#define WAIT_STOP_SUPERVISION   1

HANDLE hTerminateArr[NO_OF_TERMINATION_EVENTS];

// Thread for ipnaadmMain
 
HANDLE hIpnaAdmMainThread = 0;

extern HANDLE           hMainObjectArray[];      //Array of Main Obj. handles

//******************************************************************************
// Handle to the timer that is used to supervise the aborting of the main thread 
//******************************************************************************

HANDLE hStopSupervisionTimer;

//******************************************************************************
// Variable used for duration time when waitable timers is set.
//******************************************************************************

LONGLONG durationTime;

//==============================================================================
// External global variables
//==============================================================================

//******************************************************************************
// The beep interval in ms.
//******************************************************************************

extern int beepDelay;

//******************************************************************************
// Event used when the working threads are stopped.
//******************************************************************************

extern HANDLE hAbortEvent;

#endif
