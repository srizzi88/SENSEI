/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPStructuredDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPStructuredDataReader
 * @brief   Superclass for parallel structured data XML readers.
 *
 * svtkXMLPStructuredDataReader provides functionality common to all
 * parallel structured data format readers.
 *
 * @sa
 * svtkXMLPImageDataReader svtkXMLPStructuredGridReader
 * svtkXMLPRectilinearGridReader
 */

#ifndef svtkXMLPStructuredDataReader_h
#define svtkXMLPStructuredDataReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLPDataReader.h"

class svtkExtentSplitter;
class svtkXMLStructuredDataReader;

class SVTKIOXML_EXPORT svtkXMLPStructuredDataReader : public svtkXMLPDataReader
{
public:
  svtkTypeMacro(svtkXMLPStructuredDataReader, svtkXMLPDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // For the specified port, copy the information this reader sets up in
  // SetupOutputInformation to outInfo
  void CopyOutputInformation(svtkInformation* outInfo, int port) override;

protected:
  svtkXMLPStructuredDataReader();
  ~svtkXMLPStructuredDataReader() override;

  svtkIdType GetNumberOfPoints() override;
  svtkIdType GetNumberOfCells() override;
  void CopyArrayForPoints(svtkDataArray* inArray, svtkDataArray* outArray) override;
  void CopyArrayForCells(svtkDataArray* inArray, svtkDataArray* outArray) override;

  virtual void SetOutputExtent(int* extent) = 0;
  virtual void GetPieceInputExtent(int index, int* extent) = 0;

  // Pipeline execute data driver.  Called by svtkXMLReader.
  void ReadXMLData() override;
  int ReadPrimaryElement(svtkXMLDataElement* ePrimary) override;

  void SetupOutputData() override;

  void SetupPieces(int numPieces) override;
  void DestroyPieces() override;
  int ReadPiece(svtkXMLDataElement* ePiece) override;
  int ReadPieceData() override;
  void CopySubExtent(int* inExtent, int* inDimensions, svtkIdType* inIncrements, int* outExtent,
    int* outDimensions, svtkIdType* outIncrements, int* subExtent, int* subDimensions,
    svtkDataArray* inArray, svtkDataArray* outArray);
  int ComputePieceSubExtents();

  svtkExtentSplitter* ExtentSplitter;

  // The extent to be updated in the output.
  int UpdateExtent[6];
  int PointDimensions[3];
  svtkIdType PointIncrements[3];
  int CellDimensions[3];
  svtkIdType CellIncrements[3];

  // The extent currently being read from a piece.
  int SubExtent[6];
  int SubPointDimensions[3];
  int SubCellDimensions[3];
  int SubPieceExtent[6];
  int SubPiecePointDimensions[3];
  svtkIdType SubPiecePointIncrements[3];
  int SubPieceCellDimensions[3];
  svtkIdType SubPieceCellIncrements[3];

  // Information per-piece.
  int* PieceExtents;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkXMLPStructuredDataReader(const svtkXMLPStructuredDataReader&) = delete;
  void operator=(const svtkXMLPStructuredDataReader&) = delete;
};

#endif
