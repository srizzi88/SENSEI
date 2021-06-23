/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLImageDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLImageDataWriter
 * @brief   Write SVTK XML ImageData files.
 *
 * svtkXMLImageDataWriter writes the SVTK XML ImageData file format.
 * One image data input can be written into one file in any number of
 * streamed pieces.  The standard extension for this writer's file
 * format is "vti".  This writer is also used to write a single piece
 * of the parallel file format.
 *
 * @sa
 * svtkXMLPImageDataWriter
 */

#ifndef svtkXMLImageDataWriter_h
#define svtkXMLImageDataWriter_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLStructuredDataWriter.h"

class svtkImageData;

class SVTKIOXML_EXPORT svtkXMLImageDataWriter : public svtkXMLStructuredDataWriter
{
public:
  static svtkXMLImageDataWriter* New();
  svtkTypeMacro(svtkXMLImageDataWriter, svtkXMLStructuredDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get/Set the writer's input.
   */
  svtkImageData* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

protected:
  svtkXMLImageDataWriter();
  ~svtkXMLImageDataWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

  void WritePrimaryElementAttributes(ostream& os, svtkIndent indent) override;
  void GetInputExtent(int* extent) override;
  const char* GetDataSetName() override;

private:
  svtkXMLImageDataWriter(const svtkXMLImageDataWriter&) = delete;
  void operator=(const svtkXMLImageDataWriter&) = delete;
};

#endif
