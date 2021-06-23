/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLUnstructuredGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLUnstructuredGridReader
 * @brief   Read SVTK XML UnstructuredGrid files.
 *
 * svtkXMLUnstructuredGridReader reads the SVTK XML UnstructuredGrid
 * file format.  One unstructured grid file can be read to produce one
 * output.  Streaming is supported.  The standard extension for this
 * reader's file format is "vtu".  This reader is also used to read a
 * single piece of the parallel file format.
 *
 * @sa
 * svtkXMLPUnstructuredGridReader
 */

#ifndef svtkXMLUnstructuredGridReader_h
#define svtkXMLUnstructuredGridReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLUnstructuredDataReader.h"

class svtkUnstructuredGrid;
class svtkIdTypeArray;

class SVTKIOXML_EXPORT svtkXMLUnstructuredGridReader : public svtkXMLUnstructuredDataReader
{
public:
  svtkTypeMacro(svtkXMLUnstructuredGridReader, svtkXMLUnstructuredDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLUnstructuredGridReader* New();

  //@{
  /**
   * Get the reader's output.
   */
  svtkUnstructuredGrid* GetOutput();
  svtkUnstructuredGrid* GetOutput(int idx);
  //@}

protected:
  svtkXMLUnstructuredGridReader();
  ~svtkXMLUnstructuredGridReader() override;

  const char* GetDataSetName() override;
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel) override;
  void SetupOutputTotals() override;
  void SetupPieces(int numPieces) override;
  void DestroyPieces() override;

  void SetupOutputData() override;
  int ReadPiece(svtkXMLDataElement* ePiece) override;
  void SetupNextPiece() override;
  int ReadPieceData() override;

  // Read a data array whose tuples correspond to cells.
  int ReadArrayForCells(svtkXMLDataElement* da, svtkAbstractArray* outArray) override;

  // Get the number of cells in the given piece.  Valid after
  // UpdateInformation.
  svtkIdType GetNumberOfCellsInPiece(int piece) override;

  int FillOutputPortInformation(int, svtkInformation*) override;

  // The index of the cell in the output where the current piece
  // begins.
  svtkIdType StartCell;

  // The Cells element for each piece.
  svtkXMLDataElement** CellElements;
  svtkIdType* NumberOfCells;

  int CellsTimeStep;
  unsigned long CellsOffset;

private:
  svtkXMLUnstructuredGridReader(const svtkXMLUnstructuredGridReader&) = delete;
  void operator=(const svtkXMLUnstructuredGridReader&) = delete;
};

#endif
