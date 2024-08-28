//******************************************************************************
// NAME
// OCS_OCP_alarmMgr.cpp
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
// 2007-12-19 by EAB/FTE/DDH UABCAJN

// DESCRIPTION
// This class decides whether and which alarm that should be issued/ceased
// depending on the node's link states.

// LATEST MODIFICATION
// -
//******************************************************************************
#include "OCS_OCP_alarmMgr.h"
#include "OCS_OCP_link.h"
#include "OCS_OCP_alarmFile.h"
#include "OCS_OCP_events.h"
#include "OCS_OCP_Trace.h"
#include <sstream>
#include <string.h>

using namespace std;



vector<AlarmMgr*> AlarmMgr::alarmVec;
//******************************************************************************
// constructor
//******************************************************************************
AlarmMgr::AlarmMgr(string nName,TOCAP_Events* tev,AlarmMgr* spxptr)
:nodeName(nName),
evp(tev),
fileIndex(UNDEFINED),
spxPartner(spxptr),
attributesUpdated(true)
{

	int err=0;
	// Open the alarm file.
	if ((af=AlarmFile::getInstance(err))==0)
	{
		ostringstream ss;
		ss<<"Opening alarm file failed: "<< strerror(err);
		evp->reportEvent(10010,ss.str());
	}
	// static vector containing all instances.
	alarmVec.push_back(this);

	// cease any outstanding alarm, if existing, for this node.
	ceaseAllAlarm();

}

//******************************************************************************
// destructor
//******************************************************************************
AlarmMgr::~AlarmMgr()
{
    // cease any outstanding alarm, if existing, for this node.
    ceaseAllAlarm();

    if (af) AlarmFile::deleteInstance(af);
    for (vector<AlarmMgr*>::iterator i=alarmVec.begin();i!=alarmVec.end();i++)
    {
        if (*i)
        {
            if (*i==this)
            {
               alarmVec.erase(i);
               break;
            }
        }
    }
}
	
//******************************************************************************
// operator
//******************************************************************************
void AlarmMgr::operator()(ofstream& of,uint32_t& recNr)
{
	if (!attributesUpdated) return;
	attributesUpdated=false;

	of<<"RECORD NUMBER:"<<recNr++<<endl;
	of<<"object ref:"<<this<<endl;
	of<<"node name:"<<nodeName<<endl;
	for (vector<Link*>::iterator i=links.begin();i!=links.end();i++)
	{
		of<<"Link object:"<<*i<<endl;
	}
	of<<"Event object:"<<evp<<endl;
	of<<"Alarm file object:"<<af<<endl;
	if (af && fileIndex!=UNDEFINED) af->operator ()(of,fileIndex);
	of<<"SPX partner object:"<<spxPartner<<endl;
	of << "Number of alarm instances:" << (int)alarmVec.size() << endl;
	of<<endl;
}

//******************************************************************************
// addLink
//******************************************************************************
void AlarmMgr::addLink(Link* lnk)
{
	links.push_back(lnk);
	if (spxPartner && links.size()==2) spxPartner->initspx(this);
}

//******************************************************************************
// linkUpdate

