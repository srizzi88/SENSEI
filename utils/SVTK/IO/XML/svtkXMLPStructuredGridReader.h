/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPStructuredGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPStructuredGridReader
 * @brief   Read PSVTK XML StructuredGrid files.
 *
 * svtkXMLPStructuredGridReader reads the PSVTK XML StructuredGrid file
 * format.  This reads the parallel format's summary file and then
 * uses svtkXMLStructuredGridReader to read data from the individual
 * StructuredGrid piece files.  Streaming is supported.  The standard
 * extension for this reader's file format is "pvts".
 *
 * @sa
 * svtkXMLStructuredGridReader
 */

#ifndef svtkXMLPStructuredGridReader_h
#define svtkXMLPStructuredGridReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLPStructuredDataReader.h"

class svtkStructuredGrid;

class SVTKIOXML_EXPORT svtkXMLPStructuredGridReader : public svtkXMLPStructuredDataReader
{
public:
  svtkTypeMacro(svtkXMLPStructuredGridReader, svtkXMLPStructuredDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLPStructuredGridReader* New();

  /**
   * Get the reader's output.
   */
  svtkStructuredGrid* GetOutput();

  /**
   * Needed for ParaView
   */
  svtkStructuredGrid* GetOutput(int idx);

protected:
  svtkXMLPStructuredGridReader();
  ~svtkXMLPStructuredGridReader() override;

  svtkStructuredGrid* GetPieceInput(int index);

  void SetupEmptyOutput() override;
  const char* GetDataSetName() override;
  void SetOutputExtent(int* extent) override;
  void GetPieceInputExtent(int index, int* extent) override;
  int ReadPrimaryElement(svtkXMLDataElement* ePrimary) override;
  void SetupOutputData() override;
  int ReadPieceData() override;
  svtkXMLDataReader* CreatePieceReader() override;
  int FillOutputPortInformation(int, svtkInformation*) override;

  // The PPoints element with point information.
  svtkXMLDataElement* PPointsElement;

private:
  svtkXMLPStructuredGridReader(const svtkXMLPStructuredGridReader&) = delete;
  void operator=(const svtkXMLPStructuredGridReader&) = delete;
};

#endif
