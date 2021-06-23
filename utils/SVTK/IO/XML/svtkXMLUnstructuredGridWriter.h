/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLUnstructuredGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLUnstructuredGridWriter
 * @brief   Write SVTK XML UnstructuredGrid files.
 *
 * svtkXMLUnstructuredGridWriter writes the SVTK XML UnstructuredGrid
 * file format.  One unstructured grid input can be written into one
 * file in any number of streamed pieces (if supported by the rest of
 * the pipeline).  The standard extension for this writer's file
 * format is "vtu".  This writer is also used to write a single piece
 * of the parallel file format.
 *
 * @sa
 * svtkXMLPUnstructuredGridWriter
 */

#ifndef svtkXMLUnstructuredGridWriter_h
#define svtkXMLUnstructuredGridWriter_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLUnstructuredDataWriter.h"

class svtkUnstructuredGridBase;

class SVTKIOXML_EXPORT svtkXMLUnstructuredGridWriter : public svtkXMLUnstructuredDataWriter
{
public:
  svtkTypeMacro(svtkXMLUnstructuredGridWriter, svtkXMLUnstructuredDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLUnstructuredGridWriter* New();

  /**
   * Get/Set the writer's input.
   */
  svtkUnstructuredGridBase* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

protected:
  svtkXMLUnstructuredGridWriter();
  ~svtkXMLUnstructuredGridWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

  void AllocatePositionArrays() override;
  void DeletePositionArrays() override;

  const char* GetDataSetName() override;

  void WriteInlinePieceAttributes() override;
  void WriteInlinePiece(svtkIndent indent) override;

  void WriteAppendedPieceAttributes(int index) override;
  void WriteAppendedPiece(int index, svtkIndent indent) override;
  void WriteAppendedPieceData(int index) override;

  svtkIdType GetNumberOfInputCells() override;
  void CalculateSuperclassFraction(float* fractions);

  // Positions of attributes for each piece.
  svtkTypeInt64* NumberOfCellsPositions;
  OffsetsManagerArray* CellsOM; // one per piece

private:
  svtkXMLUnstructuredGridWriter(const svtkXMLUnstructuredGridWriter&) = delete;
  void operator=(const svtkXMLUnstructuredGridWriter&) = delete;
};

#endif
