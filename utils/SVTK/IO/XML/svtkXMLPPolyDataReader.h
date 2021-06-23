/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPPolyDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPPolyDataReader
 * @brief   Read PSVTK XML PolyData files.
 *
 * svtkXMLPPolyDataReader reads the PSVTK XML PolyData file format.
 * This reads the parallel format's summary file and then uses
 * svtkXMLPolyDataReader to read data from the individual PolyData
 * piece files.  Streaming is supported.  The standard extension for
 * this reader's file format is "pvtp".
 *
 * @sa
 * svtkXMLPolyDataReader
 */

#ifndef svtkXMLPPolyDataReader_h
#define svtkXMLPPolyDataReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLPUnstructuredDataReader.h"

class svtkPolyData;

class SVTKIOXML_EXPORT svtkXMLPPolyDataReader : public svtkXMLPUnstructuredDataReader
{
public:
  svtkTypeMacro(svtkXMLPPolyDataReader, svtkXMLPUnstructuredDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLPPolyDataReader* New();

  //@{
  /**
   * Get the reader's output.
   */
  svtkPolyData* GetOutput();
  svtkPolyData* GetOutput(int idx);
  //@}

protected:
  svtkXMLPPolyDataReader();
  ~svtkXMLPPolyDataReader() override;

  const char* GetDataSetName() override;
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel) override;
  svtkIdType GetNumberOfCellsInPiece(int piece) override;
  svtkIdType GetNumberOfVertsInPiece(int piece);
  svtkIdType GetNumberOfLinesInPiece(int piece);
  svtkIdType GetNumberOfStripsInPiece(int piece);
  svtkIdType GetNumberOfPolysInPiece(int piece);
  void SetupOutputTotals() override;

  void SetupOutputData() override;
  void SetupNextPiece() override;
  int ReadPieceData() override;

  void CopyArrayForCells(svtkDataArray* inArray, svtkDataArray* outArray) override;
  svtkXMLDataReader* CreatePieceReader() override;
  int FillOutputPortInformation(int, svtkInformation*) override;

  // The size of the UpdatePiece.
  svtkIdType TotalNumberOfVerts;
  svtkIdType TotalNumberOfLines;
  svtkIdType TotalNumberOfStrips;
  svtkIdType TotalNumberOfPolys;
  svtkIdType StartVert;
  svtkIdType StartLine;
  svtkIdType StartStrip;
  svtkIdType StartPoly;

private:
  svtkXMLPPolyDataReader(const svtkXMLPPolyDataReader&) = delete;
  void operator=(const svtkXMLPPolyDataReader&) = delete;
};

#endif
