//******************************************************************************
// NAME
// OCS_OCP_CSfunctions.cpp
//
// COPYRIGHT Ericsson AB, Sweden 2007.
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
// 190 89-CAA 109 0748

// AUTHOR 
// 2008-06-13 by EAB/FTE/DDH UABCAJN

// DESCRIPTION
// This file contains methods which interwork with the CS service, e.g. for
// retrieving a list of CP's and AP's in the cluster, together with their
// IP-addresses, node names etc.

// LATEST MODIFICATION
// -

#include "OCS_OCP_CSfunctions.h"
#include "ACS_CS_API.h"
#include "ACS_CS_API_Set.h"
#include "ACS_DSD_Server.h"
#include "ACS_DSD_Session.h"
#include "OCS_OCP_Trace.h"
#include "acs_prc_api.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <arpa/inet.h>
#include <netdb.h>           // For gethostbyname()


using namespace std;

//******************************************************************************
// operator
//******************************************************************************
void NodeData::operator()(ofstream& of, unsigned int& recNr) {
	of << "RECORD NUMBER:" << recNr++ << endl;
	of << "object ref:" << this << endl;
	of << "node name:" << name << endl;
	for (vector<string>::iterator i = ipdot.begin(); i != ipdot.end(); ++i) {
		of << "ip address:" << *i << endl;
	}
	if (nodeType == TOCAP::AP)
		of << "node type:AP" << endl;
	else if (nodeType == TOCAP::CPB)
		of << "node type:CPB" << endl;
	else
		of << "node type:SPX" << endl;
	if (thisnode)
		of << "this node: YES" << endl;
	else
		of << "this node: NO" << endl;
	of << "system ID:" << sysId << endl << endl;
}

//******************************************************************************
// toString
//******************************************************************************
void NodeData::toString(std::ostringstream& of) {
	of << "object ref:" << this << endl;
	of << "node name:" << name << endl;
	for (vector<string>::iterator i = ipdot.begin(); i != ipdot.end(); ++i) {
		of << "ip address:" << *i << endl;
	}
	if (nodeType == TOCAP::AP)
		of << "node type:AP" << endl;
	else if (nodeType == TOCAP::CPB)
		of << "node type:CPB" << endl;
	else
		of << "node type:SPX" << endl;
	if (thisnode)
		of << "this node: YES" << endl;
	else
		of << "this node: NO" << endl;
	of << "system ID:" << sysId << endl << endl;
}

//******************************************************************************
// sysId_TO_nodeName

// Translates an AP system Id, on the indicated side (A/B), into a node name.
//******************************************************************************
bool sysId_TO_nodeName(uint16_t sysid, uint16_t sidenode, string& name)
{
	 bool retCode=false;
	 if (sysid>ACS_CS_API_HWC_NS::SysType_AP &&
	 sysid<=(ACS_CS_API_HWC_NS::SysType_AP+NO_OF_AP))
	 {
	 int index=sysid-ACS_CS_API_HWC_NS::SysType_AP -1;
	 name=APNAMES[index][sidenode];
	 retCode=true;
	 }

	 return retCode;
}

//******************************************************************************
// sysId_TO_netOrderIP

// Translates an AP system Id, on the indicated side (A/B), into IP-addresses
//******************************************************************************
bool sysId_TO_netOrderIP(uint16_t sysid, uint16_t side, vector<
		uint32_t>& ip) {

	 bool retCode=false;
	 ip.clear();
	 if (sysid>ACS_CS_API_HWC_NS::SysType_AP &&
	 sysid<=(ACS_CS_API_HWC_NS::SysType_AP+NO_OF_AP))
	 {
		 int index=sysid-ACS_CS_API_HWC_NS::SysType_AP -1;
		 ip.push_back(inet_addr(APIP[index][side][0].c_str()));
		 ip.push_back(inet_addr(APIP[index][side][1].c_str()));
		 retCode=true;
	 }

	 return retCode;
}

//******************************************************************************
// netOrderIP_TO_dotIP
//******************************************************************************
string netOrderIP_TO_dotIP(uint32_t ip) {
	ostringstream ss;
	ss << (ip & 0x000000FF) << "." << ((ip & 0x0000FF00) >> 8) << "." << ((ip
			& 0x00FF0000) >> 16) << "." << ((ip & 0xFF000000) >> 24);
	string dotIp = ss.str();
	return dotIp;
}

