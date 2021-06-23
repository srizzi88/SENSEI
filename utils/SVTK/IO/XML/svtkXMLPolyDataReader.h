/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPolyDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPolyDataReader
 * @brief   Read SVTK XML PolyData files.
 *
 * svtkXMLPolyDataReader reads the SVTK XML PolyData file format.  One
 * polygonal data file can be read to produce one output.  Streaming
 * is supported.  The standard extension for this reader's file format
 * is "vtp".  This reader is also used to read a single piece of the
 * parallel file format.
 *
 * @sa
 * svtkXMLPPolyDataReader
 */

#ifndef svtkXMLPolyDataReader_h
#define svtkXMLPolyDataReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLUnstructuredDataReader.h"

class svtkPolyData;

class SVTKIOXML_EXPORT svtkXMLPolyDataReader : public svtkXMLUnstructuredDataReader
{
public:
  svtkTypeMacro(svtkXMLPolyDataReader, svtkXMLUnstructuredDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLPolyDataReader* New();

  //@{
  /**
   * Get the reader's output.
   */
  svtkPolyData* GetOutput();
  svtkPolyData* GetOutput(int idx);
  //@}

  //@{
  /**
   * Get the number of verts/lines/strips/polys in the output.
   */
  virtual svtkIdType GetNumberOfVerts();
  virtual svtkIdType GetNumberOfLines();
  virtual svtkIdType GetNumberOfStrips();
  virtual svtkIdType GetNumberOfPolys();
  //@}

protected:
  svtkXMLPolyDataReader();
  ~svtkXMLPolyDataReader() override;

  const char* GetDataSetName() override;
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel) override;
  void SetupOutputTotals() override;
  void SetupNextPiece() override;
  void SetupPieces(int numPieces) override;
  void DestroyPieces() override;

  void SetupOutputData() override;
  int ReadPiece(svtkXMLDataElement* ePiece) override;
  int ReadPieceData() override;

  // Read a data array whose tuples coorrespond to cells.
  int ReadArrayForCells(svtkXMLDataElement* da, svtkAbstractArray* outArray) override;

  // Get the number of cells in the given piece.  Valid after
  // UpdateInformation.
  svtkIdType GetNumberOfCellsInPiece(int piece) override;

  int FillOutputPortInformation(int, svtkInformation*) override;

  // The size of the UpdatePiece.
  int TotalNumberOfVerts;
  int TotalNumberOfLines;
  int TotalNumberOfStrips;
  int TotalNumberOfPolys;
  svtkIdType StartVert;
  svtkIdType StartLine;
  svtkIdType StartStrip;
  svtkIdType StartPoly;

  // The cell elements for each piece.
  svtkXMLDataElement** VertElements;
  svtkXMLDataElement** LineElements;
  svtkXMLDataElement** StripElements;
  svtkXMLDataElement** PolyElements;
  svtkIdType* NumberOfVerts;
  svtkIdType* NumberOfLines;
  svtkIdType* NumberOfStrips;
  svtkIdType* NumberOfPolys;

  // For TimeStep support
  int VertsTimeStep;
  unsigned long VertsOffset;
  int LinesTimeStep;
  unsigned long LinesOffset;
  int StripsTimeStep;
  unsigned long StripsOffset;
  int PolysTimeStep;
  unsigned long PolysOffset;

private:
  svtkXMLPolyDataReader(const svtkXMLPolyDataReader&) = delete;
  void operator=(const svtkXMLPolyDataReader&) = delete;
};

#endif
