/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPolyDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPolyDataWriter
 * @brief   Write SVTK XML PolyData files.
 *
 * svtkXMLPolyDataWriter writes the SVTK XML PolyData file format.  One
 * polygonal data input can be written into one file in any number of
 * streamed pieces (if supported by the rest of the pipeline).  The
 * standard extension for this writer's file format is "vtp".  This
 * writer is also used to write a single piece of the parallel file
 * format.
 *
 * @sa
 * svtkXMLPPolyDataWriter
 */

#ifndef svtkXMLPolyDataWriter_h
#define svtkXMLPolyDataWriter_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLUnstructuredDataWriter.h"

class svtkPolyData;

class SVTKIOXML_EXPORT svtkXMLPolyDataWriter : public svtkXMLUnstructuredDataWriter
{
public:
  svtkTypeMacro(svtkXMLPolyDataWriter, svtkXMLUnstructuredDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLPolyDataWriter* New();

  /**
   * Get/Set the writer's input.
   */
  svtkPolyData* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

protected:
  svtkXMLPolyDataWriter();
  ~svtkXMLPolyDataWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

  const char* GetDataSetName() override;

  void AllocatePositionArrays() override;
  void DeletePositionArrays() override;

  void WriteInlinePieceAttributes() override;
  void WriteInlinePiece(svtkIndent indent) override;

  void WriteAppendedPieceAttributes(int index) override;
  void WriteAppendedPiece(int index, svtkIndent indent) override;
  void WriteAppendedPieceData(int index) override;

  svtkIdType GetNumberOfInputCells() override;
  void CalculateSuperclassFraction(float* fractions);

  // Positions of attributes for each piece.
  unsigned long* NumberOfVertsPositions;
  unsigned long* NumberOfLinesPositions;
  unsigned long* NumberOfStripsPositions;
  unsigned long* NumberOfPolysPositions;

  OffsetsManagerArray* VertsOM;
  OffsetsManagerArray* LinesOM;
  OffsetsManagerArray* StripsOM;
  OffsetsManagerArray* PolysOM;

private:
  svtkXMLPolyDataWriter(const svtkXMLPolyDataWriter&) = delete;
  void operator=(const svtkXMLPolyDataWriter&) = delete;
};

#endif