//******************************************************************************
// isValidAPname
//******************************************************************************
bool isValidAPname(const string& apname) {
	bool retCode = false;
	for (uint16_t numap = 0; numap < NO_OF_AP; numap++) {
		for (uint16_t side = 0; side < 2; side++) {
			if (APNAMES[numap][side].compare(apname) == 0) {
				retCode = true;
				break;
			}
		}
		if (retCode)
			break;
	}
	return retCode;
}

//******************************************************************************
// name_TO_IPaddresses

// Translates an AP node name into IP-addresses.
//******************************************************************************
bool name_TO_IPaddresses(string& nName, vector<uint32_t>& ipul_vec, vector<string>& ipdot_vec)
{
    newTRACE((LOG_LEVEL_INFO,"name_TO_IPaddresses()",0));

	bool retCode = false;

	if (isValidAPname(nName))
	{

        for (int apnr=0;apnr<NO_OF_AP;apnr++)
        {
             for (int apside=0;apside<2;apside++)
             {
                 if (APNAMES[apnr][apside].compare(nName)==0)
                 {
                     ipdot_vec.push_back(APIP[apnr][apside][0]);
                     ipdot_vec.push_back(APIP[apnr][apside][1]);
                     ipul_vec.push_back(inet_addr(APIP[apnr][apside][0].c_str()));
                     ipul_vec.push_back(inet_addr(APIP[apnr][apside][1].c_str()));
                     retCode=true;
                 }
             }
        }
	}
	else
	{
        ostringstream trace;
        trace << "The AP name is not valid: " << nName << endl;
        TRACE((LOG_LEVEL_INFO,"%s",0,trace.str().c_str()));
	}

	return retCode;
}

//******************************************************************************
// getCPClusterData

