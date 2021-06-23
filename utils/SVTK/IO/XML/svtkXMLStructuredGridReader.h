/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLStructuredGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLStructuredGridReader
 * @brief   Read SVTK XML StructuredGrid files.
 *
 * svtkXMLStructuredGridReader reads the SVTK XML StructuredGrid file
 * format.  One structured grid file can be read to produce one
 * output.  Streaming is supported.  The standard extension for this
 * reader's file format is "vts".  This reader is also used to read a
 * single piece of the parallel file format.
 *
 * @sa
 * svtkXMLPStructuredGridReader
 */

#ifndef svtkXMLStructuredGridReader_h
#define svtkXMLStructuredGridReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLStructuredDataReader.h"

class svtkStructuredGrid;

class SVTKIOXML_EXPORT svtkXMLStructuredGridReader : public svtkXMLStructuredDataReader
{
public:
  svtkTypeMacro(svtkXMLStructuredGridReader, svtkXMLStructuredDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLStructuredGridReader* New();

  //@{
  /**
   * Get the reader's output.
   */
  svtkStructuredGrid* GetOutput();
  svtkStructuredGrid* GetOutput(int idx);
  //@}

protected:
  svtkXMLStructuredGridReader();
  ~svtkXMLStructuredGridReader() override;

  const char* GetDataSetName() override;
  void SetOutputExtent(int* extent) override;

  void SetupPieces(int numPieces) override;
  void DestroyPieces() override;
  void SetupOutputData() override;

  int ReadPiece(svtkXMLDataElement* ePiece) override;
  int ReadPieceData() override;
  int FillOutputPortInformation(int, svtkInformation*) override;

  // The elements representing the points for each piece.
  svtkXMLDataElement** PointElements;

private:
  svtkXMLStructuredGridReader(const svtkXMLStructuredGridReader&) = delete;
  void operator=(const svtkXMLStructuredGridReader&) = delete;
};

#endif