// Calling this method will trigger the object to check whether an alarm should
// be issued or ceased.
//******************************************************************************
bool AlarmMgr::linkUpdate(void)
{
    newTRACE((LOG_LEVEL_INFO, "AlarmMgr::linkUpdate()", 0));

	bool updateRecorded=false;
	AlarmFile::fileRec newAlarmStatus;
	newAlarmStatus.nodeAlarm=TOCAP::CEASED;
	newAlarmStatus.linkAlarms[0]=TOCAP::CEASED;
	newAlarmStatus.linkAlarms[1]=TOCAP::CEASED;
	newAlarmStatus.noOfIPaddresses=0;

	// suppress any issue/cease of alarm if any of the links is not stable.
	uint32_t net=0;
	TOCAP::LinkState linkState[2] = { TOCAP::LINK_OK_ALL_SESSIONS_UP, TOCAP::LINK_OK_ALL_SESSIONS_UP };
	for (vector<Link*>::iterator itLink=links.begin();itLink!=links.end();itLink++,net++)
	{
	    if (*itLink)
		{
			// if any link is unstable then abort further analysis, wait until both link are
			// stable.
			if ((*itLink)->isLinkStable()==false)
			{
			    ostringstream trace;
                trace<<"link not stable on node: "<<nodeName<<" :: net: "<<net;
                TRACE((LOG_LEVEL_INFO,"%s",0,trace.str().c_str()));

                return updateRecorded;
			}
		}
	}

	net = 0;

    for (vector<Link*>::iterator itLink=links.begin();itLink!=links.end();itLink++,net++)
    {
        if ( net > 1 )
        {
            break;
        }

        if (*itLink)
        {
			linkState[net] = (*itLink)->getLinkState();
			if ( TOCAP::LINK_NOT_OK_ALL_SESSIONS_DOWN == linkState[net] )
			{
				newAlarmStatus.linkAlarms[net]=TOCAP::ISSUED;
				// A CP (SPX or BC) link alarm should not be issued if all AP links
				// on that network is faulty.
				if ( (TOCAP::AP != (*itLink)->getLinkType())  && (isAPlinksFaulty(net)==true) )
				{
				    ostringstream trace;
					trace<<"All AP links faulty on net: "<< net <<" ignore CP link alarms on this net";
					TRACE((LOG_LEVEL_INFO, "%s",0,trace.str().c_str()));

				    newAlarmStatus.linkAlarms[net]=TOCAP::CEASED;
				    ceaseAllCPlink(net);
				}
				else
				{
				   ostringstream trace;
				   trace<<"link faulty on node: "<< nodeName <<" :: net: "<< net;
				   TRACE((LOG_LEVEL_INFO, "%s", 0,trace.str().c_str()));
				}
			}
			else if ( TOCAP::LINK_NOT_OK_SESSION_DOWN == linkState[net] )
			{
				newAlarmStatus.linkAlarms[net]=TOCAP::ISSUED;

                ostringstream trace;
                trace<<"link session faulty on node:"<< nodeName <<" :: net:"<< net;
                TRACE((LOG_LEVEL_INFO, "%s",0,trace.str().c_str()));
			}
		}
	}

	if ( ( TOCAP::ISSUED == newAlarmStatus.linkAlarms[0] ) && ( TOCAP::ISSUED == newAlarmStatus.linkAlarms[1] ) &&
      ( TOCAP::LINK_NOT_OK_ALL_SESSIONS_DOWN == linkState[0] ) && ( TOCAP::LINK_NOT_OK_ALL_SESSIONS_DOWN == linkState[1] ) ) 
	{
		// Both links are down with all sessions down in link - change to Node alarm
		newAlarmStatus.linkAlarms[0]=TOCAP::CEASED;
		newAlarmStatus.linkAlarms[1]=TOCAP::CEASED;
		newAlarmStatus.nodeAlarm=TOCAP::ISSUED;
	}
	
	// get the alarm status from the mapped alarm file.
	AlarmFile::fileRec alarmStatus;
	// no alarm ever reported for this node.
	alarmStatus.linkAlarms[0]=TOCAP::CEASED;
	alarmStatus.linkAlarms[1]=TOCAP::CEASED;
	alarmStatus.nodeAlarm=TOCAP::CEASED;
	alarmStatus.noOfIPaddresses=0;

	if (nodeName.length() < NODE_NAME_LENGTH )
	{
		strcpy(alarmStatus.nodeName,nodeName.c_str());
	}
	else
	{
		alarmStatus.nodeName[0] = 0;
	}

	if ((af) && (!af->get(fileIndex,alarmStatus)))
	{
		// no alarm ever reported for this node.
		alarmStatus.linkAlarms[0]=TOCAP::CEASED;
		alarmStatus.linkAlarms[1]=TOCAP::CEASED;
		alarmStatus.nodeAlarm=TOCAP::CEASED;
		alarmStatus.noOfIPaddresses=0;
	}

	vector<string> ipvec_spxPartner; // IP-addresses in dot string format.
	vector<string> ipvec_own;
	getIPdotAddr(ipvec_own);

	vector<Link*>::iterator itLinkX=links.begin();
	if ( (itLinkX!=links.end()) && (*itLinkX) )
	{
		// if this is an spx alarm manager then get the state of the spx partner node.
		if ((*itLinkX)->getLinkType()==TOCAP::SPX)
		{
			bool fourIPissued=false;
			TOCAP::SpxState pSpxState = TOCAP::XS_NOT_DEFINED;
			if (spxPartner)
			{
				try
				{
					  pSpxState = spxPartner->getPartnerInfo(fourIPissued,ipvec_spxPartner);
				}
				catch(...)
				{
				   pSpxState = TOCAP::XS_NOT_DEFINED;
				}
			}
			TOCAP::SpxState ownSpxState=TOCAP::XS_NOT_DEFINED;
			if ((ownSpxState=links[0]->getSpxState())==TOCAP::XS_NOT_DEFINED)
			{
				ownSpxState=links[1]->getSpxState();
			}

			ostringstream trace;
            trace<<endl<<"nodeName:"<<nodeName<<endl;
            trace<<"newAlarmStatus.linkAlarms[0]:"<<(int)newAlarmStatus.linkAlarms[0]<<endl;
            trace<<"newAlarmStatus.linkAlarms[1]:"<<(int)newAlarmStatus.linkAlarms[1]<<endl;
            trace<<"newAlarmStatus.nodeAlarm:"<<(int)newAlarmStatus.nodeAlarm<<endl;
            trace<<"alarmStatus.linkAlarms[0]:"<<(int)alarmStatus.linkAlarms[0]<<endl;
            trace<<"alarmStatus.linkAlarms[1]:"<<(int)alarmStatus.linkAlarms[1]<<endl;
            trace<<"alarmStatus.nodeAlarm:"<<(int)alarmStatus.nodeAlarm<<endl;
            trace<<"ownSpxState:"<<(int)ownSpxState<<endl;
            trace<<"pSpxState:"<<(int)pSpxState<<endl;
            TRACE((LOG_LEVEL_INFO, "%s",0,trace.str().c_str()));

			if (pSpxState==TOCAP::EX || ownSpxState==TOCAP::SB)
			{
				// no alarm should be issued and any outstanding alarm should be ceased.
				newAlarmStatus.linkAlarms[0]=TOCAP::CEASED;
				newAlarmStatus.linkAlarms[1]=TOCAP::CEASED;
				newAlarmStatus.nodeAlarm=TOCAP::CEASED;
				// inform the partner SPX node what's happening.
				if (spxPartner) spxPartner->spxInfo(ownSpxState,ipvec_own);
			}
			else if (ownSpxState==TOCAP::XS_NOT_DEFINED && pSpxState==TOCAP::XS_NOT_DEFINED)
			{
				// if other node already issued the 4 IP-address alarm then don't issue
				// any node alarm.
				if (fourIPissued)
				{
					newAlarmStatus.linkAlarms[0]=TOCAP::CEASED;
					newAlarmStatus.linkAlarms[1]=TOCAP::CEASED;
					newAlarmStatus.nodeAlarm=TOCAP::CEASED;
				}
				else if (spxPartner)
				{

					for (vector<string>::iterator it=ipvec_spxPartner.begin();it!=ipvec_spxPartner.end();++it)
					{
						ipvec_own.push_back(*it);
					}
				}
			}
			else if (ownSpxState==TOCAP::EX)
			{
				// inform the partner SPX node what's happening.
				if (spxPartner) spxPartner->spxInfo(ownSpxState,ipvec_own);
			}
		}
	}

    if (newAlarmStatus.nodeAlarm!=alarmStatus.nodeAlarm &&
                newAlarmStatus.nodeAlarm==TOCAP::CEASED)
    {
        ceaseAlarm(alarmStatus,TOCAP::NODE_ALARM,ipvec_own);
    }

    if (newAlarmStatus.linkAlarms[0]!=alarmStatus.linkAlarms[0] &&
        newAlarmStatus.linkAlarms[0]==TOCAP::CEASED)
    {
        // cease link alarm on Ethernet A
       vector<string> ipvec_own_link;
        ipvec_own_link.push_back(links[0]->getIPdot());
        ceaseAlarm(alarmStatus,TOCAP::LINK_ALARM,ipvec_own_link,0);
    }

    if (newAlarmStatus.linkAlarms[1]!=alarmStatus.linkAlarms[1] &&
        newAlarmStatus.linkAlarms[1]==TOCAP::CEASED)
    {
        // cease link alarm on Ethernet B
        vector<string> ipvec_own_link;
        ipvec_own_link.push_back(links[1]->getIPdot());
        ceaseAlarm(alarmStatus,TOCAP::LINK_ALARM,ipvec_own_link,1);
    }

    if (newAlarmStatus.nodeAlarm!=alarmStatus.nodeAlarm &&
        newAlarmStatus.nodeAlarm==TOCAP::ISSUED)
    {
        issueAlarm(alarmStatus,TOCAP::NODE_ALARM,ipvec_own);
    }

    if (newAlarmStatus.linkAlarms[0]!=alarmStatus.linkAlarms[0] &&
        newAlarmStatus.linkAlarms[0]==TOCAP::ISSUED)
    {
        // issue link alarm on Ethernet A
       vector<string> ipvec_own_link;
        ipvec_own_link.push_back(links[0]->getIPdot());
        issueAlarm(alarmStatus,TOCAP::LINK_ALARM,ipvec_own_link,0);
    }

    if (newAlarmStatus.linkAlarms[1]!=alarmStatus.linkAlarms[1] &&
        newAlarmStatus.linkAlarms[1]==TOCAP::ISSUED)
    {
        // issue link alarm on Ethernet B
        vector<string> ipvec_own_link;
        ipvec_own_link.push_back(links[1]->getIPdot());
        issueAlarm(alarmStatus,TOCAP::LINK_ALARM,ipvec_own_link,1);
    }

    // update the alarm file.
    if ((af) && ((updateRecorded=af->set(fileIndex,alarmStatus))==false))
    {
        evp->reportEvent(10010,"Update alarm file failed");
    }

    return updateRecorded;
}

