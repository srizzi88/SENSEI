/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLTableWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLTableWriter
 * @brief   Write SVTK XML Table files.
 *
 * svtkXMLTableWriter provides a functionality for writing vtTable as
 * XML .vtt files.
 */

#ifndef svtkXMLTableWriter_h
#define svtkXMLTableWriter_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLWriter.h"

class svtkTable;

class SVTKIOXML_EXPORT svtkXMLTableWriter : public svtkXMLWriter
{
public:
  svtkTypeMacro(svtkXMLTableWriter, svtkXMLWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLTableWriter* New();

  //@{
  /**
   * Get/Set the number of pieces used to stream the table through the
   * pipeline while writing to the file.
   */
  svtkSetMacro(NumberOfPieces, int);
  svtkGetMacro(NumberOfPieces, int);
  //@}

  //@{
  /**
   * Get/Set the piece to write to the file.  If this is
   * negative or equal to the NumberOfPieces, all pieces will be written.
   */
  svtkSetMacro(WritePiece, int);
  svtkGetMacro(WritePiece, int);
  //@}

  /**
   * See the svtkAlgorithm for a description of what these do
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkXMLTableWriter();
  ~svtkXMLTableWriter() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkTable* GetInputAsTable();
  const char* GetDataSetName() override; // svtkTable isn't a DataSet but it's used by svtkXMLWriter

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

  void SetInputUpdateExtent(int piece, int numPieces);

  int WriteHeader();
  int WriteAPiece();
  int WriteFooter();

  void AllocatePositionArrays();
  void DeletePositionArrays();

  int WriteInlineMode(svtkIndent indent);
  void WriteInlinePieceAttributes();
  void WriteInlinePiece(svtkIndent indent);

  void WriteAppendedPieceAttributes(int index);
  void WriteAppendedPiece(int index, svtkIndent indent);
  void WriteAppendedPieceData(int index);

  void WriteRowDataAppended(
    svtkDataSetAttributes* ds, svtkIndent indent, OffsetsManagerGroup* dsManager);

  void WriteRowDataAppendedData(
    svtkDataSetAttributes* ds, int timestep, OffsetsManagerGroup* pdManager);

  void WriteRowDataInline(svtkDataSetAttributes* ds, svtkIndent indent);

  /**
   * Number of pieces used for streaming.
   */
  int NumberOfPieces;

  /**
   * Which piece to write, if not all.
   */
  int WritePiece;

  /**
   * Positions of attributes for each piece.
   */
  svtkTypeInt64* NumberOfColsPositions;
  svtkTypeInt64* NumberOfRowsPositions;

  /**
   * For TimeStep support
   */
  OffsetsManagerArray* RowsOM;

  int CurrentPiece;

private:
  svtkXMLTableWriter(const svtkXMLTableWriter&) = delete;
  void operator=(const svtkXMLTableWriter&) = delete;
};

#endif
