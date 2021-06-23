/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPUnstructuredDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPUnstructuredDataReader
 * @brief   Superclass for parallel unstructured data XML readers.
 *
 * svtkXMLPUnstructuredDataReader provides functionality common to all
 * parallel unstructured data format readers.
 *
 * @sa
 * svtkXMLPPolyDataReader svtkXMLPUnstructuredGridReader
 */

#ifndef svtkXMLPUnstructuredDataReader_h
#define svtkXMLPUnstructuredDataReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLPDataReader.h"

class svtkPointSet;
class svtkCellArray;
class svtkXMLUnstructuredDataReader;

class SVTKIOXML_EXPORT svtkXMLPUnstructuredDataReader : public svtkXMLPDataReader
{
public:
  svtkTypeMacro(svtkXMLPUnstructuredDataReader, svtkXMLPDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // For the specified port, copy the information this reader sets up in
  // SetupOutputInformation to outInfo
  void CopyOutputInformation(svtkInformation* outInfo, int port) override;

protected:
  svtkXMLPUnstructuredDataReader();
  ~svtkXMLPUnstructuredDataReader() override;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  svtkPointSet* GetOutputAsPointSet();
  svtkPointSet* GetPieceInputAsPointSet(int piece);
  virtual void SetupOutputTotals();
  virtual void SetupNextPiece();
  svtkIdType GetNumberOfPoints() override;
  svtkIdType GetNumberOfCells() override;
  void CopyArrayForPoints(svtkDataArray* inArray, svtkDataArray* outArray) override;

  void SetupEmptyOutput() override;

  // Setup the output's information.
  void SetupOutputInformation(svtkInformation* outInfo) override;

  void SetupOutputData() override;
  virtual void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel) = 0;

  // Pipeline execute data driver.  Called by svtkXMLReader.
  void ReadXMLData() override;
  int ReadPrimaryElement(svtkXMLDataElement* ePrimary) override;
  void SetupUpdateExtent(int piece, int numberOfPieces, int ghostLevel);

  int ReadPieceData() override;
  void CopyCellArray(svtkIdType totalNumberOfCells, svtkCellArray* inCells, svtkCellArray* outCells);

  // Get the number of points/cells in the given piece.  Valid after
  // UpdateInformation.
  virtual svtkIdType GetNumberOfPointsInPiece(int piece);
  virtual svtkIdType GetNumberOfCellsInPiece(int piece);

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

  // The PPoints element with point information.
  svtkXMLDataElement* PPointsElement;

private:
  svtkXMLPUnstructuredDataReader(const svtkXMLPUnstructuredDataReader&) = delete;
  void operator=(const svtkXMLPUnstructuredDataReader&) = delete;
};

#endif
