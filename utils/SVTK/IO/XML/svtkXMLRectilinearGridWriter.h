/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLRectilinearGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLRectilinearGridWriter
 * @brief   Write SVTK XML RectilinearGrid files.
 *
 * svtkXMLRectilinearGridWriter writes the SVTK XML RectilinearGrid
 * file format.  One rectilinear grid input can be written into one
 * file in any number of streamed pieces.  The standard extension for
 * this writer's file format is "vtr".  This writer is also used to
 * write a single piece of the parallel file format.
 *
 * @sa
 * svtkXMLPRectilinearGridWriter
 */

#ifndef svtkXMLRectilinearGridWriter_h
#define svtkXMLRectilinearGridWriter_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLStructuredDataWriter.h"

class svtkRectilinearGrid;

class SVTKIOXML_EXPORT svtkXMLRectilinearGridWriter : public svtkXMLStructuredDataWriter
{
public:
  static svtkXMLRectilinearGridWriter* New();
  svtkTypeMacro(svtkXMLRectilinearGridWriter, svtkXMLStructuredDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get/Set the writer's input.
   */
  svtkRectilinearGrid* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

protected:
  svtkXMLRectilinearGridWriter();
  ~svtkXMLRectilinearGridWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

  int WriteAppendedMode(svtkIndent indent);
  void WriteAppendedPiece(int index, svtkIndent indent) override;
  void WriteAppendedPieceData(int index) override;
  void WriteInlinePiece(svtkIndent indent) override;
  void GetInputExtent(int* extent) override;
  const char* GetDataSetName() override;
  void CalculateSuperclassFraction(float* fractions);

  // Coordinate array appended data positions.
  OffsetsManagerArray* CoordinateOM;

  void AllocatePositionArrays() override;
  void DeletePositionArrays() override;

private:
  svtkXMLRectilinearGridWriter(const svtkXMLRectilinearGridWriter&) = delete;
  void operator=(const svtkXMLRectilinearGridWriter&) = delete;
};

#endif
