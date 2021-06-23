/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLSDynaSummaryParser.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLSDynaSummaryParser
 *
 * This is a helper class used by svtkLSDynaReader to read XML files.
 * @sa
 * svtkLSDynaReader
 */

#ifndef svtkLSDynaSummaryParser_h
#define svtkLSDynaSummaryParser_h

#include "svtkIOLSDynaModule.h" // For export macro
#include "svtkStdString.h"      //needed for svtkStdString
#include "svtkXMLParser.h"

class LSDynaMetaData;
class SVTKIOLSDYNA_EXPORT svtkLSDynaSummaryParser : public svtkXMLParser
{
public:
  svtkTypeMacro(svtkLSDynaSummaryParser, svtkXMLParser);
  static svtkLSDynaSummaryParser* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /// Must be set before calling Parse();
  LSDynaMetaData* MetaData;

protected:
  svtkLSDynaSummaryParser();
  ~svtkLSDynaSummaryParser() override {}

  void StartElement(const char* name, const char** atts) override;
  void EndElement(const char* name) override;
  void CharacterDataHandler(const char* data, int length) override;

  svtkStdString PartName;
  int PartId;
  int PartStatus;
  int PartMaterial;
  int InPart;
  int InDyna;
  int InName;

private:
  svtkLSDynaSummaryParser(const svtkLSDynaSummaryParser&) = delete;
  void operator=(const svtkLSDynaSummaryParser&) = delete;
};

#endif // svtkLSDynaReader_h
