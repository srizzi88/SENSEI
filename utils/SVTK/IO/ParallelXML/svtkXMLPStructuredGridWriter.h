/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPStructuredGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPStructuredGridWriter
 * @brief   Write PSVTK XML StructuredGrid files.
 *
 * svtkXMLPStructuredGridWriter writes the PSVTK XML StructuredGrid
 * file format.  One structured grid input can be written into a
 * parallel file format with any number of pieces spread across files.
 * The standard extension for this writer's file format is "pvts".
 * This writer uses svtkXMLStructuredGridWriter to write the individual
 * piece files.
 *
 * @sa
 * svtkXMLStructuredGridWriter
 */

#ifndef svtkXMLPStructuredGridWriter_h
#define svtkXMLPStructuredGridWriter_h

#include "svtkIOParallelXMLModule.h" // For export macro
#include "svtkXMLPStructuredDataWriter.h"

class svtkStructuredGrid;

class SVTKIOPARALLELXML_EXPORT svtkXMLPStructuredGridWriter : public svtkXMLPStructuredDataWriter
{
public:
  static svtkXMLPStructuredGridWriter* New();
  svtkTypeMacro(svtkXMLPStructuredGridWriter, svtkXMLPStructuredDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get/Set the writer's input.
   */
  svtkStructuredGrid* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

protected:
  svtkXMLPStructuredGridWriter();
  ~svtkXMLPStructuredGridWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

  const char* GetDataSetName() override;
  svtkXMLStructuredDataWriter* CreateStructuredPieceWriter() override;
  void WritePData(svtkIndent indent) override;

private:
  svtkXMLPStructuredGridWriter(const svtkXMLPStructuredGridWriter&) = delete;
  void operator=(const svtkXMLPStructuredGridWriter&) = delete;
};

#endif
