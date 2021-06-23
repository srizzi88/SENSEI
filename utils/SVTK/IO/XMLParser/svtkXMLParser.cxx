/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLParser.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLParser.h"
#include "svtkObjectFactory.h"
#include "svtk_expat.h"

#include "svtksys/FStream.hxx"
#include <svtksys/SystemTools.hxx>

#include <cctype>

svtkStandardNewMacro(svtkXMLParser);

//----------------------------------------------------------------------------
svtkXMLParser::svtkXMLParser()
{
  this->Stream = nullptr;
  this->Parser = nullptr;
  this->FileName = nullptr;
  this->Encoding = nullptr;
  this->InputString = nullptr;
  this->InputStringLength = 0;
  this->ParseError = 0;
  this->IgnoreCharacterData = 0;
}

//----------------------------------------------------------------------------
svtkXMLParser::~svtkXMLParser()
{
  this->SetStream(nullptr);
  this->SetFileName(nullptr);
  this->SetEncoding(nullptr);
}

//----------------------------------------------------------------------------
void svtkXMLParser::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Stream)
  {
    os << indent << "Stream: " << this->Stream << "\n";
  }
  else
  {
    os << indent << "Stream: (none)\n";
  }
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "IgnoreCharacterData: " << (this->IgnoreCharacterData ? "On" : "Off") << endl;
  os << indent << "Encoding: " << (this->Encoding ? this->Encoding : "(none)") << "\n";
}

//----------------------------------------------------------------------------
static int svtkXMLParserFail(istream* stream)
{
  // The fail() method returns true if either the failbit or badbit is set.
#if defined(__HP_aCC)
  // The HP compiler sets the badbit too often, so we ignore it.
  return (stream->rdstate() & ios::failbit) ? 1 : 0;
#else
  return stream->fail() ? 1 : 0;
#endif
}

//----------------------------------------------------------------------------
svtkTypeInt64 svtkXMLParser::TellG()
{
  // Standard tellg returns -1 if fail() is true.
  if (!this->Stream || svtkXMLParserFail(this->Stream))
  {
    return -1;
  }
  return this->Stream->tellg();
}

//----------------------------------------------------------------------------
void svtkXMLParser::SeekG(svtkTypeInt64 position)
{
  // Standard seekg does nothing if fail() is true.
  if (!this->Stream || svtkXMLParserFail(this->Stream))
  {
    return;
  }
  this->Stream->seekg(std::streampos(position));
}

//----------------------------------------------------------------------------
int svtkXMLParser::Parse(const char* inputString)
{
  this->InputString = inputString;
  this->InputStringLength = -1;
  int result = this->Parse();
  this->InputString = nullptr;
  return result;
}

//----------------------------------------------------------------------------
int svtkXMLParser::Parse(const char* inputString, unsigned int length)
{
  this->InputString = inputString;
  this->InputStringLength = length;
  int result = this->Parse();
  this->InputString = nullptr;
  this->InputStringLength = -1;
  return result;
}

//----------------------------------------------------------------------------
int svtkXMLParser::Parse()
{
  // Select source of XML
  svtksys::ifstream ifs;
  if (!this->InputString && !this->Stream && this->FileName)
  {
    // If it is file, open it and set the appropriate stream
    svtksys::SystemTools::Stat_t fs;
    if (svtksys::SystemTools::Stat(this->FileName, &fs) != 0)
    {
      svtkErrorMacro("Cannot open XML file: " << this->FileName);
      return 0;
    }
#ifdef _WIN32
    ifs.open(this->FileName, ios::binary | ios::in);
#else
    ifs.open(this->FileName, ios::in);
#endif
    if (!ifs)
    {
      svtkErrorMacro("Cannot open XML file: " << this->FileName);
      return 0;
    }
    this->Stream = &ifs;
  }

  // Create the expat XML parser.
  this->CreateParser();

  XML_SetElementHandler(
    static_cast<XML_Parser>(this->Parser), &svtkXMLParserStartElement, &svtkXMLParserEndElement);
  if (!this->IgnoreCharacterData)
  {
    XML_SetCharacterDataHandler(
      static_cast<XML_Parser>(this->Parser), &svtkXMLParserCharacterDataHandler);
  }
  else
  {
    XML_SetCharacterDataHandler(static_cast<XML_Parser>(this->Parser), nullptr);
  }
  XML_SetUserData(static_cast<XML_Parser>(this->Parser), this);

  // Parse the input.
  int result = this->ParseXML();

  if (result)
  {
    // Tell the expat XML parser about the end-of-input.
    if (!XML_Parse(static_cast<XML_Parser>(this->Parser), "", 0, 1))
    {
      this->ReportXmlParseError();
      result = 0;
    }
  }

  // Clean up the parser.
  XML_ParserFree(static_cast<XML_Parser>(this->Parser));
  this->Parser = nullptr;

  // If the source was a file, reset the stream
  if (this->Stream == &ifs)
  {
    this->Stream = nullptr;
  }

  return result;
}

