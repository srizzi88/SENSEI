/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLParser.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLParser
 * @brief   Parse XML to handle element tags and attributes.
 *
 * svtkXMLParser reads a stream and parses XML element tags and corresponding
 * attributes.  Each element begin tag and its attributes are sent to
 * the StartElement method.  Each element end tag is sent to the
 * EndElement method.  Subclasses should replace these methods to actually
 * use the tags.
 */

#ifndef svtkXMLParser_h
#define svtkXMLParser_h

#include "svtkIOXMLParserModule.h" // For export macro
#include "svtkObject.h"

extern "C"
{
  void svtkXMLParserStartElement(void*, const char*, const char**);
  void svtkXMLParserEndElement(void*, const char*);
  void svtkXMLParserCharacterDataHandler(void*, const char*, int);
}

class SVTKIOXMLPARSER_EXPORT svtkXMLParser : public svtkObject
{
public:
  svtkTypeMacro(svtkXMLParser, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkXMLParser* New();

  //@{
  /**
   * Get/Set the input stream.
   */
  svtkSetMacro(Stream, istream*);
  svtkGetMacro(Stream, istream*);
  //@}

  //@{
  /**
   * Used by subclasses and their supporting classes.  These methods
   * wrap around the tellg and seekg methods of the input stream to
   * work-around stream bugs on various platforms.
   */
  svtkTypeInt64 TellG();
  void SeekG(svtkTypeInt64 position);
  //@}

  /**
   * Parse the XML input.
   */
  virtual int Parse();

  //@{
  /**
   * Parse the XML message. If length is specified, parse only the
   * first "length" characters
   */
  virtual int Parse(const char* inputString);
  virtual int Parse(const char* inputString, unsigned int length);
  //@}

  //@{
  /**
   * When parsing fragments of XML, or when streaming XML,
   * use the following three methods:
   * - InitializeParser() initializes the parser
   * but does not perform any actual parsing.
   * - ParseChunk() parses a fragment of XML;
   * this has to match to what was already parsed.
   * - CleanupParser() finishes parsing;
   * if there were errors, it will report them.
   */
  virtual int InitializeParser();
  virtual int ParseChunk(const char* inputString, unsigned int length);
  virtual int CleanupParser();
  //@}

  //@{
  /**
   * Set and get file name.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * If this is off (the default), CharacterDataHandler will be called to
   * process text within XML Elements. If this is on, the text will be
   * ignored.
   */
  svtkSetMacro(IgnoreCharacterData, int);
  svtkGetMacro(IgnoreCharacterData, int);
  //@}

  //@{
  /**
   * Set and get the encoding the parser should expect (nullptr defaults to
   * Expat's own default encoder, i.e UTF-8).
   * This should be set before parsing (i.e. a call to Parse()) or
   * even initializing the parser (i.e. a call to InitializeParser())
   */
  svtkSetStringMacro(Encoding);
  svtkGetStringMacro(Encoding);
  //@}

protected:
  svtkXMLParser();
  ~svtkXMLParser() override;

  // Input stream.  Set by user.
  istream* Stream;

  // File name to parse
  char* FileName;

  // Encoding
  char* Encoding;

  // This variable is true if there was a parse error while parsing in
  // chunks.
  int ParseError;

  // Character message to parse
  const char* InputString;
  int InputStringLength;

  // Expat parser structure.  Exists only during call to Parse().
  void* Parser;

  // Create/Allocate the internal parser (can be overridden by subclasses).
  virtual int CreateParser();

  // Called by Parse() to read the stream and call ParseBuffer.  Can
  // be replaced by subclasses to change how input is read.
  virtual int ParseXML();

  // Called before each block of input is read from the stream to
  // check if parsing is complete.  Can be replaced by subclasses to
  // change the terminating condition for parsing.  Parsing always
  // stops when the end of file is reached in the stream.
  virtual int ParsingComplete();

  // Called when a new element is opened in the XML source.  Should be
  // replaced by subclasses to handle each element.
  //  name = Name of new element.
  //  atts = Null-terminated array of attribute name/value pairs.
  //         Even indices are attribute names, and odd indices are values.
  virtual void StartElement(const char* name, const char** atts);

  // Called at the end of an element in the XML source opened when
  // StartElement was called.
  virtual void EndElement(const char* name);

  // Called when there is character data to handle.
  virtual void CharacterDataHandler(const char* data, int length);

  // Called by begin handlers to report any stray attribute values.
  virtual void ReportStrayAttribute(const char* element, const char* attr, const char* value);

  // Called by begin handlers to report any missing attribute values.
  virtual void ReportMissingAttribute(const char* element, const char* attr);

  // Called by begin handlers to report bad attribute values.
  virtual void ReportBadAttribute(const char* element, const char* attr, const char* value);

  // Called by StartElement to report unknown element type.
  virtual void ReportUnknownElement(const char* element);

  // Called by Parse to report an XML syntax error.
  virtual void ReportXmlParseError();

  // Get the current byte index from the beginning of the XML stream.
  svtkTypeInt64 GetXMLByteIndex();

  // Send the given buffer to the XML parser.
  virtual int ParseBuffer(const char* buffer, unsigned int count);

  // Send the given c-style string to the XML parser.
  int ParseBuffer(const char* buffer);

  // Utility for convenience of subclasses.  Wraps isspace C library
  // routine.
  static int IsSpace(char c);

  friend void svtkXMLParserStartElement(void*, const char*, const char**);
  friend void svtkXMLParserEndElement(void*, const char*);
  friend void svtkXMLParserCharacterDataHandler(void*, const char*, int);

  int IgnoreCharacterData;

private:
  svtkXMLParser(const svtkXMLParser&) = delete;
  void operator=(const svtkXMLParser&) = delete;
};

//----------------------------------------------------------------------------
inline void svtkXMLParserCharacterDataHandler(void* parser, const char* data, int length)
{
  // Character data handler that is registered with the XML_Parser.
  // This just casts the user data to a svtkXMLParser and calls
  // CharacterDataHandler.
  static_cast<svtkXMLParser*>(parser)->CharacterDataHandler(data, length);
}

#endif
