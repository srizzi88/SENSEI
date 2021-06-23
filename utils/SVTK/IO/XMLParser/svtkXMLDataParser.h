/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLDataParser.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLDataParser
 * @brief   Used by svtkXMLReader to parse SVTK XML files.
 *
 * svtkXMLDataParser provides a subclass of svtkXMLParser that
 * constructs a representation of an XML data format's file using
 * svtkXMLDataElement to represent each XML element.  This
 * representation is then used by svtkXMLReader and its subclasses to
 * traverse the structure of the file and extract data.
 *
 * @sa
 * svtkXMLDataElement
 */

#ifndef svtkXMLDataParser_h
#define svtkXMLDataParser_h

#include "svtkIOXMLParserModule.h" // For export macro
#include "svtkXMLDataElement.h"    //For inline definition.
#include "svtkXMLParser.h"

class svtkInputStream;
class svtkDataCompressor;

class SVTKIOXMLPARSER_EXPORT svtkXMLDataParser : public svtkXMLParser
{
public:
  svtkTypeMacro(svtkXMLDataParser, svtkXMLParser);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLDataParser* New();

  /**
   * Get the root element from the XML document.
   */
  svtkXMLDataElement* GetRootElement();

  /**
   * Enumerate big and little endian byte order settings.
   */
  enum
  {
    BigEndian,
    LittleEndian
  };

  /**
   * Read inline data from inside the given element.  Returns the
   * number of words read.
   */
  size_t ReadInlineData(svtkXMLDataElement* element, int isAscii, void* buffer,
    svtkTypeUInt64 startWord, size_t numWords, int wordType);
  size_t ReadInlineData(
    svtkXMLDataElement* element, int isAscii, char* buffer, svtkTypeUInt64 startWord, size_t numWords)
  {
    return this->ReadInlineData(element, isAscii, buffer, startWord, numWords, SVTK_CHAR);
  }

  /**
   * Read from an appended data section starting at the given appended
   * data offset.  Returns the number of words read.
   */
  size_t ReadAppendedData(
    svtkTypeInt64 offset, void* buffer, svtkTypeUInt64 startWord, size_t numWords, int wordType);
  size_t ReadAppendedData(
    svtkTypeInt64 offset, char* buffer, svtkTypeUInt64 startWord, size_t numWords)
  {
    return this->ReadAppendedData(offset, buffer, startWord, numWords, SVTK_CHAR);
  }

  /**
   * Read from an ascii data section starting at the current position in
   * the stream.  Returns the number of words read.
   */
  size_t ReadAsciiData(void* buffer, svtkTypeUInt64 startWord, size_t numWords, int wordType);

  /**
   * Read from a data section starting at the current position in the
   * stream.  Returns the number of words read.
   */
  size_t ReadBinaryData(void* buffer, svtkTypeUInt64 startWord, size_t maxWords, int wordType);

  //@{
  /**
   * Get/Set the compressor used to decompress binary and appended data
   * after reading from the file.
   */
  virtual void SetCompressor(svtkDataCompressor*);
  svtkGetObjectMacro(Compressor, svtkDataCompressor);
  //@}

  /**
   * Get the size of a word of the given type.
   */
  size_t GetWordTypeSize(int wordType);

  /**
   * Parse the XML input and check that the file is safe to read.
   * Returns 1 for okay, 0 for error.
   */
  int Parse() override;

  //@{
  /**
   * Get/Set flag to abort reading of data.  This may be set by a
   * progress event observer.
   */
  svtkGetMacro(Abort, int);
  svtkSetMacro(Abort, int);
  //@}

  //@{
  /**
   * Get/Set progress of reading data.  This may be checked by a
   * progress event observer.
   */
  svtkGetMacro(Progress, float);
  svtkSetMacro(Progress, float);
  //@}

  //@{
  /**
   * Get/Set the character encoding that will be used to set the attributes's
   * encoding type of each svtkXMLDataElement created by this parser (i.e.,
   * the data element attributes will use that encoding internally).
   * If set to SVTK_ENCODING_NONE (default), the attribute encoding type will
   * not be changed and will default to the svtkXMLDataElement default encoding
   * type (see svtkXMLDataElement::AttributeEncoding).
   */
  svtkSetClampMacro(AttributesEncoding, int, SVTK_ENCODING_NONE, SVTK_ENCODING_UNKNOWN);
  svtkGetMacro(AttributesEncoding, int);
  //@}

