/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLUnstructuredDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLUnstructuredDataWriter
 * @brief   Superclass for SVTK XML unstructured data writers.
 *
 * svtkXMLUnstructuredDataWriter provides SVTK XML writing functionality
 * that is common among all the unstructured data formats.
 */

#ifndef svtkXMLUnstructuredDataWriter_h
#define svtkXMLUnstructuredDataWriter_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLWriter.h"

#include <svtkSmartPointer.h> // for svtkSmartPointer

class svtkPointSet;
class svtkCellArray;
class svtkCellIterator;
class svtkDataArray;
class svtkIdTypeArray;
class svtkUnstructuredGrid;

class SVTKIOXML_EXPORT svtkXMLUnstructuredDataWriter : public svtkXMLWriter
{
public:
  svtkTypeMacro(svtkXMLUnstructuredDataWriter, svtkXMLWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the number of pieces used to stream the image through the
   * pipeline while writing to the file.
   */
  svtkSetMacro(NumberOfPieces, int);
  svtkGetMacro(NumberOfPieces, int);
  //@}

  //@{
  /**
   * Get/Set the piece to write to the file.  If this is
   * negative or equal to the NumberOfPieces, all pieces will be written.
   */
  svtkSetMacro(WritePiece, int);
  svtkGetMacro(WritePiece, int);
  //@}

  //@{
  /**
   * Get/Set the ghost level used to pad each piece.
   */
  svtkSetMacro(GhostLevel, int);
  svtkGetMacro(GhostLevel, int);
  //@}

  // See the svtkAlgorithm for a description of what these do
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkXMLUnstructuredDataWriter();
  ~svtkXMLUnstructuredDataWriter() override;

  svtkPointSet* GetInputAsPointSet();
  const char* GetDataSetName() override = 0;
  virtual void SetInputUpdateExtent(int piece, int numPieces, int ghostLevel);

  virtual int WriteHeader();
  virtual int WriteAPiece();
  virtual int WriteFooter();

  virtual void AllocatePositionArrays();
  virtual void DeletePositionArrays();

  virtual int WriteInlineMode(svtkIndent indent);
  virtual void WriteInlinePieceAttributes();
  virtual void WriteInlinePiece(svtkIndent indent);

  virtual void WriteAppendedPieceAttributes(int index);
  virtual void WriteAppendedPiece(int index, svtkIndent indent);
  virtual void WriteAppendedPieceData(int index);

  void WriteCellsInline(const char* name, svtkCellIterator* cellIter, svtkIdType numCells,
    svtkIdType cellSizeEstimate, svtkIndent indent);

  void WriteCellsInline(
    const char* name, svtkCellArray* cells, svtkDataArray* types, svtkIndent indent);

  // New API with face infomration for polyhedron cell support.
  void WriteCellsInline(const char* name, svtkCellArray* cells, svtkDataArray* types,
    svtkIdTypeArray* faces, svtkIdTypeArray* faceOffsets, svtkIndent indent);

  void WriteCellsInlineWorker(const char* name, svtkDataArray* types, svtkIndent indent);

  void WriteCellsAppended(
    const char* name, svtkDataArray* types, svtkIndent indent, OffsetsManagerGroup* cellsManager);

  void WriteCellsAppended(const char* name, svtkDataArray* types, svtkIdTypeArray* faces,
    svtkIdTypeArray* faceOffsets, svtkIndent indent, OffsetsManagerGroup* cellsManager);

  void WriteCellsAppended(const char* name, svtkCellIterator* cellIter, svtkIdType numCells,
    svtkIndent indent, OffsetsManagerGroup* cellsManager);

  void WriteCellsAppendedData(
    svtkCellArray* cells, svtkDataArray* types, int timestep, OffsetsManagerGroup* cellsManager);

  void WriteCellsAppendedData(svtkCellIterator* cellIter, svtkIdType numCells,
    svtkIdType cellSizeEstimate, int timestep, OffsetsManagerGroup* cellsManager);

  // New API with face infomration for polyhedron cell support.
  void WriteCellsAppendedData(svtkCellArray* cells, svtkDataArray* types, svtkIdTypeArray* faces,
    svtkIdTypeArray* faceOffsets, int timestep, OffsetsManagerGroup* cellsManager);

  void WriteCellsAppendedDataWorker(
    svtkDataArray* types, int timestep, OffsetsManagerGroup* cellsManager);

  void ConvertCells(svtkCellIterator* cellIter, svtkIdType numCells, svtkIdType cellSizeEstimate);

  void ConvertCells(svtkCellArray* cells);

  // For polyhedron support, conversion results are stored in Faces and FaceOffsets
  void ConvertFaces(svtkIdTypeArray* faces, svtkIdTypeArray* faceOffsets);

  // Get the number of points/cells.  Valid after Update has been
  // invoked on the input.
  virtual svtkIdType GetNumberOfInputPoints();
  virtual svtkIdType GetNumberOfInputCells() = 0;
  void CalculateDataFractions(float* fractions);
  void CalculateCellFractions(float* fractions, svtkIdType typesSize);

  // Number of pieces used for streaming.
  int NumberOfPieces;

  // Which piece to write, if not all.
  int WritePiece;

  // The ghost level on each piece.
  int GhostLevel;

  // Positions of attributes for each piece.
  svtkTypeInt64* NumberOfPointsPositions;

  // For TimeStep support
  OffsetsManagerGroup* PointsOM;
  OffsetsManagerArray* PointDataOM;
  OffsetsManagerArray* CellDataOM;

  // Hold the new cell representation arrays while writing a piece.
  svtkSmartPointer<svtkDataArray> CellPoints;
  svtkSmartPointer<svtkDataArray> CellOffsets;

  int CurrentPiece;

  // Hold the face arrays for polyhedron cells.
  svtkIdTypeArray* Faces;
  svtkIdTypeArray* FaceOffsets;

private:
  svtkXMLUnstructuredDataWriter(const svtkXMLUnstructuredDataWriter&) = delete;
  void operator=(const svtkXMLUnstructuredDataWriter&) = delete;
};

#endif
