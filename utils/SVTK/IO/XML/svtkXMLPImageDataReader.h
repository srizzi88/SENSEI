/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPImageDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPImageDataReader
 * @brief   Read PSVTK XML ImageData files.
 *
 * svtkXMLPImageDataReader reads the PSVTK XML ImageData file format.
 * This reads the parallel format's summary file and then uses
 * svtkXMLImageDataReader to read data from the individual ImageData
 * piece files.  Streaming is supported.  The standard extension for
 * this reader's file format is "pvti".
 *
 * @sa
 * svtkXMLImageDataReader
 */

#ifndef svtkXMLPImageDataReader_h
#define svtkXMLPImageDataReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLPStructuredDataReader.h"

class svtkImageData;

class SVTKIOXML_EXPORT svtkXMLPImageDataReader : public svtkXMLPStructuredDataReader
{
public:
  svtkTypeMacro(svtkXMLPImageDataReader, svtkXMLPStructuredDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLPImageDataReader* New();

  //@{
  /**
   * Get the reader's output.
   */
  svtkImageData* GetOutput();
  svtkImageData* GetOutput(int idx);
  //@}

  // For the specified port, copy the information this reader sets up in
  // SetupOutputInformation to outInfo
  void CopyOutputInformation(svtkInformation* outInfo, int port) override;

protected:
  svtkXMLPImageDataReader();
  ~svtkXMLPImageDataReader() override;

  double Origin[3];
  double Spacing[3];

  svtkImageData* GetPieceInput(int index);

  void SetupEmptyOutput() override;
  const char* GetDataSetName() override;
  void SetOutputExtent(int* extent) override;
  void GetPieceInputExtent(int index, int* extent) override;
  int ReadPrimaryElement(svtkXMLDataElement* ePrimary) override;

  // Setup the output's information.
  void SetupOutputInformation(svtkInformation* outInfo) override;

  svtkXMLDataReader* CreatePieceReader() override;
  int FillOutputPortInformation(int, svtkInformation*) override;

private:
  svtkXMLPImageDataReader(const svtkXMLPImageDataReader&) = delete;
  void operator=(const svtkXMLPImageDataReader&) = delete;
};

#endif