  /**
   * If you need the text inside XMLElements, turn IgnoreCharacterData off.
   * This method will then be called when the file is parsed, and the text
   * will be stored in each XMLDataElement. SVTK XML Readers store the
   * information elsewhere, so the default is to ignore it.
   */
  void CharacterDataHandler(const char* data, int length) override;

  /**
   * Returns the byte index of where appended data starts (if the
   * file is using appended data). Valid after the XML is parsed.
   */
  svtkTypeInt64 GetAppendedDataPosition() { return this->AppendedDataPosition; }

protected:
  svtkXMLDataParser();
  ~svtkXMLDataParser() override;

  // This parser does not support parsing from a string.
  int Parse(const char*) override;
  int Parse(const char*, unsigned int) override;

  // Implement parsing methods.
  void StartElement(const char* name, const char** atts) override;
  void EndElement(const char*) override;

  int ParsingComplete() override;
  int CheckPrimaryAttributes();
  void FindAppendedDataPosition();
  int ParseBuffer(const char* buffer, unsigned int count) override;

  void AddElement(svtkXMLDataElement* element);
  void PushOpenElement(svtkXMLDataElement* element);
  svtkXMLDataElement* PopOpenElement();
  void FreeAllElements();
  void PerformByteSwap(void* data, size_t numWords, size_t wordSize);

  // Data reading methods.
  int ReadCompressionHeader();
  size_t FindBlockSize(svtkTypeUInt64 block);
  int ReadBlock(svtkTypeUInt64 block, unsigned char* buffer);
  unsigned char* ReadBlock(svtkTypeUInt64 block);
  size_t ReadUncompressedData(
    unsigned char* data, svtkTypeUInt64 startWord, size_t numWords, size_t wordSize);
  size_t ReadCompressedData(
    unsigned char* data, svtkTypeUInt64 startWord, size_t numWords, size_t wordSize);

  // Go to the start of the inline data
  void SeekInlineDataPosition(svtkXMLDataElement* element);

  // Ascii data reading methods.
  int ParseAsciiData(int wordType);
  void FreeAsciiBuffer();

  // Progress update methods.
  void UpdateProgress(float progress);

  // The root XML element.
  svtkXMLDataElement* RootElement;

  // The stack of elements currently being parsed.
  svtkXMLDataElement** OpenElements;
  unsigned int NumberOfOpenElements;
  unsigned int OpenElementsSize;

  // The position of the appended data section, if found.
  svtkTypeInt64 AppendedDataPosition;

  // How much of the string "<AppendedData" has been matched in input.
  int AppendedDataMatched;

  // The byte order of the binary input.
  int ByteOrder;

  // The word type of binary input headers.
  int HeaderType;

  // The input stream used to read data.  Set by ReadAppendedData and
  // ReadInlineData methods.
  svtkInputStream* DataStream;

  // The input stream used to read inline data.  May transparently
  // decode the data.
  svtkInputStream* InlineDataStream;

  // The stream to use for appended data.
  svtkInputStream* AppendedDataStream;

  // Decompression data.
  svtkDataCompressor* Compressor;
  size_t NumberOfBlocks;
  size_t BlockUncompressedSize;
  size_t PartialLastBlockUncompressedSize;
  size_t* BlockCompressedSizes;
  svtkTypeInt64* BlockStartOffsets;

  // Ascii data parsing.
  unsigned char* AsciiDataBuffer;
  size_t AsciiDataBufferLength;
  int AsciiDataWordType;
  svtkTypeInt64 AsciiDataPosition;

  // Progress during reading of data.
  float Progress;

  // Abort flag checked during reading of data.
  int Abort;

  int AttributesEncoding;

private:
  svtkXMLDataParser(const svtkXMLDataParser&) = delete;
  void operator=(const svtkXMLDataParser&) = delete;
};

//----------------------------------------------------------------------------
inline void svtkXMLDataParser::CharacterDataHandler(const char* data, int length)
{
  const unsigned int eid = this->NumberOfOpenElements - 1;
  this->OpenElements[eid]->AddCharacterData(data, length);
}

#endif
