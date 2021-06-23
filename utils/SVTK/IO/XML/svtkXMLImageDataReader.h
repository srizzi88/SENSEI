/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLImageDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLImageDataReader
 * @brief   Read SVTK XML ImageData files.
 *
 * svtkXMLImageDataReader reads the SVTK XML ImageData file format.  One
 * image data file can be read to produce one output.  Streaming is
 * supported.  The standard extension for this reader's file format is
 * "vti".  This reader is also used to read a single piece of the
 * parallel file format.
 *
 * @sa
 * svtkXMLPImageDataReader
 */

#ifndef svtkXMLImageDataReader_h
#define svtkXMLImageDataReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLStructuredDataReader.h"

class svtkImageData;

class SVTKIOXML_EXPORT svtkXMLImageDataReader : public svtkXMLStructuredDataReader
{
public:
  svtkTypeMacro(svtkXMLImageDataReader, svtkXMLStructuredDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLImageDataReader* New();

  //@{
  /**
   * Get the reader's output.
   */
  svtkImageData* GetOutput();
  svtkImageData* GetOutput(int idx);
  //@}

  /**
   * For the specified port, copy the information this reader sets up in
   * SetupOutputInformation to outInfo
   */
  void CopyOutputInformation(svtkInformation* outInfo, int port) override;

protected:
  svtkXMLImageDataReader();
  ~svtkXMLImageDataReader() override;

  double Origin[3];
  double Spacing[3];
  double Direction[9];
  int PieceExtent[6];

  const char* GetDataSetName() override;
  void SetOutputExtent(int* extent) override;

  int ReadPrimaryElement(svtkXMLDataElement* ePrimary) override;

  // Setup the output's information.
  void SetupOutputInformation(svtkInformation* outInfo) override;

  int FillOutputPortInformation(int, svtkInformation*) override;

private:
  svtkXMLImageDataReader(const svtkXMLImageDataReader&) = delete;
  void operator=(const svtkXMLImageDataReader&) = delete;
};

#endif
