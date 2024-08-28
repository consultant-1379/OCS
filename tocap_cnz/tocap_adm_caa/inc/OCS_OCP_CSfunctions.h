//******************************************************************************
// NAME
// OCS_OCP_CSfunctions.h
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
// 2008-02-14 by EAB/FTE/DDH UABCAJN

// DESCRIPTION
// This file contains methods which interwork with the CS service, e.g. for
// retrieving a list of CP's and AP's in the cluster, together with their
// IP-addresses, node names etc.

// LATEST MODIFICATION
// -
//******************************************************************************
#ifndef _OCS_CSfunctions_h
#define _OCS_CSfunctions_h

#include "OCS_OCP_global.h"
#include <string>
#include <vector>
#include <sstream>
#include <stdint.h>


const int NO_OF_AP=16;
const std::string APNAMES[NO_OF_AP][2]=
{
	{"AP1A","AP1B"},
	{"AP2A","AP2B"},
	{"AP3A","AP3B"},
	{"AP4A","AP4B"},
	{"AP5A","AP5B"},
	{"AP6A","AP6B"},
	{"AP7A","AP7B"},
	{"AP8A","AP8B"},
	{"AP9A","AP9B"},
	{"AP10A","AP10B"},
	{"AP11A","AP11B"},
	{"AP12A","AP12B"},
	{"AP13A","AP13B"},
	{"AP14A","AP14B"},
	{"AP15A","AP15B"},
	{"AP16A","AP16B"}
};

// The AP IP-addresses are fixed according to the tables below.
const std::string APIP[NO_OF_AP][2][2]=
{
	{{"192.168.169.1","192.168.170.1"},{"192.168.169.2","192.168.170.2"}},
	{{"192.168.169.3","192.168.170.3"},{"192.168.169.4","192.168.170.4"}},
	{{"192.168.169.5","192.168.170.5"},{"192.168.169.6","192.168.170.6"}},
	{{"192.168.169.7","192.168.170.7"},{"192.168.169.8","192.168.170.8"}},
	{{"192.168.169.9","192.168.170.9"},{"192.168.169.10","192.168.170.10"}},
	{{"192.168.169.11","192.168.170.11"},{"192.168.169.12","192.168.170.12"}},
	{{"192.168.169.13","192.168.170.13"},{"192.168.169.14","192.168.170.14"}},
	{{"192.168.169.15","192.168.170.15"},{"192.168.169.16","192.168.170.16"}},
	{{"192.168.169.17","192.168.170.17"},{"192.168.169.18","192.168.170.18"}},
	{{"192.168.169.19","192.168.170.19"},{"192.168.169.20","192.168.170.20"}},
	{{"192.168.169.21","192.168.170.21"},{"192.168.169.22","192.168.170.22"}},
	{{"192.168.169.23","192.168.170.23"},{"192.168.169.24","192.168.170.24"}},
	{{"192.168.169.25","192.168.170.25"},{"192.168.169.26","192.168.170.26"}},
	{{"192.168.169.27","192.168.170.27"},{"192.168.169.28","192.168.170.28"}},
	{{"192.168.169.29","192.168.170.29"},{"192.168.169.30","192.168.170.30"}},
	{{"192.168.169.31","192.168.170.31"},{"192.168.169.32","192.168.170.32"}}
};

enum AP_Node_State
{
    ERROR = -1,
    ACTIVE = 1,
    PASSIVE = 2
};

class NodeData
{
public:
    NodeData():name(""),ip(2,0),ipdot(2,""),nodeType(TOCAP::AP),thisnode(false),sysId(0){}
    void operator()(std::ofstream& of,unsigned int& recNr);
    void toString(std::ostringstream& of);
    std::string name;
    std::vector<uint32_t> ip;
	std::vector<std::string> ipdot;
	TOCAP::TypeOfLink nodeType;
	bool thisnode;
	uint16_t sysId;
};

extern bool name_TO_IPaddresses(std::string& nName,std::vector<uint32_t>& ipul_vec,std::vector<std::string>& ipdot_vec);
extern TOCAP::RetCodeValue getCPClusterData(std::vector<NodeData*>& cpnodes);
extern TOCAP::RetCodeValue getAPClusterData(std::vector<NodeData*>& apnodes,std::string thisNodeName);
extern TOCAP::RetCodeValue getOwnAPData(NodeData* ownAP,bool& activeNode);
extern std::string netOrderIP_TO_dotIP(uint32_t ip);
extern bool isMultipleCpSystem(bool& multiSys);
extern bool isFrontEndActiveApNode(std::string& thisNodeName,bool& frontEndSystem);
extern bool getFrontEndNames(std::vector<std::string>& names);
extern bool isOwnNodeName(uint32_t apip,std::string& nodeName);
extern bool getNodeCounters(uint32_t& apg,uint32_t& singleCP,uint32_t& doubleCP);
extern bool isNodeActive(void);
extern void returnCodeValueToString(const TOCAP::RetCodeValue rc, std::ostringstream &trace);
extern uint16_t getServerPort(const char* serverName, const char* protocol, uint16_t defaultPort);
extern AP_Node_State getAPNodeState();

#endif
