/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPRectilinearGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPRectilinearGridReader
 * @brief   Read PSVTK XML RectilinearGrid files.
 *
 * svtkXMLPRectilinearGridReader reads the PSVTK XML RectilinearGrid
 * file format.  This reads the parallel format's summary file and
 * then uses svtkXMLRectilinearGridReader to read data from the
 * individual RectilinearGrid piece files.  Streaming is supported.
 * The standard extension for this reader's file format is "pvtr".
 *
 * @sa
 * svtkXMLRectilinearGridReader
 */

#ifndef svtkXMLPRectilinearGridReader_h
#define svtkXMLPRectilinearGridReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLPStructuredDataReader.h"

class svtkRectilinearGrid;

class SVTKIOXML_EXPORT svtkXMLPRectilinearGridReader : public svtkXMLPStructuredDataReader
{
public:
  svtkTypeMacro(svtkXMLPRectilinearGridReader, svtkXMLPStructuredDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLPRectilinearGridReader* New();

  //@{
  /**
   * Get the reader's output.
   */
  svtkRectilinearGrid* GetOutput();
  svtkRectilinearGrid* GetOutput(int idx);
  //@}

protected:
  svtkXMLPRectilinearGridReader();
  ~svtkXMLPRectilinearGridReader() override;

  svtkRectilinearGrid* GetPieceInput(int index);

  void SetupEmptyOutput() override;
  const char* GetDataSetName() override;
  void SetOutputExtent(int* extent) override;
  void GetPieceInputExtent(int index, int* extent) override;
  int ReadPrimaryElement(svtkXMLDataElement* ePrimary) override;
  void SetupOutputData() override;
  int ReadPieceData() override;
  svtkXMLDataReader* CreatePieceReader() override;
  void CopySubCoordinates(
    int* inBounds, int* outBounds, int* subBounds, svtkDataArray* inArray, svtkDataArray* outArray);
  int FillOutputPortInformation(int, svtkInformation*) override;

  // The PCoordinates element with coordinate information.
  svtkXMLDataElement* PCoordinatesElement;

private:
  svtkXMLPRectilinearGridReader(const svtkXMLPRectilinearGridReader&) = delete;
  void operator=(const svtkXMLPRectilinearGridReader&) = delete;
};

#endif
