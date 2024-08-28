//******************************************************************************
// NAME
// OCS_OCP_server.h
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
// This class is use for presenting the OCP server.

// LATEST MODIFICATION
// -
//******************************************************************************

#ifndef OCS_OCP_Server_h
#define OCS_OCP_Server_h

#include "OCS_OCP_frontEndAP.h"
#include "OCS_OCP_nonFrontEndAP.h"

class OCS_OCP_Service;

class OCS_OCP_Server {
public:

	/*=====================================================================
		                        CLASS CONSTRUCTORS
	==================================================================== */
	/*=================================================================== */
		  /**

		      @brief          Constructs and initializes internal variables

		      @par			  None

		      @pre            None

		      @post           None

			  @param          None


		      @return         void

		      @exception      none
		  */
	/*=================================================================== */
	OCS_OCP_Server();

	/*=====================================================================
	                                CLASS CONSTRUCTORS
    ==================================================================== */
    /*=================================================================== */
          /**

              @brief          Constructs and initializes internal variables

              @par            pOcsOCPService - AMF service

              @pre            None

              @post           None

              @param          None


              @return         void

              @exception      none
          */
    /*=================================================================== */
    OCS_OCP_Server(OCS_OCP_Service* pOcsOCPService);


	/*===================================================================
		                        CLASS DESTRUCTOR
	=================================================================== */
	/*=================================================================== */
	   /**

		      @brief           Destroys the the volume object

		      @par             None

		      @pre             None

		      @post            None

		      @return          None

		      @exception       None
	   */
	/*=================================================================== */
	virtual ~OCS_OCP_Server();

	/*===================================================================
							PUBLIC METHOD
	=================================================================== */

	/*=================================================================== */
		   /**

		      @brief           Start echo server that listens on DSD sockets.

		      @par             None

		      @pre             None

		      @post            None

		      @return          Error or success codes.

		      @exception       None
		   */
	/*=================================================================== */
	int run();

	/*===================================================================
	                            PUBLIC METHOD
	=================================================================== */

	/*=================================================================== */
             /**

              @brief           Handle for stop event from main service.

              @par             None

              @pre             None

              @post            None

              @return          Error or success codes.

              @exception       None
             */

	/*=================================================================== */
	void stop();

	/*===================================================================
	                            PUBLIC METHOD
	=================================================================== */

	/*=================================================================== */
             /**

              @brief           Set running mode (service or no service).

              @par             mode (1: AMF service, 2: noservice)

              @pre             None

              @post            None

              @return          Error or success codes.

              @exception       None
             */

	/*=================================================================== */
	void setRunningMode(int mode);


private:
	OCS_OCP_frontEndAP* m_frontEndAP;
	OCS_OCP_nonFrontEndAP* m_nonFrontEndAP;
	bool m_serviceStop;
	int m_runningMode; // 1: AMF service, 2: no service

public:
	// TOCAP ports
	// Socket port used by CP to communicate with frontend AP
    static uint16_t s_tocapCPPort;
    // Socket port used by non-frontend AP to communicate with frontend AP
    static uint16_t s_tocapFEPort;
    // Socket port used by frontend AP to communicate with non-frontend AP
    static uint16_t s_tocapNFEPort;

    // Pointer to amf service object
    OCS_OCP_Service* m_pOcsOCPService;
};

#endif //  OCS_OCP_Server_h

//----------------------------------------------------------------------------
//
//  COPYRIGHT Ericsson AB 2001-2004
//
//  The copyright to the computer program(s) herein is the property of
//  ERICSSON AB, Sweden. The programs may be used and/or copied only
//  with the written permission from ERICSSON AB or in accordance with
//  the terms and conditions stipulated in the agreement/contract under
//  which the program(s) have been supplied.
//
//----------------------------------------------------------------------------


