/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLDataReader
 * @brief   Superclass for SVTK XML file readers.
 *
 * svtkXMLDataReader provides functionality common to all file readers for
 * <a href="http://www.svtk.org/Wiki/SVTK_XML_Formats">SVTK XML formats</a>.
 * Concrete subclasses call upon this functionality when needed.
 *
 * @sa
 * svtkXMLPDataReader
 */

#ifndef svtkXMLDataReader_h
#define svtkXMLDataReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLReader.h"

#include <memory> // for std::unique_ptr

class SVTKIOXML_EXPORT svtkXMLDataReader : public svtkXMLReader
{
public:
  svtkTypeMacro(svtkXMLDataReader, svtkXMLReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the number of points in the output.
   */
  virtual svtkIdType GetNumberOfPoints() = 0;

  /**
   * Get the number of cells in the output.
   */
  virtual svtkIdType GetNumberOfCells() = 0;

  // For the specified port, copy the information this reader sets up in
  // SetupOutputInformation to outInfo
  void CopyOutputInformation(svtkInformation* outInfo, int port) override;

protected:
  svtkXMLDataReader();
  ~svtkXMLDataReader() override;

  // Add functionality to methods from superclass.
  void CreateXMLParser() override;
  void DestroyXMLParser() override;
  void SetupOutputInformation(svtkInformation* outInfo) override;

  int ReadPrimaryElement(svtkXMLDataElement* ePrimary) override;
  void SetupOutputData() override;

  // Setup the reader for a given number of pieces.
  virtual void SetupPieces(int numPieces);
  virtual void DestroyPieces();

  // Read information from the file for the given piece.
  int ReadPiece(svtkXMLDataElement* ePiece, int piece);
  virtual int ReadPiece(svtkXMLDataElement* ePiece);

  // Read data from the file for the given piece.
  int ReadPieceData(int piece);
  virtual int ReadPieceData();

  void ReadXMLData() override;

  // Read a data array whose tuples coorrespond to points or cells.
  virtual int ReadArrayForPoints(svtkXMLDataElement* da, svtkAbstractArray* outArray);
  virtual int ReadArrayForCells(svtkXMLDataElement* da, svtkAbstractArray* outArray);

  // Callback registered with the DataProgressObserver.
  static void DataProgressCallbackFunction(svtkObject*, unsigned long, void*, void*);
  // Progress callback from XMLParser.
  virtual void DataProgressCallback();

  // The number of Pieces of data found in the file.
  int NumberOfPieces;

  // The PointData and CellData element representations for each piece.
  svtkXMLDataElement** PointDataElements;
  svtkXMLDataElement** CellDataElements;

  // The piece currently being read.
  int Piece;

  // The number of point/cell data arrays in the output.  Valid after
  // SetupOutputData has been called.
  int NumberOfPointArrays;
  int NumberOfCellArrays;

  // The observer to report progress from reading data from XMLParser.
  svtkCallbackCommand* DataProgressObserver;

private:
  class MapStringToInt;
  class MapStringToInt64;

  // Specify the last time step read, useful to know if we need to rearead data
  // //PointData
  std::unique_ptr<MapStringToInt> PointDataTimeStep;
  std::unique_ptr<MapStringToInt64> PointDataOffset;
  int PointDataNeedToReadTimeStep(svtkXMLDataElement* eNested);

  // CellData
  std::unique_ptr<MapStringToInt> CellDataTimeStep;
  std::unique_ptr<MapStringToInt64> CellDataOffset;
  int CellDataNeedToReadTimeStep(svtkXMLDataElement* eNested);

  svtkXMLDataReader(const svtkXMLDataReader&) = delete;
  void operator=(const svtkXMLDataReader&) = delete;

  void ConvertGhostLevelsToGhostType(
    FieldType type, svtkAbstractArray* data, svtkIdType startIndex, svtkIdType numValues) override;
};

#endif
