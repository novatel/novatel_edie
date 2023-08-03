# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.xx.xx] - 2022-00-00
### Added
- .
### Fixed
- Moved Decoder FieldContainers into appropriate Namespace


## [2.11.0] - 2022-07-08
### Added
- Added ascii & binary RXCONFIG decoding/encoding
### Fixed
- Updates some types to standard C++ types vs NovAtel types 0223068adee476a111354be912c94d2aca62eca5

## [2.10.00] - 2022-06-03
### Added
- Abbreviated ASCII framing and decoding 45ebaeb4cf99072d6b8e42bc5b932d586c2a07e1
- Add Abbreviated ASCII to Binary/ASCII Encoding 123aa475f12d1ce426e479b6a740ee5a565360a5
- RINEX: Allow user to override UTC offset for Glonass 39f236032f21c6bd8ebc0e266570c01b25ff54f9
- RINEX: Allow user to add comments in Obs file header 2b2a381c7ffb5827aeb7aa5c0e179efa7ccbb7ad
- RINEX: Seperate messages by antenna in EncoderStatistics b57e7d15082c160f8d3560a8b806279ee3ec82a6
### Fixed
- Improved JSON DB interface to Filter
- Logging logic. No longer create log file or console output by default
- No longer passing filtered logs back to user as 'UNNOWN' logs. The next unfilted log will be returned instead. b617da61239e6265275ab659b99fbcc3f134b2d2
- Default NMEA logs to passthrough the filter 6f6753df64ecdb28f60a8a4cfa2897a7cf760900
- RINEX: Fix RangeCMP decompression crash. Updated Signal enums d0c6b9d304607461c4e04045a4990199f2e62bfc
- RINEX: Fix PRN formatting 2c6438758ddf3466eade3128ca43f9feca062fc9

## [2.8.08] - 2022-05-05
### Added
- Redesign filter to eliminate HEADERFORMAT filter, and Couple HEADERFORMAT to Msg Id, and Msg Name
- Add ANTENNA_SOURCE enum class to DecoderResult
- Add ability to synchronize logging across Logger Objects and clear logging errors between multiple objects
- Filter to use Parser's database
- 
### Fixed
- RINEX encoding statistics
- RINEX crash
- Normalize paths to other files
- Passthrough conversion issues for 99% of cases
- Framing not having enough bytes to check CRC
- Disable JSON framing in examples and add missing decoder status
- RINEX position averaging and expose api for ellipsoid height

## [2.7.30] - 2022-03-21
### Added
- JSON encoding response fields
- Short header encoding
- RangeCMP decompression
- JSON reader performance improvments by changing pass-by-value to pass-by-reference
### Fixed
- Exception handling
- RINEX fixes
- CI/CD cleanup

## [2.0.00] - 2022-00-00
### Added
- 
### Fixed
- 

## [2.7.20] - 2022-01-24
### Added
- JSON schema updates
- Expose FileParser to DLL
### Fixed
- Various fixes

## [2.7.13] - 2022-01-07
### Added
- Filtering to Fileparser
### Fixed
- Fix byte array encoding for empty arrays
- Numerous RINEX fixes

## [2.6.4] - 2021-11-29
### Added
- Various filtering interface changes
### Fixed
- Add thread safey code to Python 
- Various CI/CD improvements
- %T log field encoding fix
- Adjustments to python DecoderResult and LogData struct members to return valid byte buffer for decoded log
- Added members to DecoderResult and LogData to indicate name, header length and body length of the decoded log
- Numerous bug fixes
- Python optimizations

## [2.5.0] - 2021-11-17
### Added
- Framing ENCRYPTED BINARY logs

## [2.4.0] - 2021-11-16
### Added
- JSON decoding and encoding added

## [2.3.0] - 2021-11-10
### Added
- RINEX converter (NovAtel to RINEX)