//******************************************************************************
// PRIVATE CLASS METHODS
//******************************************************************************


//******************************************************************************
// isAPlinksFaulty

// A static method, used to cease all CP link alarms, on the indicated
// network.
//******************************************************************************
bool AlarmMgr::isAPlinksFaulty(uint32_t network)
{
    bool apLinksFaulty=false;
    for (vector<AlarmMgr*>::iterator am=alarmVec.begin();am!=alarmVec.end();++am)
    {
        if (*am)
        {
           if ((*am)->isAP()==true)
           {
               if ((*am)->isAPlinkFaulty(network)) apLinksFaulty=true;
               else
               {
                   apLinksFaulty=false;
                   break;
               }
           }
        }
    }
    return apLinksFaulty;
}

//******************************************************************************
// ceaseAllCPlink

// A static method, used to cease all CP link alarms, on the indicated
// network.
//******************************************************************************
void AlarmMgr::ceaseAllCPlink(uint32_t network)
{
    for (vector<AlarmMgr*>::iterator am=alarmVec.begin();am!=alarmVec.end();++am)
    {
        if (*am)
        {
           if ((*am)->isAP()==false) (*am)->ceaseCPlink(network);
        }
    }
}

//******************************************************************************
// ceaseCPlink

// If issued, cease the link alarm on the indicated network.
//******************************************************************************
void AlarmMgr::ceaseCPlink(uint32_t network)
{

	AlarmFile::fileRec alarmStatus;
	vector<string> ipvec;
	getIPdotAddr(ipvec);

	if (nodeName.length() < NODE_NAME_LENGTH )
	{
		strcpy(alarmStatus.nodeName,nodeName.c_str());
	}
	else
	{
		alarmStatus.nodeName[0] = 0;
	}

	if ((af) && (af->get(fileIndex,alarmStatus)))
	{
		if (alarmStatus.linkAlarms[network]==TOCAP::ISSUED)
		{
			ceaseAlarm(alarmStatus,TOCAP::LINK_ALARM,ipvec,network);
		}
		else if (alarmStatus.nodeAlarm==TOCAP::ISSUED)
		{
			ceaseAlarm(alarmStatus,TOCAP::NODE_ALARM,ipvec);

			// issue the link alarm on the other link.
			int otherLink=0;
			if (network==0) otherLink=1;
			ipvec.push_back(links[otherLink]->getIPdot());
			issueAlarm(alarmStatus,TOCAP::LINK_ALARM,ipvec,otherLink);
		}
		// update the alarm file.
		if (!af->set(fileIndex,alarmStatus))
		{
			evp->reportEvent(10010,"Update alarm file failed");
		}
	}
}

