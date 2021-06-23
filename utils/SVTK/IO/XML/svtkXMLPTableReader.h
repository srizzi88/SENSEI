/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPTableReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPTableReader
 * @brief   Read PSVTK XML Table files.
 *
 * svtkXMLPTableReader reads the PSVTK XML Table
 * file format.  This reads the parallel format's summary file and
 * then uses svtkXMLTableReader to read data from the
 * individual Table piece files.  Streaming is supported.
 * The standard extension for this reader's file format is "pvtt".
 *
 * @sa
 * svtkXMLTableReader
 */

#ifndef svtkXMLPTableReader_h
#define svtkXMLPTableReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLPDataObjectReader.h"

class svtkTable;
class svtkXMLTableReader;

class SVTKIOXML_EXPORT svtkXMLPTableReader : public svtkXMLPDataObjectReader
{
public:
  svtkTypeMacro(svtkXMLPTableReader, svtkXMLPDataObjectReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLPTableReader* New();

  //@{
  /**
   * Get the reader's output.
   */
  svtkTable* GetOutput();
  svtkTable* GetOutput(int idx);
  //@}

  /**
   * For the specified port, copy the information this reader sets up in
   * SetupOutputInformation to outInfo
   */
  void CopyOutputInformation(svtkInformation* outInfo, int port) override;

  /**
   * Get the number of columns arrays available in the input.
   */
  int GetNumberOfColumnArrays();

  /**
   * Get the name of the column with the given index in
   * the input.
   */
  const char* GetColumnArrayName(int index);

  //@{
  /**
   * Get/Set whether the column array with the given name is to
   * be read.
   */
  int GetColumnArrayStatus(const char* name);
  void SetColumnArrayStatus(const char* name, int status);
  //@}

  /**
   * Get the data array selection tables used to configure which data
   * arrays are loaded by the reader.
   */
  svtkGetObjectMacro(ColumnSelection, svtkDataArraySelection);

protected:
  svtkXMLPTableReader();
  ~svtkXMLPTableReader() override;

  /**
   * Return the type of the dataset being read
   */
  const char* GetDataSetName() override;

  /**
   * Get the number of rows of the table
   */
  svtkIdType GetNumberOfRows();

  /**
   * Get the number of rows in the given piece.  Valid after
   * UpdateInformation.
   */
  virtual svtkIdType GetNumberOfRowsInPiece(int piece);

  svtkTable* GetOutputAsTable();

  svtkTable* GetPieceInputAsTable(int piece);

  /**
   * Get the current piece index and the total number of piece in the dataset
   */
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces);

  /**
   * Initialize current output
   */
  void SetupEmptyOutput() override;

  /**
   * Initialize current output data: allocate arrays for RowData
   */
  void SetupOutputData() override;

  /**
   * Setup the output's information.
   */
  void SetupOutputInformation(svtkInformation* outInfo) override;

  /**
   * Initialize the total number of rows to be read.
   */
  void SetupOutputTotals();

  /**
   * Initialize the index of the first row to be read in the next piece
   */
  void SetupNextPiece();

  /**
   * Setup the number of pieces to be read and allocate space accordingly
   */
  void SetupPieces(int numPieces) override;

  /**
   * Setup the extent for the parallel reader and the piece readers.
   */
  void SetupUpdateExtent(int piece, int numberOfPieces);

  /**
   * Setup the readers and then read the input data
   */
  void ReadXMLData() override;

  /**
   * Whether or not the current reader can read the current piece
   */
  int CanReadPiece(int index) override;

  /**
   * Pipeline execute data driver. Called by svtkXMLReader.
   */
  int ReadPrimaryElement(svtkXMLDataElement* ePrimary) override;

  /**
   * Delete all piece readers and related information
   */
  void DestroyPieces() override;

  using svtkXMLPDataObjectReader::ReadPiece;

  /**
   * Setup the current piece reader.
   */
  int ReadPiece(svtkXMLDataElement* ePiece) override;

  /**
   * Read piece at the given index RowData
   */
  int ReadPieceData(int index);

  /**
   * Actually read the current piece data
   */
  int ReadPieceData();

  /**
   * Create a reader according to the data to read
   */
  svtkXMLTableReader* CreatePieceReader();

  int FillOutputPortInformation(int, svtkInformation*) override;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Callback registered with the PieceProgressObserver.
   */
  void PieceProgressCallback() override;

  /**
   * Check whether the given array element is an enabled array.
   */
  int ColumnIsEnabled(svtkXMLDataElement* elementRowData);

  int GetNumberOfRowArrays();
  const char* GetRowArrayName(int index);

  /**
   * The update request.
   */
  int UpdatePieceId;
  int UpdateNumberOfPieces;

  /**
   * The range of pieces from the file that will form the UpdatePiece.
   */
  int StartPiece;
  int EndPiece;
  svtkIdType TotalNumberOfRows;
  svtkIdType StartRow;

  svtkXMLTableReader** PieceReaders;

  /**
   * The PRowData element representations.
   */
  svtkXMLDataElement* PRowElement;

  svtkDataArraySelection* ColumnSelection;

private:
  svtkXMLPTableReader(const svtkXMLPTableReader&) = delete;
  void operator=(const svtkXMLPTableReader&) = delete;
};

#endif