## [2.2.2] - 2021-10-28
### Added
- Optimize DLL/SO interfaces fa0b0b9d5a4c081ff53a9143608c2b021a02ee05
- Add FileParser class. Combines file reading and Parser classes together
### Fixed
- Various Python bug fixes
- Various bug fixes e0444866848bbebf05b0b512d9834c4e522313ef 
- Fix make booleans in decoded logs 1 byte vs NovAtel 4 byte booleans 0e0e3a59887f544d8254e20956efac4cfa942e10 

## [2.1.0] - 2021-10-21
### Added
- Logging support using spdlog 8afc67ec391703086c830c1f61226e505113c010
- JSON decoder f70a45e2270a19398bdd5d247877ef508dc36262
- Added filtering to the framer  
    ```
    novatel::edie::oem::Framer clFramer = novatel::edie::oem::Framer();
    FrameData stFrameData;
    novatel::edie::oem::Filter clFilter = novatel::edie::oem::Filter(&clJsonDb);
    std::vector<std::pair<std::string, HEADERFORMAT>> vmessageNameFilter;
    vmessageNameFilter.emplace_back(std::make_pair("BESTPOS", HEADERFORMAT::ALL));
    clFramer.AttachFilter(&clFilter);
    ```
- Added Parser class to NovAtel Decoder (combines Filter and Decoder classes)
- Expose logging to DLL/SO 12e99a4dc487895fd987db042c9400139bb2c1ea
### Changed
- Versioning changed from MR into `main` branch to MR into `develop`
- Move versioning file from `src/decoders/version.h` to `src/` e591ca45a8a5e225b4f66b4d1b0d2801ff2817fe
    - Abandoning the idea of individually versioned components for now. May revisit this decision in the future.
### Fixed
- bug fixes and improvments 621c136fd18c2839ca8aa62b9a8c39909ac85fdf e22f111489cdd4fe5e2add8841889b4dbab4be57 c026b5de61381294adb231b7eeccd59ef462a591 deb9a58725ce20b98b5033fbe9de5ea0a48c35f0 2b490e1c9207f776dcbfe2cb467aad8867a0a948 3f43489be5ac63e3188ed858084c12f7af668fd8 10edd9c0dbb25e5c5a7f84c6d01a202584432023 87766e0beae71c3b59de6c7cfcee6456dd377703 

## [2.0.0] - 2021-09-11
### Added
- Automotive framer d31d58572cb45b1598d4cc378184dc155c96d5e7
- PIMTP framer a0c3a839f4ada0287b572fb8938f049cfa42f9ab
- JSON encoder a391069ab708970d074e6469f4702c767e4cd3ea

### Changed 
#### Novatel Decoder
- Changed the default branch from `master` to `main` to refelect the updated defaults from Gitlab
- Split apart the framing and decoding code. The Decoder class no longer inherits from the Framer class.
- Split apart the decode and encode/transform aspects of the 
- Major refactorings across the entire codebase
    - Simlified decoding logic. Cleaned up maintainability and readibility.
    - Significant speed improvments upwards of 10x improvment in logs decoded per second (l/s)
