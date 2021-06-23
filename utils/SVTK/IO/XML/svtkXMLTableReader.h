/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLTableReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLTableReader
 * @brief   Read SVTK XML Table files.
 *
 * svtkXMLTableReader provides a functionality for reading .vtt files as
 * svtkTable
 *
 */

#ifndef svtkXMLTableReader_h
#define svtkXMLTableReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLReader.h"

#include <map> // needed for std::map

class svtkCellArray;
class svtkIdTypeArray;
class svtkUnsignedCharArray;
class svtkTable;

class SVTKIOXML_EXPORT svtkXMLTableReader : public svtkXMLReader
{
public:
  svtkTypeMacro(svtkXMLTableReader, svtkXMLReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLTableReader* New();

  //@{
  /**
   * Get the reader's output.
   */
  svtkTable* GetOutput();
  svtkTable* GetOutput(int idx);
  //@}

  /**
   * Get the number of rows in the output.
   */
  svtkIdType GetNumberOfRows();

  /**
   * Get the number of pieces in the file
   */
  svtkIdType GetNumberOfPieces();

  /**
   * Setup the reader as if the given update extent were requested by
   * its output.  This can be used after an UpdateInformation to
   * validate GetNumberOfPoints() and GetNumberOfCells() without
   * actually reading data.
   */
  void SetupUpdateExtent(int piece, int numberOfPieces);

  /**
   * For the specified port, copy the information this reader sets up in
   * SetupOutputInformation to outInfo
   */
  void CopyOutputInformation(svtkInformation* outInfo, int port) override;

protected:
  svtkXMLTableReader();
  ~svtkXMLTableReader() override;

  /**
   * Check whether the given array element is an enabled array.
   */
  int ColumnIsEnabled(svtkXMLDataElement* eRowData);

  void DestroyPieces();

  /**
   * Get the name of the data set being read.
   */
  const char* GetDataSetName() override;

  /**
   * Get the current piece index and the total number of piece in the dataset
   */
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces);

  /**
   * Specify the last time step read, useful to know if we need to read data
   */
  int RowDataNeedToReadTimeStep(svtkXMLDataElement* eNested);

  /**
   * Initialize current output
   */
  void SetupEmptyOutput() override;

  /**
   * Initialize the total number of rows to be read.
   */
  void SetupOutputTotals();

  /**
   * Initialize the index of the first row to be read in the next piece
   */
  void SetupNextPiece();

  /**
   * Initialize current output data: allocate arrays for RowData
   */
  void SetupOutputData() override;

  /**
   * Setup the output's information.
   */
  void SetupOutputInformation(svtkInformation* outInfo) override;

  /**
   * Setup the number of pieces to be read and allocate space accordingly
   */
  void SetupPieces(int numPieces);

  /**
   * Pipeline execute data driver.  Called by svtkXMLReader.
   */
  void ReadXMLData() override;

  /**
   * Pipeline execute data driver. Called by svtkXMLReader.
   */
  int ReadPrimaryElement(svtkXMLDataElement* ePrimary) override;

  /**
   * Setup the piece reader at the given index.
   */
  int ReadPiece(svtkXMLDataElement* ePiece, int piece);

  /**
   * Setup the current piece reader.
   */
  int ReadPiece(svtkXMLDataElement* ePiece);

  /**
   * Actually read the current piece data
   */
  int ReadPieceData(int);

  int FillOutputPortInformation(int, svtkInformation*) override;

  /**
   * The update request.
   */
  int UpdatedPiece;
  int UpdateNumberOfPieces;
  /**
   * The range of pieces from the file that will form the UpdatedPiece.
   */
  int StartPiece;
  int EndPiece;
  svtkIdType TotalNumberOfRows;
  svtkIdType StartPoint;

  /**
   * The Points element for each piece.
   */
  svtkXMLDataElement** RowElements;
  svtkIdType* NumberOfRows;

  /**
   * The number of Pieces of data found in the file.
   */
  int NumberOfPieces;

  /**
   * The piece currently being read.
   */
  int Piece;

  /**
   * The RowData element representations for each piece.
   */
  svtkXMLDataElement** RowDataElements;

  /**
   * The number of columns arrays in the output. Valid after
   * SetupOutputData has been called.
   */
  int NumberOfColumns;

private:
  std::map<std::string, int> RowDataTimeStep;
  std::map<std::string, svtkTypeInt64> RowDataOffset;

  svtkXMLTableReader(const svtkXMLTableReader&) = delete;
  void operator=(const svtkXMLTableReader&) = delete;
};

#endif