//----------------------------------------------------------------------------
int svtkXMLParser::CreateParser()
{
  if (this->Parser)
  {
    svtkErrorMacro("Parser already created");
    return 0;
  }
  // Create the expat XML parser.
  this->Parser = XML_ParserCreate(this->Encoding);
  return this->Parser ? 1 : 0;
}

//----------------------------------------------------------------------------
int svtkXMLParser::InitializeParser()
{
  // Create the expat XML parser.
  if (!this->CreateParser())
  {
    svtkErrorMacro("Parser already initialized");
    this->ParseError = 1;
    return 0;
  }

  XML_SetElementHandler(
    static_cast<XML_Parser>(this->Parser), &svtkXMLParserStartElement, &svtkXMLParserEndElement);
  if (!this->IgnoreCharacterData)
  {
    XML_SetCharacterDataHandler(
      static_cast<XML_Parser>(this->Parser), &svtkXMLParserCharacterDataHandler);
  }
  else
  {
    XML_SetCharacterDataHandler(static_cast<XML_Parser>(this->Parser), nullptr);
  }
  XML_SetUserData(static_cast<XML_Parser>(this->Parser), this);
  this->ParseError = 0;
  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLParser::ParseChunk(const char* inputString, unsigned int length)
{
  if (!this->Parser)
  {
    svtkErrorMacro("Parser not initialized");
    this->ParseError = 1;
    return 0;
  }
  int res;
  res = this->ParseBuffer(inputString, length);
  if (res == 0)
  {
    this->ParseError = 1;
  }
  return res;
}

//----------------------------------------------------------------------------
int svtkXMLParser::CleanupParser()
{
  if (!this->Parser)
  {
    svtkErrorMacro("Parser not initialized");
    this->ParseError = 1;
    return 0;
  }
  int result = !this->ParseError;
  if (result)
  {
    // Tell the expat XML parser about the end-of-input.
    if (!XML_Parse(static_cast<XML_Parser>(this->Parser), "", 0, 1))
    {
      this->ReportXmlParseError();
      result = 0;
    }
  }

  // Clean up the parser.
  XML_ParserFree(static_cast<XML_Parser>(this->Parser));
  this->Parser = nullptr;

  return result;
}

//----------------------------------------------------------------------------
int svtkXMLParser::ParseXML()
{
  // Parsing of message
  if (this->InputString)
  {
    if (this->InputStringLength >= 0)
    {
      return this->ParseBuffer(this->InputString, this->InputStringLength);
    }
    else
    {
      return this->ParseBuffer(this->InputString);
    }
  }

  // Make sure we have input.
  if (!this->Stream)
  {
    svtkErrorMacro("Parse() called with no Stream set.");
    return 0;
  }

  // Default stream parser just reads a block at a time.
  istream& in = *(this->Stream);
  const int bufferSize = 4096;
  char buffer[bufferSize];

  // Read in the stream and send its contents to the XML parser.  This
  // read loop is very sensitive on certain platforms with slightly
  // broken stream libraries (like HPUX).  Normally, it is incorrect
  // to not check the error condition on the fin.read() before using
  // the data, but the fin.gcount() will be zero if an error occurred.
  // Therefore, the loop should be safe everywhere.
  while (!this->ParseError && !this->ParsingComplete() && in)
  {
    in.read(buffer, bufferSize);
    if (in.gcount())
    {
      if (!this->ParseBuffer(buffer, in.gcount()))
      {
        return 0;
      }
    }
  }

  // Clear the fail and eof bits on the input stream so we can later
  // seek back to read data.
  this->Stream->clear(this->Stream->rdstate() & ~ios::eofbit);
  this->Stream->clear(this->Stream->rdstate() & ~ios::failbit);

  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLParser::ParsingComplete()
{
  // Default behavior is to parse to end of stream.
  return 0;
}

//----------------------------------------------------------------------------
void svtkXMLParser::StartElement(const char* name, const char** svtkNotUsed(atts))
{
  this->ReportUnknownElement(name);
}

//----------------------------------------------------------------------------
void svtkXMLParser::EndElement(const char* svtkNotUsed(name)) {}

//----------------------------------------------------------------------------
void svtkXMLParser::CharacterDataHandler(const char* svtkNotUsed(inData), int svtkNotUsed(inLength)) {}

//----------------------------------------------------------------------------
void svtkXMLParser::ReportStrayAttribute(const char* element, const char* attr, const char* value)
{
  svtkWarningMacro("Stray attribute in XML stream: Element " << element << " has " << attr << "=\""
                                                            << value << "\"");
}

//----------------------------------------------------------------------------
void svtkXMLParser::ReportMissingAttribute(const char* element, const char* attr)
{
  svtkErrorMacro("Missing attribute in XML stream: Element " << element << " is missing " << attr);
}

//----------------------------------------------------------------------------
void svtkXMLParser::ReportBadAttribute(const char* element, const char* attr, const char* value)
{
  svtkErrorMacro("Bad attribute value in XML stream: Element " << element << " has " << attr << "=\""
                                                              << value << "\"");
}

//----------------------------------------------------------------------------
void svtkXMLParser::ReportUnknownElement(const char* element)
{
  svtkErrorMacro("Unknown element in XML stream: " << element);
}

//----------------------------------------------------------------------------
void svtkXMLParser::ReportXmlParseError()
{
  svtkErrorMacro("Error parsing XML in stream at line "
    << XML_GetCurrentLineNumber(static_cast<XML_Parser>(this->Parser)) << ", column "
    << XML_GetCurrentColumnNumber(static_cast<XML_Parser>(this->Parser)) << ", byte index "
    << XML_GetCurrentByteIndex(static_cast<XML_Parser>(this->Parser)) << ": "
    << XML_ErrorString(XML_GetErrorCode(static_cast<XML_Parser>(this->Parser))));
}

//----------------------------------------------------------------------------
svtkTypeInt64 svtkXMLParser::GetXMLByteIndex()
{
  return XML_GetCurrentByteIndex(static_cast<XML_Parser>(this->Parser));
}

//----------------------------------------------------------------------------
int svtkXMLParser::ParseBuffer(const char* buffer, unsigned int count)
{
  // Pass the buffer to the expat XML parser.
  if (!XML_Parse(static_cast<XML_Parser>(this->Parser), buffer, count, 0))
  {
    this->ReportXmlParseError();
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLParser::ParseBuffer(const char* buffer)
{
  return this->ParseBuffer(buffer, static_cast<int>(strlen(buffer)));
}

//----------------------------------------------------------------------------
int svtkXMLParser::IsSpace(char c)
{
  return isspace(c);
}

//----------------------------------------------------------------------------
void svtkXMLParserStartElement(void* parser, const char* name, const char** atts)
{
  // Begin element handler that is registered with the XML_Parser.
  // This just casts the user data to a svtkXMLParser and calls
  // StartElement.
  static_cast<svtkXMLParser*>(parser)->StartElement(name, atts);
}

//----------------------------------------------------------------------------
void svtkXMLParserEndElement(void* parser, const char* name)
{
  // End element handler that is registered with the XML_Parser.  This
  // just casts the user data to a svtkXMLParser and calls EndElement.
  static_cast<svtkXMLParser*>(parser)->EndElement(name);
}
