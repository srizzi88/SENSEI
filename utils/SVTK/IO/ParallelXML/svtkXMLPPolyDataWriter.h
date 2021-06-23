/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPPolyDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPPolyDataWriter
 * @brief   Write PSVTK XML PolyData files.
 *
 * svtkXMLPPolyDataWriter writes the PSVTK XML PolyData file format.
 * One poly data input can be written into a parallel file format with
 * any number of pieces spread across files.  The standard extension
 * for this writer's file format is "pvtp".  This writer uses
 * svtkXMLPolyDataWriter to write the individual piece files.
 *
 * @sa
 * svtkXMLPolyDataWriter
 */

#ifndef svtkXMLPPolyDataWriter_h
#define svtkXMLPPolyDataWriter_h

#include "svtkIOParallelXMLModule.h" // For export macro
#include "svtkXMLPUnstructuredDataWriter.h"

class svtkPolyData;

class SVTKIOPARALLELXML_EXPORT svtkXMLPPolyDataWriter : public svtkXMLPUnstructuredDataWriter
{
public:
  static svtkXMLPPolyDataWriter* New();
  svtkTypeMacro(svtkXMLPPolyDataWriter, svtkXMLPUnstructuredDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get/Set the writer's input.
   */
  svtkPolyData* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

protected:
  svtkXMLPPolyDataWriter();
  ~svtkXMLPPolyDataWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

  const char* GetDataSetName() override;
  svtkXMLUnstructuredDataWriter* CreateUnstructuredPieceWriter() override;

private:
  svtkXMLPPolyDataWriter(const svtkXMLPPolyDataWriter&) = delete;
  void operator=(const svtkXMLPPolyDataWriter&) = delete;
};

#endif
