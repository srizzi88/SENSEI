/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLRectilinearGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLRectilinearGridReader
 * @brief   Read SVTK XML RectilinearGrid files.
 *
 * svtkXMLRectilinearGridReader reads the SVTK XML RectilinearGrid file
 * format.  One rectilinear grid file can be read to produce one
 * output.  Streaming is supported.  The standard extension for this
 * reader's file format is "vtr".  This reader is also used to read a
 * single piece of the parallel file format.
 *
 * @sa
 * svtkXMLPRectilinearGridReader
 */

#ifndef svtkXMLRectilinearGridReader_h
#define svtkXMLRectilinearGridReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLStructuredDataReader.h"

class svtkRectilinearGrid;

class SVTKIOXML_EXPORT svtkXMLRectilinearGridReader : public svtkXMLStructuredDataReader
{
public:
  svtkTypeMacro(svtkXMLRectilinearGridReader, svtkXMLStructuredDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLRectilinearGridReader* New();

  //@{
  /**
   * Get the reader's output.
   */
  svtkRectilinearGrid* GetOutput();
  svtkRectilinearGrid* GetOutput(int idx);
  //@}

protected:
  svtkXMLRectilinearGridReader();
  ~svtkXMLRectilinearGridReader() override;

  const char* GetDataSetName() override;
  void SetOutputExtent(int* extent) override;

  void SetupPieces(int numPieces) override;
  void DestroyPieces() override;
  void SetupOutputData() override;
  int ReadPiece(svtkXMLDataElement* ePiece) override;
  int ReadPieceData() override;
  int ReadSubCoordinates(
    int* inBounds, int* outBounds, int* subBounds, svtkXMLDataElement* da, svtkDataArray* array);
  int FillOutputPortInformation(int, svtkInformation*) override;

  // The elements representing the coordinate arrays for each piece.
  svtkXMLDataElement** CoordinateElements;

private:
  svtkXMLRectilinearGridReader(const svtkXMLRectilinearGridReader&) = delete;
  void operator=(const svtkXMLRectilinearGridReader&) = delete;
};

#endif
