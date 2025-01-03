////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020 NovAtel Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

// Includes

#include <lib/nlohmann/json.hpp>

#include "hw_interface/stream_interface/api/inputstreaminterface.hpp"
#include "hw_interface/stream_interface/api/inputfilestream.hpp"
#include "decoders/novatel/api/framer.hpp"
#include "decoders/novatel/api/decoder.hpp"
#include "decoders/jsoninterface/api/logutils.hpp"
#include <gtest/gtest.h>

#include <iostream>
#include <fstream>

using json = nlohmann::json;


#ifndef DATADIR
    #define DATADIR
#endif

class DecoderTest : public ::testing::Test {
public:
  virtual void SetUp() {
	  std::string filename_json = std::string(DATADIR) + "dynamicdef.json";
	  const char*	filepath_json = filename_json.c_str();
	  loadDataFromJSON::initializeDatabaseObj();
	  JSONFileReader* jsonReader = NULL;
	  jsonReader = loadDataFromJSON::getDatabaseObj(filepath_json);
  }

  virtual void TearDown() {
  }

private:
};

TEST_F(DecoderTest, BESTPOS_JSON) {

   std::string filename = std::string(DATADIR) + "advancedecoder_bestpos.bin";

   const char* filepath = filename.c_str();

   InputStreamInterface* ifs = NULL;

   ifs = new InputFileStream(filepath);

   Decoder stDDecoder(ifs);
   stDDecoder.EnableUnknownData(FALSE);
   stDDecoder.SetBMDOutput(JSON);
   StreamReadStatus stStreamReadStatus;
   //Declare base message data
   BaseMessageData *BMD = NULL;
   stStreamReadStatus = stDDecoder.ReadMessage(&BMD);

   json message_data = json::parse(BMD->getjsonMessageData());
   ASSERT_EQ("SOL_COMPUTED", message_data["eMyPositionStatus"]);

}

TEST_F(DecoderTest, BESTPOS_FLATTEN) {

   std::string filename = std::string(DATADIR) + "advancedecoder_bestpos.bin";

   typedef struct
   {
	   UINT    PositionStatus;
	   UINT    PositionType;
	   DOUBLE    Latitude;
	   DOUBLE    Longitude;
	   DOUBLE    OrthometricHeight;
	   FLOAT    Undulation;
	   UINT    DatumType;
	   FLOAT    LatStdDev;
	   FLOAT    LongStdDev;
	   FLOAT    HgtStdDev;
	   CHAR    BaseID[4];
	   FLOAT    DifferentialLag;
	   FLOAT    SolutionAge;
	   UCHAR    NumTracked;
	   UCHAR    NumInSolution;
	   UCHAR    NumInSolutionSingleFreq;
	   UCHAR    NumInSolutionDualFreq;
	   UCHAR    ExtendedSolutionStatus2;
	   UCHAR    ExtendedSolutionStatus;
	   UCHAR    GALandBDSSignals;
	   UCHAR    GPSandGLOSignals;
   }BESTPOS_Message;

   const char* filepath = filename.c_str();

   InputStreamInterface* ifs = NULL;

   ifs = new InputFileStream(filepath);

   Decoder stDDecoder(ifs);
   stDDecoder.EnableUnknownData(FALSE);
   stDDecoder.SetBMDOutput(FLATTEN);
   StreamReadStatus stStreamReadStatus;
   //Declare base message data
   BaseMessageData *BMD = NULL;
   stStreamReadStatus = stDDecoder.ReadMessage(&BMD);

   char* ppOutBuf = BMD->getFlattenMessageData();

   BESTPOS_Message *ptrbestpos = reinterpret_cast<BESTPOS_Message*>(ppOutBuf);

   ASSERT_EQ(0, ptrbestpos->PositionStatus);

}
TEST_F(DecoderTest, CHANCONFIGLIST) {

	std::string filename = std::string(DATADIR) + "chanconfiglist.BIN";


	typedef struct
	{
		ULONG   ulNumChans;
		UINT   eSignalType;
	}CHANCONFIGLIST_classarray;

	typedef struct
	{
		ULONG arraylength_1;
		CHANCONFIGLIST_classarray   chanconfiglist_cla[5];
	}CHANCONFIGLIST_mid_classarray;

	typedef struct
	{
		ULONG   SetInUse;
		ULONG arraylength;
		CHANCONFIGLIST_mid_classarray   chanconfiglist[2];
	}CHANCONFIGLIST_Message;



	const char* filepath = filename.c_str();

	InputStreamInterface* ifs = NULL;

	ifs = new InputFileStream(filepath);

	Decoder stDDecoder(ifs);
	stDDecoder.EnableUnknownData(FALSE);
	StreamReadStatus stStreamReadStatus;
	//Declare base message data
	BaseMessageData *BMD = NULL;
	stStreamReadStatus = stDDecoder.ReadMessage(&BMD);

	char* ppOutBuf = BMD->getFlattenMessageData();

	CHANCONFIGLIST_Message *ptrchanconfig = reinterpret_cast<CHANCONFIGLIST_Message*>(ppOutBuf);

	ASSERT_EQ(4, ptrchanconfig->chanconfiglist[0].chanconfiglist_cla[2].ulNumChans);
	json message_data = json::parse(BMD->getjsonMessageData());
	ASSERT_EQ("SBASL1", message_data["ChanCfgListArray"][2]["eSignalType"]);

}
TEST_F(DecoderTest, CHANCONFIGLIST_ASCII) {

	std::string filename = std::string(DATADIR) + "chanconfiglist.ASC";

	typedef struct
	{
		ULONG   ulNumChans;
		UINT   eSignalType;
	}CHANCONFIGLIST_classarray;

	typedef struct
	{
		ULONG arraylength_1;
		CHANCONFIGLIST_classarray   chanconfiglist_cla[5];
	}CHANCONFIGLIST_mid_classarray;

	typedef struct
	{
		ULONG   SetInUse;
		ULONG arraylength;
		CHANCONFIGLIST_mid_classarray   chanconfiglist[2];
	}CHANCONFIGLIST_Message;


	const char* filepath = filename.c_str();

	InputStreamInterface* ifs = NULL;

	ifs = new InputFileStream(filepath);

	Decoder stDDecoder(ifs);
	stDDecoder.EnableUnknownData(FALSE);
	StreamReadStatus stStreamReadStatus;
	//Declare base message data
	BaseMessageData *BMD = NULL;
	stStreamReadStatus = stDDecoder.ReadMessage(&BMD);

	char* ppOutBuf = BMD->getFlattenMessageData();

	CHANCONFIGLIST_Message *ptrchanconfig = reinterpret_cast<CHANCONFIGLIST_Message*>(ppOutBuf);

	ASSERT_EQ(4, ptrchanconfig->chanconfiglist[0].chanconfiglist_cla[2].ulNumChans);
	json message_data = json::parse(BMD->getjsonMessageData());
	ASSERT_EQ("SBASL1", message_data["ChanCfgListArray"][2]["eSignalType"]);

}

