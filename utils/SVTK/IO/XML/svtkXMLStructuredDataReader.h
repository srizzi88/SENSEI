/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLStructuredDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLStructuredDataReader
 * @brief   Superclass for structured data XML readers.
 *
 * svtkXMLStructuredDataReader provides functionality common to all
 * structured data format readers.
 *
 * @sa
 * svtkXMLImageDataReader svtkXMLStructuredGridReader
 * svtkXMLRectilinearGridReader
 */

#ifndef svtkXMLStructuredDataReader_h
#define svtkXMLStructuredDataReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLDataReader.h"

class SVTKIOXML_EXPORT svtkXMLStructuredDataReader : public svtkXMLDataReader
{
public:
  svtkTypeMacro(svtkXMLStructuredDataReader, svtkXMLDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the number of points in the output.
   */
  svtkIdType GetNumberOfPoints() override;

  /**
   * Get the number of cells in the output.
   */
  svtkIdType GetNumberOfCells() override;

  //@{
  /**
   * Get/Set whether the reader gets a whole slice from disk when only
   * a rectangle inside it is needed.  This mode reads more data than
   * necessary, but prevents many short reads from interacting poorly
   * with the compression and encoding schemes.
   */
  svtkSetMacro(WholeSlices, svtkTypeBool);
  svtkGetMacro(WholeSlices, svtkTypeBool);
  svtkBooleanMacro(WholeSlices, svtkTypeBool);
  //@}

  /**
   * For the specified port, copy the information this reader sets up in
   * SetupOutputInformation to outInfo
   */
  void CopyOutputInformation(svtkInformation* outInfo, int port) override;

protected:
  svtkXMLStructuredDataReader();
  ~svtkXMLStructuredDataReader() override;

  virtual void SetOutputExtent(int* extent) = 0;
  int ReadPrimaryElement(svtkXMLDataElement* ePrimary) override;

  // Pipeline execute data driver.  Called by svtkXMLReader.
  void ReadXMLData() override;

  void SetupOutputInformation(svtkInformation* outInfo) override;

  // Internal representation of pieces in the file that may have come
  // from a streamed write.
  int* PieceExtents;
  int* PiecePointDimensions;
  svtkIdType* PiecePointIncrements;
  int* PieceCellDimensions;
  svtkIdType* PieceCellIncrements;

  // Whether to read in whole slices mode.
  svtkTypeBool WholeSlices;

  // The update extent and corresponding increments and dimensions.
  int UpdateExtent[6];
  int PointDimensions[3];
  int CellDimensions[3];
  svtkIdType PointIncrements[3];
  svtkIdType CellIncrements[3];

  int WholeExtent[6];

  // The extent currently being read.
  int SubExtent[6];
  int SubPointDimensions[3];
  int SubCellDimensions[3];

  // Override methods from superclass.
  void SetupEmptyOutput() override;
  void SetupPieces(int numPieces) override;
  void DestroyPieces() override;
  int ReadArrayForPoints(svtkXMLDataElement* da, svtkAbstractArray* outArray) override;
  int ReadArrayForCells(svtkXMLDataElement* da, svtkAbstractArray* outArray) override;

  // Internal utility methods.
  int ReadPiece(svtkXMLDataElement* ePiece) override;
  virtual int ReadSubExtent(int* inExtent, int* inDimensions, svtkIdType* inIncrements,
    int* outExtent, int* outDimensions, svtkIdType* outIncrements, int* subExtent,
    int* subDimensions, svtkXMLDataElement* da, svtkAbstractArray* array, FieldType type);

private:
  svtkXMLStructuredDataReader(const svtkXMLStructuredDataReader&) = delete;
  void operator=(const svtkXMLStructuredDataReader&) = delete;
};

#endif