//******************************************************************************
// isAP

// Is this object managing an AP node or not.
//******************************************************************************
bool AlarmMgr::isAP(void)
{
    bool isApNode=false;
    vector<Link*>::iterator itLink=links.begin();
    if ( (itLink!=links.end()) && (*itLink) )
    {
       if ((*itLink)->getLinkType()==TOCAP::AP) isApNode=true;
    }
    return isApNode;
}

//******************************************************************************
// isAPlinkFaulty

// Returns the alarm link status for the indicated network.
//******************************************************************************
bool AlarmMgr::isAPlinkFaulty(uint32_t network)
{

	bool apLinkFaulty=false;

	AlarmFile::fileRec alarmStatus;

	if (nodeName.length() < NODE_NAME_LENGTH )
	{
		strcpy(alarmStatus.nodeName,nodeName.c_str());
	}
	else
	{
		alarmStatus.nodeName[0] = 0;
	}

	if ((af) && (af->get(fileIndex,alarmStatus)))
	{
		if (alarmStatus.linkAlarms[network]==TOCAP::ISSUED || 
			alarmStatus.nodeAlarm==TOCAP::ISSUED)
		{
			apLinkFaulty=true;
		}
	}

	return apLinkFaulty;

}

//******************************************************************************
// getIPdotAddr
//******************************************************************************
void AlarmMgr::getIPdotAddr(vector<string>& ipv)
{
    for (vector<Link*>::iterator it=links.begin();it!=links.end();++it)
    {
        if (*it)
        {
           ipv.push_back((*it)->getIPdot());
        }
    }
}