TEST_F(DecoderTest, BESTPOS) {

   std::string filename = std::string(DATADIR) + "advancedecoder_bestpos.bin";


   typedef struct
   {
	   UINT    PositionStatus;
	   UINT    PositionType;
	   DOUBLE    Latitude;
	   DOUBLE    Longitude;
	   DOUBLE    OrthometricHeight;
	   FLOAT    Undulation;
	   UINT    DatumType;
	   FLOAT    LatStdDev;
	   FLOAT    LongStdDev;
	   FLOAT    HgtStdDev;
	   CHAR    BaseID[4];
	   FLOAT    DifferentialLag;
	   FLOAT    SolutionAge;
	   UCHAR    NumTracked;
	   UCHAR    NumInSolution;
	   UCHAR    NumInSolutionSingleFreq;
	   UCHAR    NumInSolutionDualFreq;
	   UCHAR    ExtendedSolutionStatus2;
	   UCHAR    ExtendedSolutionStatus;
	   UCHAR    GALandBDSSignals;
	   UCHAR    GPSandGLOSignals;
   }BESTPOS_Message;


   const char* filepath = filename.c_str();

   InputStreamInterface* ifs = NULL;

   ifs = new InputFileStream(filepath);

   Decoder stDDecoder(ifs);
   stDDecoder.EnableUnknownData(FALSE);
   StreamReadStatus stStreamReadStatus;
   //Declare base message data
   BaseMessageData *BMD = NULL;
   stStreamReadStatus = stDDecoder.ReadMessage(&BMD);

   char* ppOutBuf = BMD->getFlattenMessageData();

   BESTPOS_Message *ptrbestpos = reinterpret_cast<BESTPOS_Message*>(ppOutBuf);

   ASSERT_EQ(0, ptrbestpos->PositionStatus);
   json message_data = json::parse(BMD->getjsonMessageData());
   ASSERT_EQ("SOL_COMPUTED", message_data["eMyPositionStatus"]);

}


TEST_F(DecoderTest, VERSION) {

	std::string filename = std::string(DATADIR) + "advancedecoder_version.bin";

	typedef struct
	{
		UINT    eComponentType;
		CHAR    szModelName[16];
		CHAR    szPSN[16];
		CHAR    szHardwareVersion[16];
		CHAR    szSoftwareVersion[16];
		CHAR    szBootVersion[16];
		CHAR    szCompileDate[12];
		CHAR    szCompileTime[12];
	}VERSION_aclVersions;
	// Msg Struct
	typedef struct
	{
		ULONG    aclVersions_arraylength;
		VERSION_aclVersions    version_aclversions[20];
	}VERSION_Message;

	const char* filepath = filename.c_str();
	InputStreamInterface* ifs = NULL;
	ifs = new InputFileStream(filepath);

	Decoder stDDecoder(ifs);
	stDDecoder.EnableUnknownData(FALSE);
	StreamReadStatus stStreamReadStatus;
	//Declare base message data
	BaseMessageData *BMD = NULL;
	stStreamReadStatus = stDDecoder.ReadMessage(&BMD);

	char* ppOutBuf = BMD->getFlattenMessageData();
	VERSION_Message *ptrversion = reinterpret_cast<VERSION_Message*>(ppOutBuf);

	ASSERT_EQ(1, ptrversion->version_aclversions[0].eComponentType);
	json message_data = json::parse(BMD->getjsonMessageData());
	ASSERT_EQ("GPSCARD", message_data["aclVersions"][0]["eComponentType"]);
}

