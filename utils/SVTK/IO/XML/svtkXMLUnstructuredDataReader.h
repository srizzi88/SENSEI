/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLUnstructuredDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLUnstructuredDataReader
 * @brief   Superclass for unstructured data XML readers.
 *
 * svtkXMLUnstructuredDataReader provides functionality common to all
 * unstructured data format readers.
 *
 * @sa
 * svtkXMLPolyDataReader svtkXMLUnstructuredGridReader
 */

#ifndef svtkXMLUnstructuredDataReader_h
#define svtkXMLUnstructuredDataReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLDataReader.h"

class svtkCellArray;
class svtkIdTypeArray;
class svtkPointSet;
class svtkUnsignedCharArray;

class SVTKIOXML_EXPORT svtkXMLUnstructuredDataReader : public svtkXMLDataReader
{
public:
  svtkTypeMacro(svtkXMLUnstructuredDataReader, svtkXMLDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the number of points in the output.
   */
  svtkIdType GetNumberOfPoints() override;

  /**
   * Get the number of cells in the output.
   */
  svtkIdType GetNumberOfCells() override;

  /**
   * Get the number of pieces in the file
   */
  virtual svtkIdType GetNumberOfPieces();

  /**
   * Setup the reader as if the given update extent were requested by
   * its output.  This can be used after an UpdateInformation to
   * validate GetNumberOfPoints() and GetNumberOfCells() without
   * actually reading data.
   */
  void SetupUpdateExtent(int piece, int numberOfPieces, int ghostLevel);

  // For the specified port, copy the information this reader sets up in
  // SetupOutputInformation to outInfo
  void CopyOutputInformation(svtkInformation* outInfo, int port) override;

protected:
  svtkXMLUnstructuredDataReader();
  ~svtkXMLUnstructuredDataReader() override;

  svtkPointSet* GetOutputAsPointSet();
  svtkXMLDataElement* FindDataArrayWithName(svtkXMLDataElement* eParent, const char* name);

  // note that these decref the input array and return an array with a
  // new reference:
  svtkIdTypeArray* ConvertToIdTypeArray(svtkDataArray* a);
  svtkUnsignedCharArray* ConvertToUnsignedCharArray(svtkDataArray* a);

  // Pipeline execute data driver.  Called by svtkXMLReader.
  void ReadXMLData() override;

  void SetupEmptyOutput() override;
  virtual void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel) = 0;
  virtual void SetupOutputTotals();
  virtual void SetupNextPiece();
  void SetupPieces(int numPieces) override;
  void DestroyPieces() override;

  // Setup the output's information.
  void SetupOutputInformation(svtkInformation* outInfo) override;

  void SetupOutputData() override;
  int ReadPiece(svtkXMLDataElement* ePiece) override;
  int ReadPieceData() override;
  int ReadCellArray(svtkIdType numberOfCells, svtkIdType totalNumberOfCells,
    svtkXMLDataElement* eCells, svtkCellArray* outCells);

  // Read faces and faceoffsets arrays for unstructured grid with polyhedon cells
  int ReadFaceArray(svtkIdType numberOfCells, svtkXMLDataElement* eCells, svtkIdTypeArray* outFaces,
    svtkIdTypeArray* outFaceOffsets);

  // Read a data array whose tuples coorrespond to points.
  int ReadArrayForPoints(svtkXMLDataElement* da, svtkAbstractArray* outArray) override;

  // Get the number of points/cells in the given piece.  Valid after
  // UpdateInformation.
  virtual svtkIdType GetNumberOfPointsInPiece(int piece);
  virtual svtkIdType GetNumberOfCellsInPiece(int piece) = 0;

  // The update request.
  int UpdatePieceId;
  int UpdateNumberOfPieces;
  int UpdateGhostLevel;

  // The range of pieces from the file that will form the UpdatePiece.
  int StartPiece;
  int EndPiece;
  svtkIdType TotalNumberOfPoints;
  svtkIdType TotalNumberOfCells;
  svtkIdType StartPoint;

  // The Points element for each piece.
  svtkXMLDataElement** PointElements;
  svtkIdType* NumberOfPoints;

  int PointsTimeStep;
  unsigned long PointsOffset;
  int PointsNeedToReadTimeStep(svtkXMLDataElement* eNested);
  int CellsNeedToReadTimeStep(
    svtkXMLDataElement* eNested, int& cellstimestep, unsigned long& cellsoffset);

private:
  svtkXMLUnstructuredDataReader(const svtkXMLUnstructuredDataReader&) = delete;
  void operator=(const svtkXMLUnstructuredDataReader&) = delete;
};

#endif