//******************************************************************************
// initspx

// It's necessary that the two spx nodes knows each other.
//******************************************************************************
void AlarmMgr::initspx(AlarmMgr* amptr)
{
	spxPartner=amptr;
}

//******************************************************************************
// getPartnerInfo

// Retrieves needed data from the other spx node, i.e spx state, IP-addresses
// and whether the other node has already issued the Node alarm containing
// all four IP-addresses.
//******************************************************************************
TOCAP::SpxState AlarmMgr::getPartnerInfo(bool& fourIPalarmIssued,vector<string>& ipaddr)
{
	TOCAP::SpxState spxs=TOCAP::XS_NOT_DEFINED;
	fourIPalarmIssued=false;
	ipaddr.clear();
	getIPdotAddr(ipaddr);

	AlarmFile::fileRec fr;

	if (nodeName.length() < NODE_NAME_LENGTH )
	{
		strcpy(fr.nodeName,nodeName.c_str());
	}
	else
	{
		fr.nodeName[0] = 0;
	}

	if ((af) && (af->get(fileIndex,fr)))
	{
		if (fr.nodeAlarm==TOCAP::ISSUED && fr.noOfIPaddresses==4) 
		{
			fourIPalarmIssued=true;
		}
	}

	if ((spxs=links[0]->getSpxState())==TOCAP::XS_NOT_DEFINED)
	{
		spxs=links[1]->getSpxState();
	}

	return spxs;
}

//******************************************************************************
// spxInfo

