/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPUnstructuredGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPUnstructuredGridReader
 * @brief   Read PSVTK XML UnstructuredGrid files.
 *
 * svtkXMLPUnstructuredGridReader reads the PSVTK XML UnstructuredGrid
 * file format.  This reads the parallel format's summary file and
 * then uses svtkXMLUnstructuredGridReader to read data from the
 * individual UnstructuredGrid piece files.  Streaming is supported.
 * The standard extension for this reader's file format is "pvtu".
 *
 * @sa
 * svtkXMLUnstructuredGridReader
 */

#ifndef svtkXMLPUnstructuredGridReader_h
#define svtkXMLPUnstructuredGridReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLPUnstructuredDataReader.h"

class svtkUnstructuredGrid;

class SVTKIOXML_EXPORT svtkXMLPUnstructuredGridReader : public svtkXMLPUnstructuredDataReader
{
public:
  svtkTypeMacro(svtkXMLPUnstructuredGridReader, svtkXMLPUnstructuredDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLPUnstructuredGridReader* New();

  //@{
  /**
   * Get the reader's output.
   */
  svtkUnstructuredGrid* GetOutput();
  svtkUnstructuredGrid* GetOutput(int idx);
  //@}

protected:
  svtkXMLPUnstructuredGridReader();
  ~svtkXMLPUnstructuredGridReader() override;

  const char* GetDataSetName() override;
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel) override;
  void SetupOutputTotals() override;

  void SetupOutputData() override;
  void SetupNextPiece() override;
  int ReadPieceData() override;

  void CopyArrayForCells(svtkDataArray* inArray, svtkDataArray* outArray) override;
  svtkXMLDataReader* CreatePieceReader() override;
  int FillOutputPortInformation(int, svtkInformation*) override;

  void SqueezeOutputArrays(svtkDataObject*) override;

  // The index of the cell in the output where the current piece
  // begins.
  svtkIdType StartCell;

private:
  svtkXMLPUnstructuredGridReader(const svtkXMLPUnstructuredGridReader&) = delete;
  void operator=(const svtkXMLPUnstructuredGridReader&) = delete;
};

#endif