TEST_F(DecoderTest, FILESTATUS) {

	std::string filename = std::string(DATADIR) + "advanceddecoder_filestatus.BIN";


	typedef struct
	{
		UINT    MediaType;
		UINT    FileStatus;
		UCHAR    FileName[128];
		ULONG    FileSize;
		ULONG    MediaRemainingCapacity;
		ULONG    MediaTotalCapacity;
		UCHAR    ErrorMsgString[128];
	}FILESTATUS_Message;


   const char* filepath = filename.c_str();

	InputStreamInterface* ifs = NULL;

	ifs = new InputFileStream(filepath);

	Decoder stDDecoder(ifs);
	stDDecoder.EnableUnknownData(FALSE);
	StreamReadStatus stStreamReadStatus;
	//Declare base message data
	BaseMessageData *BMD = NULL;
	stStreamReadStatus = stDDecoder.ReadMessage(&BMD);

	char* ppOutBuf = BMD->getFlattenMessageData();
	FILESTATUS_Message *ptrFileStatus = reinterpret_cast<FILESTATUS_Message*>(ppOutBuf);

	ASSERT_EQ(2, ptrFileStatus->FileStatus);
	json message_data = json::parse(BMD->getjsonMessageData());
	ASSERT_EQ("CLOSED", message_data["eMyFileStatus"]);
}
TEST_F(DecoderTest, PSRDOP) {

	std::string filename = std::string(DATADIR) + "advanceddecoder_psrdop.BIN";

	typedef struct
	{
		FLOAT    GDOP;
		FLOAT    PDOP;
		FLOAT    HDOP;
		FLOAT    HTDOP;
		FLOAT    TDOP;
		FLOAT    GPSElevMask;
		ULONG    Sats_Len;
		ULONG    Sats[325];
	}PSRDOP_Message;

	const char* filepath = filename.c_str();

	InputStreamInterface* ifs = NULL;

	ifs = new InputFileStream(filepath);

	Decoder stDDecoder(ifs);
	stDDecoder.EnableUnknownData(FALSE);
	StreamReadStatus stStreamReadStatus;
	//Declare base message data
	BaseMessageData *BMD = NULL;
	stStreamReadStatus = stDDecoder.ReadMessage(&BMD);

	char* ppOutBuf = BMD->getFlattenMessageData();
	PSRDOP_Message *ptrPSRDOP = reinterpret_cast<PSRDOP_Message*>(ppOutBuf);
	ASSERT_EQ(5.0, ptrPSRDOP->GPSElevMask);
	json message_data = json::parse(BMD->getjsonMessageData());
	ASSERT_EQ(5.0, message_data["fMyGPSElevMask"]);

}
TEST_F(DecoderTest, PSRDOP_ASCII) {

	std::string filename = std::string(DATADIR) + "advanceddecoder_psrdop.ASC";

	typedef struct
	{
		FLOAT    GDOP;
		FLOAT    PDOP;
		FLOAT    HDOP;
		FLOAT    HTDOP;
		FLOAT    TDOP;
		FLOAT    GPSElevMask;
		ULONG    Sats_Len;
		ULONG    Sats[325];
	}PSRDOP_Message;

	const char* filepath = filename.c_str();

	InputStreamInterface* ifs = NULL;

	ifs = new InputFileStream(filepath);

	Decoder stDDecoder(ifs);
	stDDecoder.EnableUnknownData(FALSE);
	StreamReadStatus stStreamReadStatus;
	//Declare base message data
	BaseMessageData *BMD = NULL;
	stStreamReadStatus = stDDecoder.ReadMessage(&BMD);

	char* ppOutBuf = BMD->getFlattenMessageData();
	PSRDOP_Message *ptrPSRDOP = reinterpret_cast<PSRDOP_Message*>(ppOutBuf);
	ASSERT_EQ(5.0, ptrPSRDOP->GPSElevMask);
	json message_data = json::parse(BMD->getjsonMessageData());
	ASSERT_EQ(5.0, message_data["fMyGPSElevMask"]);

}
TEST_F(DecoderTest, IPSTATUS) {

	std::string filename = std::string(DATADIR) + "advancedecoder_ipstatus.bin";

#pragma pack(push, 1)
	// Msg Struct
	typedef struct
	{
		UINT    Interface;
		UCHAR    IPAddress[16];
		UCHAR    Netmask[16];
		UCHAR    Gateway[16];
	}IPSTATUS_IPStatus;
	// Msg Struct
	typedef struct
	{
		UCHAR    sIPAddress[16];
	}IPSTATUS_DNSServer;
	// Msg Struct
	typedef struct
	{
		ULONG    IPStatus_arraylength;
		IPSTATUS_IPStatus    ipstatus_ipstatus[1];
		ULONG    DNSServer_arraylength;
		IPSTATUS_DNSServer    ipstatus_dnsserver[1];
	}IPSTATUS_Message;
#pragma pack(pop)

	const char* filepath = filename.c_str();

	InputStreamInterface* ifs = NULL;

	ifs = new InputFileStream(filepath);

	Decoder stDDecoder(ifs);
	stDDecoder.EnableUnknownData(FALSE);
	StreamReadStatus stStreamReadStatus;
	//Declare base message data
	BaseMessageData *BMD = NULL;
	stStreamReadStatus = stDDecoder.ReadMessage(&BMD);
	char* ppOutBuf = BMD->getFlattenMessageData();

	IPSTATUS_Message *ptrIpstatus = reinterpret_cast<IPSTATUS_Message*>(ppOutBuf);
	std::string ipaddress((CHAR*)ptrIpstatus->ipstatus_ipstatus[0].IPAddress);
	ASSERT_EQ("10.0.84.244", ipaddress);
	json message_data = json::parse(BMD->getjsonMessageData());
	ASSERT_EQ("10.0.84.244", message_data["aclMyIPStatus"][0]["sMyIPAddress"]);

}
TEST_F(DecoderTest, RAWNAVDATA)
{

#pragma pack(push, 1)
	// Msg Struct
	typedef struct
	{
		CHAR    DataBit;
		UCHAR    PowerStatus;
		ULONG    ResetLockCount;
	}RAWNAVDATA_RawNavData;
	// Msg Struct
	typedef struct
	{
		ULONG    SigChan;
		ULONG    SatID;
		UINT    SignalType;
		BOOL    NewData;
		ULONG    Weeks;
		ULONG    Milliseconds;
		ULONG    RawNavData_arraylength;
		RAWNAVDATA_RawNavData    rawnavdata_rawnavdata[256];
	}RAWNAVDATA_Message;
#pragma pack(pop)

	std::string filename = std::string(DATADIR) + "encoder_RAWNAVDATA.BIN";
	const char* filepath = filename.c_str();

	InputStreamInterface* ifs = NULL;

	ifs = new InputFileStream(filepath);

	Decoder stDDecoder(ifs);
	stDDecoder.EnableUnknownData(FALSE);
	StreamReadStatus stStreamReadStatus;
	//Declare base message data
	BaseMessageData *BMD = NULL;
	stStreamReadStatus = stDDecoder.ReadMessage(&BMD);
	char* ppOutBuf = BMD->getFlattenMessageData();
	RAWNAVDATA_Message *ptrRAWNAVDATA = reinterpret_cast<RAWNAVDATA_Message*>(ppOutBuf);
	ASSERT_EQ(2177, ptrRAWNAVDATA->SignalType);
	json message_data = json::parse(BMD->getjsonMessageData());
	ASSERT_EQ("GLOL1CA", message_data["eMySignalType"]);
}
TEST_F(DecoderTest, RAWNAVDATA_ASCII)
{

#pragma pack(push, 1)
	// Msg Struct
	typedef struct
	{
		CHAR    DataBit;
		UCHAR    PowerStatus;
		ULONG    ResetLockCount;
	}RAWNAVDATA_RawNavData;
	// Msg Struct
	typedef struct
	{
		ULONG    SigChan;
		ULONG    SatID;
		UINT    SignalType;
		BOOL    NewData;
		ULONG    Weeks;
		ULONG    Milliseconds;
		ULONG    RawNavData_arraylength;
		RAWNAVDATA_RawNavData    rawnavdata_rawnavdata[256];
	}RAWNAVDATA_Message;
#pragma pack(pop)

	std::string filename = std::string(DATADIR) + "encoder_RAWNAVDATA.GPS";
	const char* filepath = filename.c_str();

	InputStreamInterface* ifs = NULL;

	ifs = new InputFileStream(filepath);

	Decoder stDDecoder(ifs);
	stDDecoder.EnableUnknownData(FALSE);
	StreamReadStatus stStreamReadStatus;
	//Declare base message data
	BaseMessageData *BMD = NULL;
	stStreamReadStatus = stDDecoder.ReadMessage(&BMD);
	char* ppOutBuf = BMD->getFlattenMessageData();
	RAWNAVDATA_Message *ptrRAWNAVDATA = reinterpret_cast<RAWNAVDATA_Message*>(ppOutBuf);
	ASSERT_EQ(2177, ptrRAWNAVDATA->SignalType);
	json message_data = json::parse(BMD->getjsonMessageData());
	ASSERT_EQ("GLOL1CA", message_data["eMySignalType"]);
}
TEST_F(DecoderTest, VERSION_ASCII)
{

	typedef struct
	{
		UINT    eComponentType;
		CHAR    szModelName[16];
		CHAR    szPSN[16];
		CHAR    szHardwareVersion[16];
		CHAR    szSoftwareVersion[16];
		CHAR    szBootVersion[16];
		CHAR    szCompileDate[12];
		CHAR    szCompileTime[12];
	}VERSION_aclVersions;
	// Msg Struct
	typedef struct
	{
		ULONG    aclVersions_arraylength;
		VERSION_aclVersions    version_aclversions[20];
	}VERSION_Message;

	std::string filename = std::string(DATADIR) + "advancedecoder_version_asc.txt";
	const char* filepath = filename.c_str();

	InputStreamInterface* ifs = NULL;

	ifs = new InputFileStream(filepath);

	Decoder stDDecoder(ifs);
	stDDecoder.EnableUnknownData(FALSE);
	StreamReadStatus stStreamReadStatus;
	//Declare base message data
	BaseMessageData *BMD = NULL;
	stStreamReadStatus = stDDecoder.ReadMessage(&BMD);
	char* ppOutBuf = BMD->getFlattenMessageData();
	VERSION_Message *ptrversion = reinterpret_cast<VERSION_Message*>(ppOutBuf);

	ASSERT_EQ(1, ptrversion->version_aclversions[0].eComponentType);
	json message_data = json::parse(BMD->getjsonMessageData());
	ASSERT_EQ("GPSCARD", message_data["aclVersions"][0]["eComponentType"]);
}
TEST_F(DecoderTest, IPSTATUS_ASCII) {

	std::string filename = std::string(DATADIR) + "advancedecoder_ipstatus.ASC";

#pragma pack(push, 1)
	// Msg Struct
	typedef struct
	{
		UINT    Interface;
		UCHAR    IPAddress[16];
		UCHAR    Netmask[16];
		UCHAR    Gateway[16];
	}IPSTATUS_IPStatus;
	// Msg Struct
	typedef struct
	{
		UCHAR    sIPAddress[16];
	}IPSTATUS_DNSServer;
	// Msg Struct
	typedef struct
	{
		ULONG    IPStatus_arraylength;
		IPSTATUS_IPStatus    ipstatus_ipstatus[1];
		ULONG    DNSServer_arraylength;
		IPSTATUS_DNSServer    ipstatus_dnsserver[1];
	}IPSTATUS_Message;
#pragma pack(pop)

	const char* filepath = filename.c_str();

	InputStreamInterface* ifs = NULL;

	ifs = new InputFileStream(filepath);

	Decoder stDDecoder(ifs);
	stDDecoder.EnableUnknownData(FALSE);
	StreamReadStatus stStreamReadStatus;
	//Declare base message data
	BaseMessageData *BMD = NULL;
	stStreamReadStatus = stDDecoder.ReadMessage(&BMD);
	char* ppOutBuf = BMD->getFlattenMessageData();

	IPSTATUS_Message *ptrIpstatus = reinterpret_cast<IPSTATUS_Message*>(ppOutBuf);
	std::string ipaddress((CHAR*)ptrIpstatus->ipstatus_ipstatus[0].IPAddress);
	ASSERT_EQ("10.0.84.244", ipaddress);
	json message_data = json::parse(BMD->getjsonMessageData());
	ASSERT_EQ("10.0.84.244", message_data["aclMyIPStatus"][0]["sMyIPAddress"]);

}
TEST_F(DecoderTest, BESTPOS_ASCII) {

	std::string filename = std::string(DATADIR) + "advancedecoder_bestpos.ASC";


	typedef struct
	{
		UINT    PositionStatus;
		UINT    PositionType;
		DOUBLE    Latitude;
		DOUBLE    Longitude;
		DOUBLE    OrthometricHeight;
		FLOAT    Undulation;
		UINT    DatumType;
		FLOAT    LatStdDev;
		FLOAT    LongStdDev;
		FLOAT    HgtStdDev;
		CHAR    BaseID[4];
		FLOAT    DifferentialLag;
		FLOAT    SolutionAge;
		UCHAR    NumTracked;
		UCHAR    NumInSolution;
		UCHAR    NumInSolutionSingleFreq;
		UCHAR    NumInSolutionDualFreq;
		UCHAR    ExtendedSolutionStatus2;
		UCHAR    ExtendedSolutionStatus;
		UCHAR    GALandBDSSignals;
		UCHAR    GPSandGLOSignals;
	}BESTPOS_Message;


	const char* filepath = filename.c_str();

	InputStreamInterface* ifs = NULL;

	ifs = new InputFileStream(filepath);

	Decoder stDDecoder(ifs);
	stDDecoder.EnableUnknownData(FALSE);
	StreamReadStatus stStreamReadStatus;
	//Declare base message data
	BaseMessageData *BMD = NULL;
	stStreamReadStatus = stDDecoder.ReadMessage(&BMD);

	char* ppOutBuf = BMD->getFlattenMessageData();

	BESTPOS_Message *ptrbestpos = reinterpret_cast<BESTPOS_Message*>(ppOutBuf);

	ASSERT_EQ(0, ptrbestpos->PositionStatus);
	json message_data = json::parse(BMD->getjsonMessageData());
	ASSERT_EQ("SOL_COMPUTED", message_data["eMyPositionStatus"]);

}
TEST_F(DecoderTest, FILESTATUS_ASCII) {

	std::string filename = std::string(DATADIR) + "advanceddecoder_filestatus.ASC";


	typedef struct
	{
		UINT    MediaType;
		UINT    FileStatus;
		UCHAR    FileName[128];
		ULONG    FileSize;
		ULONG    MediaRemainingCapacity;
		ULONG    MediaTotalCapacity;
		UCHAR    ErrorMsgString[128];
	}FILESTATUS_Message;


	const char* filepath = filename.c_str();

	InputStreamInterface* ifs = NULL;

	ifs = new InputFileStream(filepath);

	Decoder stDDecoder(ifs);
	stDDecoder.EnableUnknownData(FALSE);
	StreamReadStatus stStreamReadStatus;
	//Declare base message data
	BaseMessageData *BMD = NULL;
	stStreamReadStatus = stDDecoder.ReadMessage(&BMD);

	char* ppOutBuf = BMD->getFlattenMessageData();
	FILESTATUS_Message *ptrFileStatus = reinterpret_cast<FILESTATUS_Message*>(ppOutBuf);

	ASSERT_EQ(2, ptrFileStatus->FileStatus);
	json message_data = json::parse(BMD->getjsonMessageData());
	ASSERT_EQ("CLOSED", message_data["eMyFileStatus"]);
}
TEST_F(DecoderTest, AllLogs)
{
	std::string filename = std::string(DATADIR) + "advanceddecoder_alllogs.BIN";
	const char* filepath = filename.c_str();

	InputStreamInterface* ifs = NULL;
	ifs = new InputFileStream(filepath);

	Decoder stADecoder(ifs);
	stADecoder.EnableUnknownData(FALSE);
	StreamReadStatus stStreamReadStatus;
	do
	{
		//Declare base message data
		BaseMessageData *pBMD = NULL;
		// Read message
		stStreamReadStatus = stADecoder.ReadMessage(&pBMD);
		if (pBMD == NULL)
		{
			continue;
		}
		delete pBMD;
		pBMD = NULL;
	} while (stStreamReadStatus.bEOS != TRUE);
	if (ifs != NULL)
	{
		delete ifs;
		ifs = NULL;
	}
}
TEST_F(DecoderTest, BESTSATS) {

   std::string filename = std::string(DATADIR) + "bestsats.BIN";

   const char* filepath = filename.c_str();

   InputStreamInterface* ifs = NULL;
   ifs = new InputFileStream(filepath);
   string nconvert_bestsats = "#BESTSATSA,COM1,0,70.5,FINESTEERING,1983,319810.000,02000020,be05,14570;21,GPS,11,GOOD,00000003,GPS,21,GOOD,00000003,GPS,24,GOOD,00000003,GPS,8,GOOD,00000003,GPS,1,GOOD,00000003,GPS,10,GOOD,00000003,GPS,14,GOOD,00000003,GPS,32,GOOD,00000003,GPS,18,GOOD,00000003,GPS,27,GOOD,00000003,SBAS,138,NOTUSED,00000000,SBAS,135,NOTUSED,00000000,GLONASS,10-7,LOCKEDOUT,00000000,GLONASS,3+5,LOCKEDOUT,00000000,GLONASS,2-4,LOCKEDOUT,00000000,GLONASS,18-3,LOCKEDOUT,00000000,GLONASS,11,LOCKEDOUT,00000000,GLONASS,1+1,LOCKEDOUT,00000000,GLONASS,24+2,LOCKEDOUT,00000000,GLONASS,17+4,LOCKEDOUT,00000000,GLONASS,8+6,LOCKEDOUT,00000000*84d3bac5\r\n";
   Decoder stDDecoder(ifs);
   stDDecoder.EnableUnknownData(FALSE);
   StreamReadStatus stStreamReadStatus;
   //Declare base message data
   BaseMessageData *BMD = NULL;
   stStreamReadStatus = stDDecoder.ReadMessage(&BMD);
   BMD->setMessageFormat(MESSAGE_ASCII);
   string message_ascii(BMD->getMessageData(), BMD->getMessageLength());
   ASSERT_EQ(message_ascii, nconvert_bestsats);

}
TEST_F(DecoderTest, PSRPOS) {

   typedef struct
    {
        UINT    PositionStatus;
        UINT    PositionType;
        DOUBLE    Latitude;
        DOUBLE    Longitude;
        DOUBLE    Height;
        FLOAT    Undulation;
        UINT    DatumType;
        FLOAT    LatStdDev;
        FLOAT    LongStdDev;
        FLOAT    HgtStdDev;
        CHAR    BaseID[4];
        FLOAT    DifferentialLag;
        FLOAT    SolutionAge;
        UCHAR    NumTracked;
        UCHAR    NumInSolution;
        UCHAR    cCharAsInt1;
        UCHAR    cCharAsInt2;
        UCHAR    MeasurementSource;
        UCHAR    ExtendedSolutionStatus;
        UCHAR    GALandBDSSignals;
        UCHAR    GPSandGLOSignals;
    }PSRPOS_Message;

   std::string filename = std::string(DATADIR) + "PSRPOS.txt";

   const char* filepath = filename.c_str();

   InputStreamInterface* ifs = NULL;
   ifs = new InputFileStream(filepath);
   Decoder stDDecoder(ifs);
   stDDecoder.EnableUnknownData(FALSE);
   StreamReadStatus stStreamReadStatus;
   //Declare base message data
   BaseMessageData *BMD = NULL;
   stStreamReadStatus = stDDecoder.ReadMessage(&BMD);
   char* ppOutBuf = BMD->getFlattenMessageData();
	PSRPOS_Message *ptrPSRPOS = reinterpret_cast<PSRPOS_Message*>(ppOutBuf);
   ASSERT_EQ(17.44301336223, ptrPSRPOS->Latitude);
}

