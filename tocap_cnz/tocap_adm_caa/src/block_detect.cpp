//******************************************************************************
// NAME
// block_detect.cpp
//
// COPYRIGHT Ericsson AB, Sweden 2010.
// All rights reserved.
//
// The Copyright to the computer program(s) herein
// is the property of Ericsson AB, Sweden.
// The program(s) may be used and/or copied only with
// the written permission from Ericsson AB or in
// accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been
// supplied.

// DOCUMENT NO
// ???

// AUTHOR
// 2010-12-01 by XDT/DEK XTUANGU

// DESCRIPTION
// This class is used for block detect and release lock for OCS_OCP_frontEndAP and OCS_OCP_nonFrontEndAP.

// LATEST MODIFICATION
// -
//******************************************************************************

#include "block_detect.h"

Event Block_Detect::stop_thread_handle;
int Block_Detect::aliveCnt=0;
ProtHandler_Ptr Block_Detect::obj;
pthread_t Block_Detect::thread_handle = 0;

void Block_Detect::create_block_detect(void)
{
	int rc;
	stop_thread_handle.resetEvent();

	rc = ::pthread_create(&thread_handle, NULL, hang_detection, NULL);
	if (rc)
	{
		 exit(-1);
	}

}
void Block_Detect::delete_block_detect(void)
{
	stop_thread_handle.setEvent();

	if ( thread_handle )
      {
		::pthread_exit(0);
		thread_handle = 0;
      }

}

void Block_Detect::alive(void)
{
	if (thread_handle == 0)
		create_block_detect();

	aliveCnt++;
}

void Block_Detect::add_object(ProtHandler_Ptr& p)  // HY34582
{
	obj = p;
}

void Block_Detect::reset_object(void)
{
	obj.reset();
}

void* Block_Detect::hang_detection(void*)
{
	int snapshot_aliveCnt = aliveCnt;
	int threadHangCnt = 0;

    fd_set readset, allset;

    int maxfd = stop_thread_handle.getFd();
    FD_ZERO(&allset);

            // Add stop event to allset
    FD_SET(stop_thread_handle.getFd(), &allset);

    timeval selectTimeout;

    while (true)
    {
        readset = allset;
        selectTimeout.tv_sec = 1;
        selectTimeout.tv_usec = 0;
        readset = allset;

        int select_return = ::select(maxfd+1,&readset, NULL, NULL, &selectTimeout);

        if(select_return == 0) // Timeout.
        {
        	try
			{
				if (obj != NULL)
				{
					if (snapshot_aliveCnt == aliveCnt)
					{
						threadHangCnt++;
					}
					else
					{
						threadHangCnt=0;
						snapshot_aliveCnt = aliveCnt;
					}

					if (threadHangCnt>=5)
					{
						if (obj != NULL)
						{
							obj->closeDSD(true);
						}

						obj.reset();
						threadHangCnt = 0;
					}
				}
				else
				{
					threadHangCnt = 0;
				}
			}
			catch(...)
			{
				obj.reset();
				threadHangCnt = 0;
			}
        }
        else if(select_return == -1) //Error.
        {
        	break;
        }
        else
        {
			//Check for stop event
			if(FD_ISSET(stop_thread_handle.getFd(),&readset))
			{
				stop_thread_handle.resetEvent();
				//hang = false;
				break;
			}
        }

    }

    ::pthread_exit(0);

    return 0;
  }