// The partner SPX node informs about its state.
// Actions:
// 1: if this node has issued any alarm and the other node is EX then cease
//    the alarm.
// 2: if a link is OK on the other node and this node issued the 4 IP-address
//    node alarm, then cease it.
// 3: replace the 4 IP-address alarm with a 2 IP-address (node alarm) alarm if
//    the other node is in SB state.
//******************************************************************************
void AlarmMgr::spxInfo(TOCAP::SpxState otherSpxState,vector<string> ipvec_spxPartner)
{

	vector<string> ipvec;

	AlarmFile::fileRec alarmStatus;
	if (nodeName.length() < NODE_NAME_LENGTH )
	{
		strcpy(alarmStatus.nodeName,nodeName.c_str());
	}
	else
	{
		alarmStatus.nodeName[0] = 0;
	}

	if ((af) && (af->get(fileIndex,alarmStatus)))
	{
		if (otherSpxState==TOCAP::EX)
		{
			if (alarmStatus.nodeAlarm==TOCAP::ISSUED)
			{		
				getIPdotAddr(ipvec);
				ceaseAlarm(alarmStatus,TOCAP::NODE_ALARM,ipvec);
			}
			else if (alarmStatus.linkAlarms[0]==TOCAP::ISSUED)
			{
				ipvec.push_back(links[0]->getIPdot());
				ceaseAlarm(alarmStatus,TOCAP::LINK_ALARM,ipvec,0);
			}
			else if (alarmStatus.linkAlarms[1]==TOCAP::ISSUED)
			{
				ipvec.push_back(links[1]->getIPdot());
				ceaseAlarm(alarmStatus,TOCAP::LINK_ALARM,ipvec,1);
			}
		}
		else if (otherSpxState==TOCAP::SB)
		{
			// if the 4 IP-address alarm is issued then replace it with the 2 IP-address
			// node alarm.

			if (alarmStatus.nodeAlarm==TOCAP::ISSUED && alarmStatus.noOfIPaddresses==4)
			{
				getIPdotAddr(ipvec);
				for (vector<string>::iterator i=ipvec_spxPartner.begin();
					i!=ipvec_spxPartner.end();i++)
				{
					ipvec.push_back(*i);
				}
				ceaseAlarm(alarmStatus,TOCAP::NODE_ALARM,ipvec);
				
				ipvec.clear();
				getIPdotAddr(ipvec);
				issueAlarm(alarmStatus,TOCAP::NODE_ALARM,ipvec);
			}		
		}
		
		// update the alarm file.
		if (!af->set(fileIndex,alarmStatus))
		{
			evp->reportEvent(10010,"Update alarm file failed");
		}
	}
}

//******************************************************************************
// issueAlarm
//******************************************************************************			
void AlarmMgr::issueAlarm(AlarmFile::fileRec& alStatus,TOCAP::TypeOfAlarm alType,std::vector<std::string>& ipvec,int net)
{

	string nName=nodeName;
	alStatus.noOfIPaddresses = (int)ipvec.size();
	if (alType==TOCAP::NODE_ALARM && alStatus.noOfIPaddresses==4)
	{
		// an SPX node alarm should be issued.
		// discard the side part of the node name.
		nName=nName.erase(nName.size()-1);
	}

	evp->alarmUpdate("A2",alType,nName,ipvec,net);
	if (alType==TOCAP::NODE_ALARM) alStatus.nodeAlarm=TOCAP::ISSUED;
	else alStatus.linkAlarms[net]=TOCAP::ISSUED;
	
	attributesUpdated=true;

}

//******************************************************************************
// ceaseAlarm
//******************************************************************************
void AlarmMgr::ceaseAlarm(AlarmFile::fileRec& alStatus,TOCAP::TypeOfAlarm alType,std::vector<std::string>& ipvec,int net)
{
	string nName=nodeName;
	if (alType==TOCAP::NODE_ALARM && alStatus.noOfIPaddresses==4)
	{
		// an SPX node alarm should be ceased.
		// discard the side part of the node name.
		nName=nName.erase(nName.size()-1);
	}
	evp->alarmUpdate("CEASING",alType,nName,ipvec,net);

	if (alType==TOCAP::NODE_ALARM) alStatus.nodeAlarm=TOCAP::CEASED;
	else alStatus.linkAlarms[net]=TOCAP::CEASED;
	
	attributesUpdated=true;
}


void AlarmMgr::ceaseAllAlarm()
{
	// cease any outstanding alarm, if existing, for this node.
	AlarmFile::fileRec alarmStatus;
	if (nodeName.length() < NODE_NAME_LENGTH )
	{
		strcpy(alarmStatus.nodeName,nodeName.c_str());
	}
	else
	{
		alarmStatus.nodeName[0] = 0;
	}
	vector<string> ipEmpty;
	if ((af) && (af->get(fileIndex,alarmStatus)))
	{
		if (alarmStatus.nodeAlarm==TOCAP::ISSUED)
		{
			ceaseAlarm(alarmStatus,TOCAP::NODE_ALARM,ipEmpty);
		}
		if (alarmStatus.linkAlarms[0]==TOCAP::ISSUED)
		{
			ceaseAlarm(alarmStatus,TOCAP::LINK_ALARM,ipEmpty,0);
		}
		if (alarmStatus.linkAlarms[1]==TOCAP::ISSUED)
		{
			ceaseAlarm(alarmStatus,TOCAP::LINK_ALARM,ipEmpty,1);
		}
		af->set(fileIndex,alarmStatus);
	}
}
