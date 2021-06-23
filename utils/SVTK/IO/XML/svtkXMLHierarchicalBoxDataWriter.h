/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLHierarchicalBoxDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLHierarchicalBoxDataWriter
 * @brief   writer for svtkHierarchicalBoxDataSet
 * for backwards compatibility.
 *
 * svtkXMLHierarchicalBoxDataWriter is an empty subclass of
 * svtkXMLUniformGridAMRWriter for writing svtkUniformGridAMR datasets in
 * SVTK-XML format.
 */

#ifndef svtkXMLHierarchicalBoxDataWriter_h
#define svtkXMLHierarchicalBoxDataWriter_h

#include "svtkXMLUniformGridAMRWriter.h"

class SVTKIOXML_EXPORT svtkXMLHierarchicalBoxDataWriter : public svtkXMLUniformGridAMRWriter
{
public:
  static svtkXMLHierarchicalBoxDataWriter* New();
  svtkTypeMacro(svtkXMLHierarchicalBoxDataWriter, svtkXMLUniformGridAMRWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override { return "vth"; }

protected:
  svtkXMLHierarchicalBoxDataWriter();
  ~svtkXMLHierarchicalBoxDataWriter() override;

private:
  svtkXMLHierarchicalBoxDataWriter(const svtkXMLHierarchicalBoxDataWriter&) = delete;
  void operator=(const svtkXMLHierarchicalBoxDataWriter&) = delete;
};

#endif
