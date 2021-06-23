/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLFileReadTester.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLFileReadTester
 * @brief   Utility class for svtkXMLReader and subclasses.
 *
 * svtkXMLFileReadTester reads the smallest part of a file necessary to
 * determine whether it is a SVTK XML file.  If so, it extracts the
 * file type and version number.
 */

#ifndef svtkXMLFileReadTester_h
#define svtkXMLFileReadTester_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLParser.h"

class SVTKIOXML_EXPORT svtkXMLFileReadTester : public svtkXMLParser
{
public:
  svtkTypeMacro(svtkXMLFileReadTester, svtkXMLParser);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLFileReadTester* New();

  /**
   * Try to read the file given by FileName.  Returns 1 if the file is
   * a SVTK XML file, and 0 otherwise.
   */
  int TestReadFile();

  //@{
  /**
   * Get the data type of the XML file tested.  If the file could not
   * be read, returns nullptr.
   */
  svtkGetStringMacro(FileDataType);
  //@}

  //@{
  /**
   * Get the file version of the XML file tested.  If the file could not
   * be read, returns nullptr.
   */
  svtkGetStringMacro(FileVersion);
  //@}

protected:
  svtkXMLFileReadTester();
  ~svtkXMLFileReadTester() override;

  void StartElement(const char* name, const char** atts) override;
  int ParsingComplete() override;
  void ReportStrayAttribute(const char*, const char*, const char*) override {}
  void ReportMissingAttribute(const char*, const char*) override {}
  void ReportBadAttribute(const char*, const char*, const char*) override {}
  void ReportUnknownElement(const char*) override {}
  void ReportXmlParseError() override {}

  char* FileDataType;
  char* FileVersion;
  int Done;

  svtkSetStringMacro(FileDataType);
  svtkSetStringMacro(FileVersion);

private:
  svtkXMLFileReadTester(const svtkXMLFileReadTester&) = delete;
  void operator=(const svtkXMLFileReadTester&) = delete;
};

#endif