// Gets all configuration data for CP blades and SPX's from the CS database.
//******************************************************************************
TOCAP::RetCodeValue getCPClusterData(vector<NodeData*>& cp_nodes) {

	TOCAP::RetCodeValue retCode = TOCAP::RET_OK;

	 bool AnyBoard=false;
	 bool ReadingSuccesful=false;
	 ACS_CS_API_HWC* hwc=ACS_CS_API::createHWCInstance();
	 ACS_CS_API_CP* cpObj=ACS_CS_API::createCPInstance();
	 ACS_CS_API_BoardSearch* bs=ACS_CS_API_HWC::createBoardSearchInstance();

	 if (hwc && cpObj && bs)
	 {
		 ACS_CS_API_IdList cpList;
		 ACS_CS_API_IdList boardList;

		 if (cpObj->getCPList(cpList)==ACS_CS_API_NS::Result_Success)
		 {
			 // a list of CP system ID's retrieved from CS.
			 ACS_CS_API_Name cpname;
			 const size_t CPNAMELENGTH = 16;
			 char cpNameChar[CPNAMELENGTH];
			 NodeData* ntmp = NULL;
			 NodeData* ntmpB = NULL;

			 for (uint16_t i=0;i<cpList.size();i++,ntmp = NULL,ntmpB = NULL)
			 {
				 ReadingSuccesful=false;
				 if (ACS_CS_API_NetworkElement::getDefaultCPName(cpList[i],cpname)==
				 ACS_CS_API_NS::Result_Success)
				 {
					 // the default node name for the system ID retrieved.
					 size_t ln = CPNAMELENGTH;
					 cpname.getName(cpNameChar,ln);
					 ntmp = new NodeData;
					 if (ntmp)
					 {
						 // the node name will later on be extended with A or B for
						 // double sided CP's.
						 ntmp->name = cpNameChar;
						 //ntmpB->name = std::string((char*)cpNameChar);
						 ntmp->sysId = cpList[i];
						 if (cpList[i]>=ACS_CS_API_HWC_NS::SysType_CP &&
						 cpList[i]< ACS_CS_API_HWC_NS::SysType_AP)
						 {
							 ntmp->nodeType = TOCAP::SPX;
							 // get the A-side IP-addresses.
							 bs->reset();
							 bs->setSysId(ntmp->sysId);
							 bs->setSide(ACS_CS_API_HWC_NS::Side_A);
							 bs->setFBN(ACS_CS_API_HWC_NS::FBN_CPUB);
							 if (hwc->getBoardIds(boardList,bs)==ACS_CS_API_NS::Result_Success && boardList.size()==1)
							 {
								 ReadingSuccesful=true;
								 // A node
								 if (hwc->getIPEthA(ntmp->ip[0],boardList[0])== ACS_CS_API_NS::Result_Success)
								 {
									 ntmp->ip[0]=htonl(ntmp->ip[0]);
									 ntmp->ipdot[0]=netOrderIP_TO_dotIP(ntmp->ip[0]);
									 // B node
									 if (hwc->getIPEthB(ntmp->ip[1],boardList[0]) == ACS_CS_API_NS::Result_Success)
									 {
										 ntmp->ip[1]=htonl(ntmp->ip[1]);
										 ntmp->ipdot[1]=netOrderIP_TO_dotIP(ntmp->ip[1]);
									 }
									 else
									 {	// failed to get Eth-B IP-address for SPX-A.
										 ReadingSuccesful=false;
										 retCode=TOCAP::GETIPETHB_FAILED;
									 }
								 }
								 else
								 {	// failed to get Eth-A IP-address for SPX-A.
									 ReadingSuccesful=false;
									 retCode=TOCAP::GETIPETHA_FAILED;
								 }

								 if (ReadingSuccesful)
								 {
									 ntmp->name+="A";
									 cp_nodes.push_back(ntmp);
									 ntmp = NULL;
									 AnyBoard=true;
								 }

								 // allocate a new object for the B-node
								 ntmpB = new NodeData;

							 }else
							 {	// failed to get SPX-A board for the indicated sys ID.
								 retCode=TOCAP::GETBOARDIDS_FAILED;
							 }

							 if (ntmpB)
							 {
								 ntmpB->nodeType = TOCAP::SPX;
								 ntmpB->name = std::string((char*)cpNameChar);
								 ntmpB->sysId = cpList[i];
								 bs->reset();
								 bs->setSysId(ntmpB->sysId);
								 bs->setSide(ACS_CS_API_HWC_NS::Side_B);
								 bs->setFBN(ACS_CS_API_HWC_NS::FBN_CPUB);
								 if (hwc->getBoardIds(boardList,bs)==ACS_CS_API_NS::Result_Success &&
								 boardList.size()==1)
								 {
									 ReadingSuccesful=true;
									 // A node
									 if (hwc->getIPEthA(ntmpB->ip[0],boardList[0]) == ACS_CS_API_NS::Result_Success)
									 {
										 ntmpB->ip[0]=htonl(ntmpB->ip[0]);
										 ntmpB->ipdot[0]=netOrderIP_TO_dotIP(ntmpB->ip[0]);
										 // B node
										 if (hwc->getIPEthB(ntmpB->ip[1],boardList[0])
										 ==ACS_CS_API_NS::Result_Success)
										 {
										 ntmpB->ip[1]=htonl(ntmpB->ip[1]);
										 ntmpB->ipdot[1]=netOrderIP_TO_dotIP(ntmpB->ip[1]);
										 }
										 else
										 {	// failed to get Eth-B IP-address for SPX-B.
										 ReadingSuccesful=false;
										 retCode=TOCAP::GETIPETHB_FAILED;
										 }
									 }
									 else
									 {	// failed to get Eth-A IP-address for SPX-B.
										 ReadingSuccesful=false;
										 retCode=TOCAP::GETIPETHA_FAILED;
									 }

									 if (ReadingSuccesful)
									 {
									 ntmpB->name+="B";
									 cp_nodes.push_back(ntmpB);
									 ntmpB = NULL;
									 AnyBoard=true;
									 }
								 }
								 else
								 {	// failed to get SPX-B board for the indicated sys ID.
									 retCode=TOCAP::GETBOARDIDS_FAILED;
								 }
							 }
						 }
						 else if (cpList[i]<ACS_CS_API_HWC_NS::SysType_CP)
						 {
							 ntmp->nodeType=TOCAP::CPB;
							 bs->reset();
							 bs->setSysId(ntmp->sysId);
							 bs->setFBN(ACS_CS_API_HWC_NS::FBN_CPUB);
							 if (hwc->getBoardIds(boardList,bs)==ACS_CS_API_NS::Result_Success &&
							 boardList.size()==1)
							 {
								 ReadingSuccesful=true;
								 // A node
								 if (hwc->getIPEthA(ntmp->ip[0],boardList[0])
								 ==ACS_CS_API_NS::Result_Success)
								 {
									 ntmp->ip[0]=htonl(ntmp->ip[0]);
									 ntmp->ipdot[0]=netOrderIP_TO_dotIP(ntmp->ip[0]);
									 // B node
									 if (hwc->getIPEthB(ntmp->ip[1],boardList[0])
									 ==ACS_CS_API_NS::Result_Success)
									 {
										 ntmp->ip[1]=htonl(ntmp->ip[1]);
										 ntmp->ipdot[1]=netOrderIP_TO_dotIP(ntmp->ip[1]);
									 }
									 else
									 {	// failed to get Eth-B IP-address for CPB.
										 ReadingSuccesful=false;
										 retCode=TOCAP::GETIPETHB_FAILED;
									 }
								 }
								 else
								 {	// failed to get Eth-A IP-address for CPB.
									 ReadingSuccesful=false;
									 retCode=TOCAP::GETIPETHA_FAILED;
								 }

								 if (ReadingSuccesful)
								 {
									 cp_nodes.push_back(ntmp);
									 ntmp = NULL;
									 AnyBoard=true;
								 }
							 }
							 else
							 {	// failed to get CPB board for the indicated sys ID.
								 retCode=TOCAP::GETBOARDIDS_FAILED;
							 }
						 }
					 }
					 else
					 {	// failed to create a NodeData object.
						 retCode=TOCAP::ALLOCATION_FAILED;
					 }
				 }
				 else
				 {	// failed to get the default CP name.
					 retCode=TOCAP::GETDEFAULTCPNAME_FAILED;
				 }
				 if ( ntmp )
				 {
					 delete ntmp;
					 ntmp = NULL;
				 }
				 if ( ntmpB )
				 {
					 delete ntmpB;
					 ntmpB = NULL;
				 }
			} // for
		 }
		 else
		 {	// failed to get a CP list of system ID's.
			 retCode=TOCAP::GETCPLIST_FAILED;
		 }
	 }
	 else
	 {	// failed to create CS instances, memory problem.
		 retCode=TOCAP::ALLOCATION_FAILED;
	 }

	 // Only one board OK is ok
	 if (AnyBoard) retCode=TOCAP::RET_OK;

	 if (retCode!=TOCAP::RET_OK)
	 {	// deallocate all created objects.
		 for (vector<NodeData*>::iterator itr = cp_nodes.begin(); itr != cp_nodes.end(); ++itr)
		 {
			 if (*itr)
			 {
			 delete *itr;
			 *itr = NULL;
			 }
		 }
	 }
	 if (cpObj) ACS_CS_API::deleteCPInstance(cpObj);
	 if (bs) ACS_CS_API_HWC::deleteBoardSearchInstance(bs);
	 if (hwc) ACS_CS_API::deleteHWCInstance(hwc);

	 return retCode;


}

