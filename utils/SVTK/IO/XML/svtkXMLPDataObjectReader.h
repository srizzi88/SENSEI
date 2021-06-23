/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPDataObjectReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPDataObjectReader
 * @brief   Superclass for PSVTK XML file readers.
 *
 * svtkXMLPDataObjectReader provides functionality common to all PSVTK XML
 * file readers. Concrete subclasses call upon this functionality when needed.
 */

#ifndef svtkXMLPDataObjectReader_h
#define svtkXMLPDataObjectReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLReader.h"

class SVTKIOXML_EXPORT svtkXMLPDataObjectReader : public svtkXMLReader
{
public:
  svtkTypeMacro(svtkXMLPDataObjectReader, svtkXMLReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the number of pieces from the summary file being read.
   */
  svtkGetMacro(NumberOfPieces, int);

protected:
  svtkXMLPDataObjectReader();
  ~svtkXMLPDataObjectReader() override;

  /**
   * Delete all piece readers and related information
   */
  virtual void DestroyPieces();

  /**
   * Initialize the output data
   */
  void SetupOutputData() override;

  /**
   * Setup the number of pieces to be read and allocate space accordingly
   */
  virtual void SetupPieces(int numPieces);

  /**
   * Pipeline execute information driver.  Called by svtkXMLReader.
   */
  int ReadXMLInformation() override;

  /**
   * Whether or not the current reader can read the current piece
   */
  virtual int CanReadPiece(int index) = 0;

  /**
   * Setup the piece reader at the given index
   */
  int ReadPiece(svtkXMLDataElement* ePiece, int index);

  /**
   * Setup the current piece reader. It needs to be overridden by subclass.
   */
  virtual int ReadPiece(svtkXMLDataElement* ePiece) = 0;

  //@{
  /**
   * Methods for creating a filename for each piece in the dataset
   */
  char* CreatePieceFileName(const char* fileName);
  void SplitFileName();
  //@}

  //@{
  /**
   * Callback registered with the PieceProgressObserver.
   */
  static void PieceProgressCallbackFunction(svtkObject*, unsigned long, void*, void*);
  virtual void PieceProgressCallback() = 0;
  //@}

  /**
   * Pieces from the input summary file.
   */
  int NumberOfPieces;

  /**
   * The piece currently being read.
   */
  int Piece;

  /**
   * The path to the input file without the file name.
   */
  char* PathName;

  //@{
  /**
   * Information per-piece.
   */
  svtkXMLDataElement** PieceElements;
  int* CanReadPieceFlag;
  //@}

  svtkCallbackCommand* PieceProgressObserver;

private:
  svtkXMLPDataObjectReader(const svtkXMLPDataObjectReader&) = delete;
  void operator=(const svtkXMLPDataObjectReader&) = delete;
};

#endif
