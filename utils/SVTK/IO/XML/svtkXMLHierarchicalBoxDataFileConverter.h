/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLHierarchicalBoxDataFileConverter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLHierarchicalBoxDataFileConverter
 * @brief   converts older *.vth, *.vthb
 * files to newer format.
 *
 * svtkXMLHierarchicalBoxDataFileConverter is a utility class to convert v0.1 and
 * v1.0 of the SVTK XML hierarchical file format to the v1.1. Users can then use
 * svtkXMLUniformGridAMRReader to read the dataset into SVTK.
 */

#ifndef svtkXMLHierarchicalBoxDataFileConverter_h
#define svtkXMLHierarchicalBoxDataFileConverter_h

#include "svtkIOXMLModule.h" // needed for export macro.
#include "svtkObject.h"

class svtkXMLDataElement;

class SVTKIOXML_EXPORT svtkXMLHierarchicalBoxDataFileConverter : public svtkObject
{
public:
  static svtkXMLHierarchicalBoxDataFileConverter* New();
  svtkTypeMacro(svtkXMLHierarchicalBoxDataFileConverter, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the input filename.
   */
  svtkSetStringMacro(InputFileName);
  svtkGetStringMacro(InputFileName);
  //@}

  //@{
  /**
   * Set the output filename.
   */
  svtkSetStringMacro(OutputFileName);
  svtkGetStringMacro(OutputFileName);
  //@}

  /**
   * Converts the input file to new format and writes out the output file.
   */
  bool Convert();

protected:
  svtkXMLHierarchicalBoxDataFileConverter();
  ~svtkXMLHierarchicalBoxDataFileConverter() override;

  svtkXMLDataElement* ParseXML(const char* filename);

  // Returns GridDescription. SVTK_UNCHANGED for invalid/failure.
  int GetOriginAndSpacing(svtkXMLDataElement* ePrimary, double origin[3], double*& spacing);

  char* InputFileName;
  char* OutputFileName;
  char* FilePath;
  svtkSetStringMacro(FilePath);

private:
  svtkXMLHierarchicalBoxDataFileConverter(const svtkXMLHierarchicalBoxDataFileConverter&) = delete;
  void operator=(const svtkXMLHierarchicalBoxDataFileConverter&) = delete;
};

#endif