//******************************************************************************
// getAPClusterData

// Gets all AP configuration data, stored in the internal hardcoded tables. 
// The CS only contains which AP's that exist in the cluster, represented by
// the system ID.
//******************************************************************************
TOCAP::RetCodeValue getAPClusterData(vector<NodeData*>& ap_nodes, string thisNodeName) {

	TOCAP::RetCodeValue retCode = TOCAP::RET_OK;

	ACS_CS_API_HWC* hwc = ACS_CS_API::createHWCInstance();
	ACS_CS_API_BoardSearch* bs = ACS_CS_API_HWC::createBoardSearchInstance();
	NodeData* ntmp = NULL;
	if (hwc && bs) {
		bs->reset();
		bs->setFBN(ACS_CS_API_HWC_NS::FBN_APUB);
		ACS_CS_API_IdList boardList;

		// search for all AP boards in the cluster.
		if (hwc->getBoardIds(boardList, bs) == ACS_CS_API_NS::Result_Success) {
			APID sysId;
			vector<int> nodeInd(NO_OF_AP, 0);
			int indexAP = 0;
			for (unsigned int i = 0; i < boardList.size(); i++) {
				if (hwc->getSysId(sysId, boardList[i])
						== ACS_CS_API_NS::Result_Success) {
					indexAP = sysId - ACS_CS_API_HWC_NS::SysType_AP - 1;
					if((indexAP >= 0) && (indexAP < NO_OF_AP)){
                        ntmp = new NodeData;
                        if (ntmp) {
                            vector<uint32_t> vip;
                            if (sysId_TO_netOrderIP(sysId, nodeInd[indexAP], vip)
                                    && sysId_TO_nodeName(sysId, nodeInd[indexAP],
                                            ntmp->name)) {
                                ntmp->sysId = sysId;
                                ntmp->nodeType = TOCAP::AP;
                                if (thisNodeName == ntmp->name)
                                    ntmp->thisnode = true;
                                ntmp->ip[0] = vip[0];
                                ntmp->ipdot[0] = netOrderIP_TO_dotIP(vip[0]);
                                ntmp->ip[1] = vip[1];
                                ntmp->ipdot[1] = netOrderIP_TO_dotIP(vip[1]);
                                ap_nodes.push_back(ntmp);
                                ntmp = NULL;
                                nodeInd[indexAP] = 1;
                            } else {
                                delete ntmp;
                                ntmp = NULL;
                                // sysId_TO_netOrderIP failed
                                retCode = TOCAP::CONVERSION_FAILED;
                                break;
                            }
                        } else {
                            // failed to create a NodeData object.
                            retCode = TOCAP::ALLOCATION_FAILED;
                            break;
                        }
                    } else {
                        // failed to get system ID for board.
                        retCode = TOCAP::GETSYSID_FAILED;
                        break;
                    }
				}
			} // for
		} else {
			// failed to get AP board list.
			retCode = (TOCAP::RetCodeValue) boardList.size(); //TOCAP::GETBOARDIDS_FAILED;
		}
	} else {
		// failed to create CS instances, memory problem.
		retCode = TOCAP::ALLOCATION_FAILED;
	}

	if (retCode != TOCAP::RET_OK) {
		// deallocate all created objects.
		for (vector<NodeData*>::iterator itr = ap_nodes.begin(); itr
				!= ap_nodes.end(); ++itr) {
			if (*itr) {
				delete *itr;
				*itr = NULL;
			}
		}
	}

	if (bs)
		ACS_CS_API_HWC::deleteBoardSearchInstance(bs);
	if (hwc)
		ACS_CS_API::deleteHWCInstance(hwc);

	return retCode;
}

