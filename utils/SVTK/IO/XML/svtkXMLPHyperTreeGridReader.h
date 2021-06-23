/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPHyperTreeGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPHyperTreeGridReader
 * @brief   Read PSVTK XML HyperTreeGrid files.
 *
 * svtkXMLPHyperTreeGridReader reads the PSVTK XML HyperTreeGrid file format.
 * This reader uses svtkXMLHyperTreeGridReader to read data from the
 * individual HyperTreeGrid piece files.  Streaming is supported.
 * The standard extension for this reader's file format is "phtg".
 *
 * @sa
 * svtkXMLHyperTreeGridReader
 */

#ifndef svtkXMLPHyperTreeGridReader_h
#define svtkXMLPHyperTreeGridReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLPDataObjectReader.h"

class svtkHyperTreeCursor;
class svtkHyperTreeGrid;
class svtkHyperTreeGridNonOrientedCursor;
class svtkXMLHyperTreeGridReader;

class SVTKIOXML_EXPORT svtkXMLPHyperTreeGridReader : public svtkXMLPDataObjectReader
{
public:
  svtkTypeMacro(svtkXMLPHyperTreeGridReader, svtkXMLPDataObjectReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLPHyperTreeGridReader* New();

  //@{
  /**
   * Get the reader's output.
   */
  svtkHyperTreeGrid* GetOutput();
  svtkHyperTreeGrid* GetOutput(int idx);
  //@}

  /**
   * For the specified port, copy the information this reader sets up in
   * SetupOutputInformation to outInfo
   */
  void CopyOutputInformation(svtkInformation* outInfo, int port) override;

protected:
  svtkXMLPHyperTreeGridReader();
  ~svtkXMLPHyperTreeGridReader() override;

  /**
   * Return the type of the dataset being read
   */
  const char* GetDataSetName() override;

  /**
   * Get the number of vertices available in the input.
   */
  svtkIdType GetNumberOfPoints();
  svtkIdType GetNumberOfPointsInPiece(int piece);

  svtkHyperTreeGrid* GetOutputAsHyperTreeGrid();
  svtkHyperTreeGrid* GetPieceInputAsHyperTreeGrid(int piece);

  /**
   * Get the current piece index and the total number of pieces in the dataset
   * Here let's consider a piece to be one hypertreegrid file
   */
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces);

  /**
   * Initialize current output
   */
  void SetupEmptyOutput() override;

  /**
   * Initialize current output data
   */
  void SetupOutputData() override;

  /**
   * Setup the output's information.
   */
  void SetupOutputInformation(svtkInformation* outInfo) override;

  /**
   * Initialize the number of vertices from all the pieces
   */
  void SetupOutputTotals();

  /**
   * no-op
   */
  void SetupNextPiece();

  /**
   * Setup the number of pieces to be read
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
   * Actually read the current piece data
   */
  int ReadPieceData(int index);
  int ReadPieceData();
  void RecursivelyProcessTree(
    svtkHyperTreeGridNonOrientedCursor* inCursor, svtkHyperTreeGridNonOrientedCursor* outCursor);

  /**
   * Create a reader according to the data to read
   */
  svtkXMLHyperTreeGridReader* CreatePieceReader();

  int FillOutputPortInformation(int, svtkInformation*) override;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Callback registered with the PieceProgressObserver.
   */
  void PieceProgressCallback() override;

  /**
   * The update request.
   */
  int UpdatePiece;
  int UpdateNumberOfPieces;

  /**
   * The range of pieces from the file that will form the UpdatePiece.
   */
  int StartPiece;
  int EndPiece;

  svtkIdType TotalNumberOfPoints;
  svtkIdType PieceStartIndex;

  svtkXMLHyperTreeGridReader** PieceReaders;

private:
  svtkXMLPHyperTreeGridReader(const svtkXMLPHyperTreeGridReader&) = delete;
  void operator=(const svtkXMLPHyperTreeGridReader&) = delete;
};

#endif
