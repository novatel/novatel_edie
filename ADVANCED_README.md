<div align="center">
    <img alt="edie_logo" src=./resources/novatel-edie-logo-body.png width="40%">
</div>

# NovAtel EDIE Advanced Readme

## Table of Contents

- [Generate Code Documentation](#generate-code-documentation)
  - [Windows](#windows)
  - [Linux](#linux)
- [Decoding Chain](#decoding-chain)
- [EDIE Data Structures](#edie-data-structures)
  - [MessageDataStruct](#messagedatastruct)
  - [MetaDataStruct](#metadatastruct)

## Generate Code Documentation

### Windows

1. Install exhale and sphinx_rtd_theme: `pip3 install exhale sphinx_rtd_theme`
2. Run Sphinx on each component. For example: `sphinx-build src/decoders/common/doc doc/decoders/common/doc/html`

### Linux

1. Update the system: `apt-get update`
2. Install tzdata, doxygen, and python3-pip: `apt-get install -y tzdata doxygen python3-pip`
3. Install exhale and sphinx_rtd_theme: `pip3 install exhale sphinx_rtd_theme`
4. Run Sphinx on each component. For example: `sphinx-build src/decoders/common/doc doc/decoders/common/doc/html`

## Decoding Chain

The EDIE decoding chain has components that pass common structures (green boxes) between them to frame, decode, and encode each message.

<div align="center">
    <img alt="edie_logo" src=./resources/novatel-edie-decode-chain.png>
</div>

## EDIE Data Structures

#### MessageDataStruct

The `MessageDataStruct` stores encoded message components such as the header, header length, message body, and body length.

```cpp
struct MessageDataStruct
{
   unsigned char* pucMessageHeader; // Pointer to the message header.
   uint32_t uiMessageHeaderLength;  // Length of the message header in bytes.
   unsigned char* pucMessageBody;   // Pointer to the message body.
   uint32_t uiMessageBodyLength;    // Length of the message body in bytes.
   unsigned char* pucMessage;       // Pointer to the message beginning.
   uint32_t uiMessageLength;        // Length of the message in bytes.
}
```

#### MetaDataStruct

The `MetaDataStruct` stores message information before it was decoded, or information that is unchanged by decoding and encoding.

```cpp
struct MetaDataStruct
{
   HEADERFORMAT eFormat;                                // The format of the message when it was framed.
   MEASUREMENT_SOURCE eMeasurementSource;               // Also called the sibling ID.
   TIME_STATUS eTimeStatus;                             // The GPS Time Status of the message.
   bool bResponse;                                      // True if the message is a response to a command.
   uint16_t usWeek;                                     // The GPS Week number.
   double dMilliseconds;                                // The GPS Milliseconds.
   uint32_t uiBinaryMsgLength;                          // Message length according to the binary header. This field is only used if eFormat is HEADERFORMAT::BINARY.
   uint32_t uiLength;                                   // Length of the entire message, including the header and CRC.
   uint32_t uiHeaderLength;                             // The length of the message header.
   uint16_t usMessageID;                                // The message ID.
   uint32_t uiMessageCRC;                               // The message definition CRC.
   char acMessageName[OEM4_ASCII_MESSAGE_NAME_MAX+1];   // The name of the message
};
```