//******************************************************************************
// getOwnAPdata
//******************************************************************************
TOCAP::RetCodeValue getOwnAPData(NodeData* ownAP, bool& activeNode)
{

	TOCAP::RetCodeValue retCode = TOCAP::CONVERSION_FAILED;
	activeNode = false;
	ACS_DSD_Server s(acs_dsd::SERVICE_MODE_INET_SOCKET_PRIVATE);
	ACS_DSD_Node my_node;
	s.get_local_node(my_node);

	ownAP->name = my_node.node_name;
	ownAP->nodeType = TOCAP::AP;
	ownAP->sysId = (unsigned short) my_node.system_id;
	ownAP->thisnode = true;
	// convert to capital letters.
	for (unsigned int i = 0; i < ownAP->name.size(); i++)
	{
		ownAP->name[i] = char(toupper(ownAP->name[i]));
	}

	vector<uint32_t> ipul;
	vector<string> ipdot;
	if (name_TO_IPaddresses(ownAP->name, ipul, ipdot))
	{
		retCode = TOCAP::RET_OK;
		ownAP->ip[0] = ipul[0];
		ownAP->ip[1] = ipul[1];
		ownAP->ipdot[0] = ipdot[0];
		ownAP->ipdot[1] = ipdot[1];
	}
	if (isNodeActive())
	{
		activeNode = true;
	}

	return retCode;


}

