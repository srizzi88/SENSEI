/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLStructuredDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLStructuredDataWriter
 * @brief   Superclass for SVTK XML structured data writers.
 *
 * svtkXMLStructuredDataWriter provides SVTK XML writing functionality that
 * is common among all the structured data formats.
 */

#ifndef svtkXMLStructuredDataWriter_h
#define svtkXMLStructuredDataWriter_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLWriter.h"

class svtkAbstractArray;
class svtkInformation;
class svtkInformationVector;

class SVTKIOXML_EXPORT svtkXMLStructuredDataWriter : public svtkXMLWriter
{
public:
  svtkTypeMacro(svtkXMLStructuredDataWriter, svtkXMLWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the number of pieces used to stream the image through the
   * pipeline while writing to the file.
   */
  svtkSetMacro(NumberOfPieces, int);
  svtkGetMacro(NumberOfPieces, int);
  //@}

  //@{
  /**
   * Get/Set the piece to write to the file.  If this is
   * negative, all pieces will be written.
   */
  svtkSetMacro(WritePiece, int);
  svtkGetMacro(WritePiece, int);
  //@}

  //@{
  /**
   * Get/Set the ghost level used to pad each piece.
   */
  svtkSetMacro(GhostLevel, int);
  svtkGetMacro(GhostLevel, int);
  //@}

  //@{
  /**
   * Get/Set the extent of the input that should be treated as the
   * WholeExtent in the output file.  The default is the WholeExtent
   * of the input.
   */
  svtkSetVector6Macro(WriteExtent, int);
  svtkGetVector6Macro(WriteExtent, int);
  //@}

protected:
  svtkXMLStructuredDataWriter();
  ~svtkXMLStructuredDataWriter() override;

  // Writing drivers defined by subclasses.
  void WritePrimaryElementAttributes(ostream& os, svtkIndent indent) override;
  virtual void WriteAppendedPiece(int index, svtkIndent indent);
  virtual void WriteAppendedPieceData(int index);
  virtual void WriteInlinePiece(svtkIndent indent);
  virtual void GetInputExtent(int* extent) = 0;

  virtual int WriteHeader();
  virtual int WriteAPiece();
  virtual int WriteFooter();

  virtual void AllocatePositionArrays();
  virtual void DeletePositionArrays();

  virtual int WriteInlineMode(svtkIndent indent);
  svtkIdType GetStartTuple(int* extent, svtkIdType* increments, int i, int j, int k);
  void CalculatePieceFractions(float* fractions);

  void SetInputUpdateExtent(int piece);
  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  svtkSetVector6Macro(InternalWriteExtent, int);

  static svtkIdType GetNumberOfValues(svtkDataSet* input);

  // The extent of the input to write, as specified by user
  int WriteExtent[6];

  // The actual extent of the input to write.
  int InternalWriteExtent[6];

  // Number of pieces used for streaming.
  int NumberOfPieces;

  int WritePiece;

  float* ProgressFractions;

  int CurrentPiece;

  int GhostLevel;

  svtkTypeInt64* ExtentPositions;

  // Appended data offsets of point and cell data arrays.
  // Store offset position (add TimeStep support)
  OffsetsManagerArray* PointDataOM;
  OffsetsManagerArray* CellDataOM;

private:
  svtkXMLStructuredDataWriter(const svtkXMLStructuredDataWriter&) = delete;
  void operator=(const svtkXMLStructuredDataWriter&) = delete;
};

#endif