TEST_F(DecoderTest, InvalidStream)
{
   InputStreamInterface* ifs = NULL;
   try
   {
      Decoder clDecoder(ifs);
	   ASSERT_TRUE(1 == 0); // Execution should not reach here
   }
   catch(nExcept e)
   {
      ASSERT_STRCASEEQ(e.buffer, "NULL Input Stream Interface Pointer");
   }
}

TEST_F(DecoderTest, UnknownDataFile)
{
   std::string filename = std::string(DATADIR) + "unknowndata.txt";
   const char* filepath = filename.c_str();
   InputStreamInterface* ifs = new InputFileStream(filepath);

   Decoder clDecoder(ifs);
   clDecoder.EnableUnknownData(TRUE);

   StreamReadStatus stStreamReadStatus;
   //Declare base message data
   BaseMessageData *BMD = NULL;
   stStreamReadStatus = clDecoder.ReadMessage(&BMD);

   ASSERT_TRUE(BMD->getMessageFormat() == MESSAGE_UNKNOWN);
   ASSERT_TRUE(BMD->getMessageLength() == 28);
}

TEST_F(DecoderTest, NMEAMessageDecoding)
{
   std::string filename = std::string(DATADIR) + "nmea_sample.txt";
   const char* filepath = filename.c_str();
   InputStreamInterface* ifs = new InputFileStream(filepath);

   Decoder clDecoder(ifs);
   clDecoder.EnableUnknownData(TRUE);

   StreamReadStatus stStreamReadStatus;
   //Declare base message data
   BaseMessageData *BMD = NULL;

   stStreamReadStatus = clDecoder.ReadMessage(&BMD);
   ASSERT_TRUE(BMD->getMessageFormat() == MESSAGE_ASCII);
   ASSERT_TRUE(BMD->getMessageLength() == 82);
   ASSERT_TRUE(BMD->getMessageName() == "GPALM");
   delete BMD;
   BMD = NULL;

   stStreamReadStatus = clDecoder.ReadMessage(&BMD);
   ASSERT_TRUE(BMD->getMessageFormat() == MESSAGE_ASCII);
   ASSERT_TRUE(BMD->getMessageLength() == 82);
   ASSERT_TRUE(BMD->getMessageName() == "GPALM");
   delete BMD;
   BMD = NULL;

   stStreamReadStatus = clDecoder.ReadMessage(&BMD);
   ASSERT_TRUE(BMD->getMessageFormat() == MESSAGE_ASCII);
   ASSERT_TRUE(BMD->getMessageLength() == 34);
   ASSERT_TRUE(BMD->getMessageName() == "GPZDA");
   delete BMD;
   BMD = NULL;

   clDecoder.Reset();
   stStreamReadStatus = clDecoder.ReadMessage(&BMD);
   ASSERT_TRUE(BMD->getMessageFormat() == MESSAGE_ASCII);
   ASSERT_TRUE(BMD->getMessageLength() == 82);
   ASSERT_TRUE(BMD->getMessageName() == "GPALM");
   delete BMD;
   BMD = NULL;

   delete ifs;
   ifs = NULL;
}