//******************************************************************************
// isMultipleCpSystem
//******************************************************************************
bool isMultipleCpSystem(bool& multiSys)
{
    newTRACE((LOG_LEVEL_INFO,"isMultipleCpSystem()",0));

	bool retCode = false;
	int ret = ACS_CS_API_NetworkElement::isMultipleCPSystem(multiSys);
	if ( ret == ACS_CS_API_NS::Result_Success)
	{
		retCode = true;
	}
	else
	{
        ostringstream trace;
        trace << "ACS_CS_API_NetworkElement::isMultipleCPSystem() failed with return code: "<< ret << endl;
        TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));
	}

	return retCode;

}

//******************************************************************************
// isFrontEndActiveApNode
//******************************************************************************
bool isFrontEndActiveApNode(string& thisNodeName, bool& frontEndSystem)
{
    newTRACE((LOG_LEVEL_INFO,"isFrontEndActiveApNode()",0));

    bool retCode=false;

    bool activeAPnode=false;
    frontEndSystem=false;

    NodeData thisAP;
    if (getOwnAPData(&thisAP,activeAPnode)==TOCAP::RET_OK)
    {
    APID frontAP;
    int ret = ACS_CS_API_NetworkElement::getFrontAPG(frontAP);
    if ( ret == ACS_CS_API_NS::Result_Success)
    {
       if (frontAP==thisAP.sysId)
       {
            frontEndSystem=true;

            ostringstream trace;
            trace << "This is front end APG" << endl;
            TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));
       }
    }
    else
    {
        ostringstream trace;
        trace << "ACS_CS_API_NetworkElement::getFrontAPG() failed with return code: "<< ret << endl;
        TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));
    }

    if (activeAPnode && frontEndSystem)
    {
      thisNodeName=thisAP.name;
      retCode=true;
    }

    }
    return retCode;

}

//******************************************************************************
// getFrontEndNames
//******************************************************************************
bool getFrontEndNames(vector<string>& names)
{
    newTRACE((LOG_LEVEL_INFO,"getFrontEndNames()",0));

    bool retCode=false;
    names.clear();
    APID frontAP=0;
    int ret = ACS_CS_API_NetworkElement::getFrontAPG(frontAP);
    if ( ret == ACS_CS_API_NS::Result_Success)
    {
     string feNodeName="";
     if (sysId_TO_nodeName(frontAP,0,feNodeName))
     {
         names.push_back(feNodeName);
     }
     if (sysId_TO_nodeName(frontAP,1,feNodeName))
     {
         names.push_back(feNodeName);
     }
     if (names.size()) retCode=true;
    }
    else
    {
        ostringstream trace;
        trace << "ACS_CS_API_NetworkElement::getFrontAPG() failed with return code: "<< ret << endl;
        TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));
    }

    return retCode;
}

//******************************************************************************
// isOwnNodeName
//******************************************************************************
bool isOwnNodeName(uint32_t apip, string& nodeName)
{

	 bool retCode=false;
	 for (int numap=0;numap<NO_OF_AP;numap++)
	 {
		 for (int side=0;side<2;side++)
		 {
			 if (APNAMES[numap][side].compare(nodeName)==0)
			 {
					if (inet_addr(APIP[numap][side][0].c_str())==apip || inet_addr(APIP[numap][side][1].c_str())==apip)
					{
						retCode=true;
						break;
					}
			 }
		 }

		 if (retCode) break;

	 }
	 return retCode;

}

//******************************************************************************
// getNodeCounters
//******************************************************************************
bool getNodeCounters(uint32_t& apg, uint32_t& singleCP, uint32_t& doubleCP) {

	bool retCode = true;
	if (ACS_CS_API_NetworkElement::getAPGCount(apg)
			!= ACS_CS_API_NS::Result_Success) {
		retCode = false;
	} else if (ACS_CS_API_NetworkElement::getSingleSidedCPCount(singleCP)
			!= ACS_CS_API_NS::Result_Success) {
		retCode = false;
	} else if (ACS_CS_API_NetworkElement::getDoubleSidedCPCount(doubleCP)
			!= ACS_CS_API_NS::Result_Success) {
		retCode = false;
	}

	return retCode;

}