- Split framing and decoding into discrete classes  
    ### Version differences examples  
    **v1.0.0**  
    ```
    InputStreamInterface* ifs = new InputFileStream(convert_file.c_str());
    loadDataFromJSON::initializeDatabaseObj();
    JSONFileReader* jsonReader = NULL;
    jsonReader = loadDataFromJSON::getDatabaseObj(json_file.c_str());

    Decoder stDDecoder(ifs);
    StreamReadStatus stStreamReadStatus;

    do
    {
        BMD->setMessageFormat(MESSAGE_ASCII);
        if (BMD == NULL)
        {
            continue;
        }
    } while (stStreamReadStatus.bEOS != TRUE);
    ```
    
    **v2.0.0 Framer + Decoder**  
    ```
    JsonReader clJsonDb;
    clJsonDb.LoadFile(sJsonDB);

    novatel::edie::oem::Framer clFramer = novatel::edie::oem::Framer(&clJsonDb);
    novatel::edie::oem::Decoder clDecoder = novatel::edie::oem::Decoder(&clJsonDb);

    FrameData stFrameData;

    novatel::edie::oem::DecoderResult stDecoderResult;
    InputFileStream clTestFS = InputFileStream(sInFilename.c_str());

    StreamReadStatus stReadStatus;
    ReadDataStructure stReadData;

    CHAR* pcReadBuffer = new CHAR[MAX_ASCII_MESSAGE_LENGTH];
    CHAR* pcDecoderOutputBuffer = new CHAR[MAX_ASCII_MESSAGE_LENGTH];

    while (!stReadStatus.bEOS)
    {
        stReadStatus = clTestFS.ReadData(stReadData);
        clFramer.Write(stReadData.cData, stReadStatus.uiCurrentStreamRead);

        while (true)
        {
            stFrameData = clFramer.GetFrame(pcReadBuffer, MAX_ASCII_MESSAGE_LENGTH);
            stDecoderResult = clDecoder.DecodeMessage(pcReadBuffer, &pcDecoderOutputBuffer, MAX_ASCII_MESSAGE_LENGTH, eOutputFormat);      
        }
    }
    ```  

    **v2.0.0 Parser (combines Framer and Decoder)**  
    ```
    novatel::edie::oem::Parser clParser = novatel::edie::oem::Parser(sJsonDB);
    InputFileStream clTestFS = InputFileStream(sInFilename.c_str());
    
    LogDataStruct stLogData;
    ParserConfigStruct stConfig;
    StreamReadStatus stReadStatus;

    while (!stReadStatus.bEOS)
    {
        stReadStatus = clTestFS.ReadData(stReadData);
        uiBytesWritten = clParser.Write(stReadData.cData, stReadStatus.uiCurrentStreamRead);

        do
        {
            stLogData = clParser.Read();
        } while (stLogData.eParserStatus != PARSERSTATUS::NEED_BYTES);
    }
    ```

    **v2.0.0 FileParser (combines Parser and InputFileStream)**  
    ```
    novatel::edie::oem::FileParser clFileParser = novatel::edie::oem::FileParser(sJsonDB);
    clFileParser.SetStream(&clTestFS, eOutputStyle)

    LogDataStruct stLogData;
    ParserConfigStruct stConfig;
    while (stLogData.eParserStatus != PARSERSTATUS::STREAM_EXHASUTED)
    {
        stLogData = clFileParser.Read();
    }
    ```
 
- New Parser class that uses Framer and Decoder classes to make it easier for users to frame and decode data
- New FileParser class that uses Parser class to make it easier for users to manage an input file with a parser
- New JSON DB format
- Expanded the unit testing. There is now unit testing for decoding of every field type.
- Stats for framed and decoded logs in the Parser class

### Removed 
- Removed BMD from most of the active library
    - There are a few instances of it still in EDIE. Future version of EDIE will remove any other instances of BMD 

### Fixed 
- Fixed many decoding bugs


## [1.0.0] - 2021-05-04
### Added
- Initial Release of EDIE
- Decode NovAtel Logs (https://docs.novatel.com/OEM7/Content/Logs/OEM7_Core_Logs.htm)
- Interfaces for files, serial ports, and memory buffers
- DLL/SO interfaces for most libraries
- Python package utilizing the EDIE DLL/SO objects

[Unreleased]: https://gitlab.corp.novatel.ca/hyd-tools/open-source-edie/nov-decoder
<!--- [2.2.2]: https://gitlab.corp.novatel.ca/hyd-tools/open-source-edie/nov-decoder/-/tree/2.2.2 Release CI broken --->
<!--- [//]: # ([2.1.0]: https://gitlab.corp.novatel.ca/hyd-tools/open-source-edie/nov-decoder/-/tree/2.1.0 Release CI broken) --->
[2.0.0]: https://gitlab.corp.novatel.ca/hyd-tools/open-source-edie/nov-decoder/-/tree/2.0.1
[1.0.0]: https://github.com/novatel/novatel_edie/releases/tag/1.0.0 
