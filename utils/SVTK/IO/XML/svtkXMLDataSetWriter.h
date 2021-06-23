/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLDataSetWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLDataSetWriter
 * @brief   Write any type of SVTK XML file.
 *
 * svtkXMLDataSetWriter is a wrapper around the SVTK XML file format
 * writers.  Given an input svtkDataSet, the correct writer is
 * automatically selected based on the type of input.
 *
 * @sa
 * svtkXMLImageDataWriter svtkXMLStructuredGridWriter
 * svtkXMLRectilinearGridWriter svtkXMLPolyDataWriter
 * svtkXMLUnstructuredGridWriter
 */

#ifndef svtkXMLDataSetWriter_h
#define svtkXMLDataSetWriter_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLDataObjectWriter.h"

class svtkCallbackCommand;

class SVTKIOXML_EXPORT svtkXMLDataSetWriter : public svtkXMLDataObjectWriter
{
public:
  svtkTypeMacro(svtkXMLDataSetWriter, svtkXMLDataObjectWriter);
  static svtkXMLDataSetWriter* New();

protected:
  svtkXMLDataSetWriter();
  ~svtkXMLDataSetWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkXMLDataSetWriter(const svtkXMLDataSetWriter&) = delete;
  void operator=(const svtkXMLDataSetWriter&) = delete;
};

#endif