//******************************************************************************
// isNodeActive
//******************************************************************************
bool isNodeActive(void)
{
    newTRACE((LOG_LEVEL_INFO,"isNodeActive()",0));

    ACS_PRC_API prcApi;
	int nodeState = prcApi.askForNodeState();
	bool isActive = false;
	// nodeState = -1 Error detected, 1 Active, 2 Passive.

    ostringstream trace;
    trace << "This AP Node state (-1 Error detected, 1 Active, 2 Passive): "<< nodeState << endl;
    TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

	if (nodeState == 1) // Active
	{
		isActive = true;
	}

	if (nodeState == 2) // Passive
	{
		isActive = false;
	}

	if (nodeState == -1) // Error detected
	{
		isActive = false;
	}

    return isActive;

}

void returnCodeValueToString(const TOCAP::RetCodeValue rc,
		std::ostringstream &trace) {
	switch (rc) {
	case TOCAP::RET_OK:
		trace << "RET_OK";
		break;
	case TOCAP::ANOTHER_TRY:
		trace << "ANOTHER_TRY";
		break;
	case TOCAP::GETIPETHA_FAILED:
		trace << "GETIPETHA_FAILED";
		break;
	case TOCAP::GETIPETHB_FAILED:
		trace << "GETIPETHB_FAILED";
		break;
	case TOCAP::GETBOARDIDS_FAILED:
		trace << "GETBOARDIDS_FAILED";
		break;
	case TOCAP::ALLOCATION_FAILED:
		trace << "ALLOCATION_FAILED";
		break;
	case TOCAP::GETDEFAULTCPNAME_FAILED:
		trace << "GETDEFAULTCPNAME_FAILED";
		break;
	case TOCAP::GETCPLIST_FAILED:
		trace << "GETCPLIST_FAILED";
		break;
	case TOCAP::INCOMPLETE_CLUSTER:
		trace << "INCOMPLETE_CLUSTER";
		break;
	case TOCAP::OPEN14007_FAILED:
		trace << "OPEN14007_FAILED";
		break;
	case TOCAP::OPEN14008_FAILED:
		trace << "OPEN14008_FAILED";
		break;
	case TOCAP::OPEN14009_FAILED:
		trace << "OPEN14009_FAILED";
		break;
	case TOCAP::AP_GETHANDLES_FAILED:
		trace << "AP_GETHANDLES_FAILED";
		break;
	case TOCAP::CP_GETHANDLES_FAILED:
		trace << "CP_GETHANDLES_FAILED";
		break;
	case TOCAP::WAITFORMULTIPLE_FAILED:
		trace << "WAITFORMULTIPLE_FAILED";
		break;
	case TOCAP::GETSYSID_FAILED:
		trace << "GETSYSID_FAILED";
		break;
	case TOCAP::CONVERSION_FAILED:
		trace << "CONVERSION_FAILED";
		break;
	case TOCAP::INTERNAL_RESTART:
		trace << "INTERNAL_RESTART";
		break;
	case TOCAP::TIMEOUT_READ:
		trace << "TIMEOUT_READ";
		break;
	default:
		trace << "__UNKNOWN__";
		break;
	}
}

uint16_t getServerPort(const char* serverName, const char* protocol, uint16_t defaultPort)
{
    struct servent   *servp;    // Pointer to service entry
    try
    {
        if ((servp=getservbyname(serverName,protocol)) == NULL)
        {
            return defaultPort;
        }
        else
        {
            return ntohs(servp->s_port);
        }
    }
    catch (...)
    {
        return defaultPort;
    }
}

AP_Node_State getAPNodeState()
{
    newTRACE((LOG_LEVEL_INFO,"getAPNodeState()",0));

    AP_Node_State state = ERROR;

    ACS_PRC_API prcApi;
    int nodeState = prcApi.askForNodeState();

    // nodeState = -1 Error detected, 1 Active, 2 Passive.

    TRACE((LOG_LEVEL_INFO,"This AP Node state: %d",0,nodeState));

    if (nodeState == 1) // Active
    {
        state = ACTIVE;
    }

    if (nodeState == 2) // Passive
    {
        state = PASSIVE;
    }

    if (nodeState == -1) // Error detected
    {
        state = ERROR;
    }

    return state;
}
