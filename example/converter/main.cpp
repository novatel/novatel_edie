// demoapplication.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <chrono>
#include "src/hw_interface/stream_interface/api/inputstreaminterface.hpp"
#include "src/hw_interface/stream_interface/api/inputfilestream.hpp"
#include "src/hw_interface/stream_interface/api/outputfilestream.hpp"
#include "src/decoders/novatel/api/framer.hpp"
#include "src/decoders/novatel/api/decoder.hpp"
#include "src/decoders/version.h"

inline bool file_exists(const std::string& name) {
   struct stat buffer;
   return (stat(name.c_str(), &buffer) == 0);
}

int main(int argc, char** argv)
{
   if (argc == 2 && strcmp(argv[1], "-V") == 0)
   {
      std::cout << "Decoder library infomration" << endl << get_pretty_version() << endl;
      return 0;
   }

   if (argc != 3)
   {
      std::cerr << "Usage: " << argv[0] << "LOG_DEFINITION CONVERT_FILE" << std::endl << "\t-V:\tPrint Version information" << std::endl;
      return 1;
   }
   if (!file_exists(argv[1]))
   {
      std::cerr << "File \"" << argv[1] << "\" does not exist" << std::endl;
      return 1;
   }
   if (!file_exists(argv[2]))
   {
      std::cerr << "File \"" << argv[2] << "\" does not exist" << std::endl;
      return 1;
   }

   std::cout << "Decoder library infomration" << endl;
   std::cout << get_pretty_version() << endl << endl;;

   std::map<UINT, UINT> mConversionCounter;

   std::string json_file = argv[1];
   std::string convert_file = argv[2];
   
   InputStreamInterface* ifs = new InputFileStream(convert_file.c_str());;
   OutputFileStream* ofs = new OutputFileStream((convert_file + ".asc").c_str());

   // Initialize the JSON database
   auto t1 = chrono::high_resolution_clock::now();
   cout << "Loading Database... ";

   loadDataFromJSON::initializeDatabaseObj();
   JSONFileReader* jsonReader = NULL;
   jsonReader = loadDataFromJSON::getDatabaseObj(json_file.c_str());

   std::cout << "DONE(" << chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - t1).count() << "ms)" << std::endl;

   // Initialize the Decoder
   Decoder stDDecoder(ifs);
   stDDecoder.EnableUnknownData(FALSE);
   stDDecoder.SetBMDOutput(BOTH);
   StreamReadStatus stStreamReadStatus;

   //Declare base message data
   BaseMessageData *BMD = NULL;

   t1 = chrono::high_resolution_clock::now();
   do
   {
      stStreamReadStatus = stDDecoder.ReadMessage(&BMD);

      if (BMD != NULL)
      {
         BMD->setMessageFormat(MESSAGE_ASCII);
         UINT uiLenght = ofs->WriteData(*BMD);
         if (BMD == NULL)
         {
            continue;
         }

         mConversionCounter[BMD->getMessageID()] += 1;
         delete BMD;
         BMD = NULL;
      }
      else
      {
         continue;
      }
   } while (stStreamReadStatus.bEOS != TRUE);

   cout << endl << "Conversion time: " << chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - t1).count() << "ms" << endl << endl;

   for (auto uiMsgCounter : mConversionCounter)
   {
      cout << uiMsgCounter.first << ": " << uiMsgCounter.second << endl;
   }
}