TEST_F(DecoderTest, SkipCRCValidation)
{
   std::string filename = std::string(DATADIR) + "invalidCRC.txt";
   const char* filepath = filename.c_str();
   InputStreamInterface* ifs = new InputFileStream(filepath);

   Decoder clDecoder(ifs);
   clDecoder.EnableUnknownData(TRUE);

   StreamReadStatus stStreamReadStatus;
   //Declare base message data
   BaseMessageData *BMD = NULL;

   clDecoder.SkipCRCValidation(TRUE);
   stStreamReadStatus = clDecoder.ReadMessage(&BMD);
   ASSERT_TRUE(BMD->getMessageFormat() == MESSAGE_ASCII);
   ASSERT_TRUE(BMD->getMessageLength() == 218);
   ASSERT_TRUE(BMD->getMessageName() == "BESTPOS");
   ASSERT_TRUE(stStreamReadStatus.bEOS == FALSE);
   ASSERT_TRUE(stStreamReadStatus.uiPercentStreamRead == 100);
   delete BMD;
   BMD = NULL;

   stStreamReadStatus = clDecoder.ReadMessage(&BMD);
   ASSERT_TRUE(stStreamReadStatus.bEOS == TRUE);
   ASSERT_TRUE(stStreamReadStatus.uiPercentStreamRead == 100);
   delete BMD;
   BMD = NULL;
}

