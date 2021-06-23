/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLUniformGridAMRWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLUniformGridAMRWriter
 * @brief   writer for svtkUniformGridAMR.
 *
 * svtkXMLUniformGridAMRWriter is a svtkXMLCompositeDataWriter subclass to
 * handle svtkUniformGridAMR datasets (including svtkNonOverlappingAMR and
 * svtkOverlappingAMR).
 */

#ifndef svtkXMLUniformGridAMRWriter_h
#define svtkXMLUniformGridAMRWriter_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLCompositeDataWriter.h"

class SVTKIOXML_EXPORT svtkXMLUniformGridAMRWriter : public svtkXMLCompositeDataWriter
{
public:
  static svtkXMLUniformGridAMRWriter* New();
  svtkTypeMacro(svtkXMLUniformGridAMRWriter, svtkXMLCompositeDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override { return "vth"; }

protected:
  svtkXMLUniformGridAMRWriter();
  ~svtkXMLUniformGridAMRWriter() override;

  /**
   * Methods to define the file's major and minor version numbers.
   * VTH/VTHB version number 1.1 is used for overlapping/non-overlapping AMR
   * datasets.
   */
  int GetDataSetMajorVersion() override { return 1; }
  int GetDataSetMinorVersion() override { return 1; }

  int FillInputPortInformation(int port, svtkInformation* info) override;

  // Internal method called recursively to create the xml tree for the children
  // of compositeData.
  int WriteComposite(
    svtkCompositeDataSet* compositeData, svtkXMLDataElement* parent, int& writerIdx) override;

private:
  svtkXMLUniformGridAMRWriter(const svtkXMLUniformGridAMRWriter&) = delete;
  void operator=(const svtkXMLUniformGridAMRWriter&) = delete;
};

#endif
