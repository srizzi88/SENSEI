/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPDataReader
 * @brief   Superclass for PSVTK XML file readers that read svtkDataSets.
 *
 * svtkXMLPDataReader provides functionality common to all PSVTK XML
 * file readers that read svtkDataSets. Concrete subclasses call upon
 * this functionality when needed.
 *
 * @sa
 * svtkXMLDataReader
 */

#ifndef svtkXMLPDataReader_h
#define svtkXMLPDataReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLPDataObjectReader.h"

class svtkDataArray;
class svtkDataSet;
class svtkXMLDataReader;

class SVTKIOXML_EXPORT svtkXMLPDataReader : public svtkXMLPDataObjectReader
{
public:
  svtkTypeMacro(svtkXMLPDataReader, svtkXMLPDataObjectReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * For the specified port, copy the information this reader sets up in
   * SetupOutputInformation to outInfo
   */
  void CopyOutputInformation(svtkInformation* outInfo, int port) override;

protected:
  svtkXMLPDataReader();
  ~svtkXMLPDataReader() override;

  // Re-use any superclass signatures that we don't override.
  using svtkXMLPDataObjectReader::ReadPiece;

  /**
   * Delete all piece readers and related information
   */
  void DestroyPieces() override;

  virtual svtkIdType GetNumberOfPoints() = 0;

  virtual svtkIdType GetNumberOfCells() = 0;

  /**
   * Get a given piece input as a dataset, return nullptr if there is none.
   */
  svtkDataSet* GetPieceInputAsDataSet(int piece);

  /**
   * Initialize the output data
   */
  void SetupOutputData() override;

  /**
   * Pipeline execute information driver.  Called by svtkXMLReader.
   */
  void SetupOutputInformation(svtkInformation* outInfo) override;

  /**
   * Setup the number of pieces to be read and allocate space accordingly
   */
  void SetupPieces(int numPieces) override;

  /**
   * Whether or not the current reader can read the current piece
   */
  int CanReadPiece(int index) override;

  /**
   * Create a reader according to the data to read. It needs to be overridden by subclass.
   */
  virtual svtkXMLDataReader* CreatePieceReader() = 0;

  /**
   * Setup the current piece reader
   */
  int ReadPiece(svtkXMLDataElement* ePiece) override;

  /**
   * Actually read the piece at the given index data
   */
  int ReadPieceData(int index);

  /**
   * Actually read the current piece data
   */
  virtual int ReadPieceData();

  /**
   * Read the information relative to the dataset and allocate the needed structures according to it
   */
  int ReadPrimaryElement(svtkXMLDataElement* ePrimary) override;

  virtual void CopyArrayForPoints(svtkDataArray* inArray, svtkDataArray* outArray) = 0;
  virtual void CopyArrayForCells(svtkDataArray* inArray, svtkDataArray* outArray) = 0;

  /**
   * Callback registered with the PieceProgressObserver.
   */
  void PieceProgressCallback() override;

  /**
   * The ghost level available on each input piece.
   */
  int GhostLevel;

  /**
   * Information per-piece.
   */
  svtkXMLDataReader** PieceReaders;

  /**
   * The PPointData and PCellData element representations.
   */
  svtkXMLDataElement* PPointDataElement;
  svtkXMLDataElement* PCellDataElement;

private:
  svtkXMLPDataReader(const svtkXMLPDataReader&) = delete;
  void operator=(const svtkXMLPDataReader&) = delete;
};

#endif