TEST_F(DecoderTest, ResponseDecoding)
{
   std::string filename = std::string(DATADIR) + "asciiresponse.txt";
   const char* filepath = filename.c_str();
   InputStreamInterface* ifs = new InputFileStream(filepath);

   Decoder clDecoder(ifs);
   clDecoder.EnableUnknownData(TRUE);

   StreamReadStatus stStreamReadStatus;
   //Declare base message data
   BaseMessageData *BMD = NULL;

   clDecoder.SkipCRCValidation(TRUE);
   stStreamReadStatus = clDecoder.ReadMessage(&BMD);

   EXPECT_TRUE(BMD->getMessageType() == TRUE);
   EXPECT_TRUE(BMD->getMessageName() == "BESTPOS");

   delete BMD;
   BMD = NULL;
}
TEST_F(DecoderTest, RAWIMUSX) {


#pragma pack(push, 1)
   // Msg Struct
   typedef struct
   {
      UCHAR    IMUStatusInfo;
      UCHAR    IMUType;
      USHORT    GPSWeek;
      DOUBLE    GPSSeconds;
      ULONG    IMUStatus;
      LONG    AccelZ;
      LONG    AccelY;
      LONG    AccelX;
      LONG    GyroZ;
      LONG    GyroY;
      LONG    GyroX;
   }RAWIMUSX_Message;
#pragma pack(pop)

   std::string filename = std::string(DATADIR) + "rawimusx.BIN";

   const char* filepath = filename.c_str();

   InputStreamInterface* ifs = NULL;
   ifs = new InputFileStream(filepath);
   Decoder stDDecoder(ifs);
   stDDecoder.EnableUnknownData(FALSE);
   StreamReadStatus stStreamReadStatus;
   //Declare base message data
   BaseMessageData *BMD = NULL;
   stStreamReadStatus = stDDecoder.ReadMessage(&BMD);
   char* ppOutBuf = BMD->getFlattenMessageData();
   RAWIMUSX_Message *ptrRAWIMUSX = reinterpret_cast<RAWIMUSX_Message*>(ppOutBuf);
   ASSERT_EQ(-817242, ptrRAWIMUSX->AccelY);
   json message_data = json::parse(BMD->getjsonMessageData());
   ASSERT_EQ(-817242, message_data["lMyAccelY"]);
}
TEST_F(DecoderTest, RAWIMUSX_ASCII) {


#pragma pack(push, 1)
   // Msg Struct
   typedef struct
   {
      UCHAR    IMUStatusInfo;
      UCHAR    IMUType;
      USHORT    GPSWeek;
      DOUBLE    GPSSeconds;
      ULONG    IMUStatus;
      LONG    AccelZ;
      LONG    AccelY;
      LONG    AccelX;
      LONG    GyroZ;
      LONG    GyroY;
      LONG    GyroX;
   }RAWIMUSX_Message;
#pragma pack(pop)

   std::string filename = std::string(DATADIR) + "rawimusx.ASC";

   const char* filepath = filename.c_str();

   InputStreamInterface* ifs = NULL;
   ifs = new InputFileStream(filepath);
   Decoder stDDecoder(ifs);
   stDDecoder.EnableUnknownData(FALSE);
   StreamReadStatus stStreamReadStatus;
   //Declare base message data
   BaseMessageData *BMD = NULL;
   stStreamReadStatus = stDDecoder.ReadMessage(&BMD);
   char* ppOutBuf = BMD->getFlattenMessageData();
   RAWIMUSX_Message *ptrRAWIMUSX = reinterpret_cast<RAWIMUSX_Message*>(ppOutBuf);
   ASSERT_EQ(-817242, ptrRAWIMUSX->AccelY);
   json message_data = json::parse(BMD->getjsonMessageData());
   ASSERT_EQ(-817242, message_data["lMyAccelY"]);
}
