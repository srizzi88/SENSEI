/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLStructuredGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLStructuredGridWriter
 * @brief   Write SVTK XML StructuredGrid files.
 *
 * svtkXMLStructuredGridWriter writes the SVTK XML StructuredGrid file
 * format.  One structured grid input can be written into one file in
 * any number of streamed pieces.  The standard extension for this
 * writer's file format is "vts".  This writer is also used to write a
 * single piece of the parallel file format.
 *
 * @sa
 * svtkXMLPStructuredGridWriter
 */

#ifndef svtkXMLStructuredGridWriter_h
#define svtkXMLStructuredGridWriter_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLStructuredDataWriter.h"

class svtkStructuredGrid;

class SVTKIOXML_EXPORT svtkXMLStructuredGridWriter : public svtkXMLStructuredDataWriter
{
public:
  static svtkXMLStructuredGridWriter* New();
  svtkTypeMacro(svtkXMLStructuredGridWriter, svtkXMLStructuredDataWriter);
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
  svtkXMLStructuredGridWriter();
  ~svtkXMLStructuredGridWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

  void WriteAppendedPiece(int index, svtkIndent indent) override;
  void WriteAppendedPieceData(int index) override;
  void WriteInlinePiece(svtkIndent indent) override;
  void GetInputExtent(int* extent) override;
  const char* GetDataSetName() override;
  void CalculateSuperclassFraction(float* fractions);

  // The position of the appended data offset attribute for the points
  // array.
  OffsetsManagerGroup* PointsOM; // one per piece

  void AllocatePositionArrays() override;
  void DeletePositionArrays() override;

private:
  svtkXMLStructuredGridWriter(const svtkXMLStructuredGridWriter&) = delete;
  void operator=(const svtkXMLStructuredGridWriter&) = delete;
};

#endif
