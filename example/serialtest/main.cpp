
#include <Windows.h>
#include <iostream>
#include "inputstreaminterface.hpp"
#include "inputportstream.hpp"
#include "outputstreaminterface.hpp"
#include "outputportstream.hpp"
#include "outputfilestream.hpp"
#include "multioutputfilestream.hpp"
#include "framer.hpp"
#include "DriverInterfaceAPI.h"
#include "decoder.hpp"

using namespace std;
using namespace DriverInterfaceAPI;

int main()
{
   // JSON file should be at the same location of executable 
   std::string filename_json = "dynamicdef.JSON";
   const char*	filepath_json = filename_json.c_str();
   loadDataFromJSON::initializeDatabaseObj();
   JSONFileReader* jsonReader = NULL;
   jsonReader = loadDataFromJSON::getDatabaseObj(filepath_json);
   
   // Port can be configured here with port number, baud rate etc
   SerialPortConfig* config = new SerialPortConfig(12,115200);
   IDeviceDriver* iDriver = NULL;
   GetInterface(config, iDriver);
   iDriver->Open(config);

   InputStreamInterface* ifs = new InputPortStream(iDriver);

   // Call back can be enabled here, if disabled polling mechanism will be active
   ifs->EnableCallBack(FALSE);

  /* OutputStreamInterface* ofs = new OutputPortStream(iDriver);
   // we are creating a basemessagedata to write on port 
   MessageHeader stMessageHeader;
   char* chDecodedMessage = new char [strlen("log usb1 psrdopb ontime 1\r") + 1];
   strcpy(chDecodedMessage, "log usb1 psrdopb ontime 1\r");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
   BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
   clBaseMessageData.setMessageLength(strlen("log usb1 psrdopb ontime 1\r"));
   ofs->WriteData(clBaseMessageData);*/
   //std::cin.get();

   // Testing the psrdop messages 
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
 
   Decoder stDDecoder(ifs);
   stDDecoder.EnableUnknownData(FALSE);
 
   //Declare Stream Read Status Enum
   StreamReadStatus stStreamReadStatus;
   int iNumIterations = 25;
   
   while((iNumIterations > 0))
   {
      Sleep(1000);
      BaseMessageData *BMD = NULL;
      try
      {
		 // Reading the messages from the configured port 
         stStreamReadStatus = stDDecoder.ReadMessage(&BMD);
         char* ppOutBuf = BMD->getFlattenMessageData();
         std::cout << BMD->getjsonMessageData() << "\n";
         PSRDOP_Message *ptrVERSION_aclVersions = reinterpret_cast<PSRDOP_Message*>(ppOutBuf);
		 std::cout << ptrVERSION_aclVersions->GDOP << " >>>>" << std::endl;

         delete BMD;
         BMD = NULL;
      }
      catch(char const *caught) 
      {
         std::cout << "Got " << caught << std::endl;
      }
      //iNumIterations--;
    }

    /*MessageHeader stMessageHeader1;
    char* chDecodedMessage1 = new char [strlen("unlog bestposa\r") + 1];
    strcpy(chDecodedMessage1, "unlog bestposa\r");
    stMessageHeader1.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
    BaseMessageData clBaseMessageData1(&stMessageHeader1, chDecodedMessage1);
    clBaseMessageData1.setMessageLength(strlen("unlog bestposa\r"));
    ofs->WriteData(clBaseMessageData1);*/
	
    std::cin.get();

    iDriver->Close();
    delete config;
    delete ifs;
    //delete ofs;
    config = NULL;
    ifs = NULL;
    //ofs = NULL;
    return 0;
}
