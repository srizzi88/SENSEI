/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPImageDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPImageDataWriter
 * @brief   Write PSVTK XML ImageData files.
 *
 * svtkXMLPImageDataWriter writes the PSVTK XML ImageData file format.
 * One image data input can be written into a parallel file format
 * with any number of pieces spread across files.  The standard
 * extension for this writer's file format is "pvti".  This writer
 * uses svtkXMLImageDataWriter to write the individual piece files.
 *
 * @sa
 * svtkXMLImageDataWriter
 */

#ifndef svtkXMLPImageDataWriter_h
#define svtkXMLPImageDataWriter_h

#include "svtkIOParallelXMLModule.h" // For export macro
#include "svtkXMLPStructuredDataWriter.h"

class svtkImageData;

class SVTKIOPARALLELXML_EXPORT svtkXMLPImageDataWriter : public svtkXMLPStructuredDataWriter
{
public:
  static svtkXMLPImageDataWriter* New();
  svtkTypeMacro(svtkXMLPImageDataWriter, svtkXMLPStructuredDataWriter);
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
  svtkXMLPImageDataWriter();
  ~svtkXMLPImageDataWriter() override;

  const char* GetDataSetName() override;
  void WritePrimaryElementAttributes(ostream& os, svtkIndent indent) override;
  svtkXMLStructuredDataWriter* CreateStructuredPieceWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkXMLPImageDataWriter(const svtkXMLPImageDataWriter&) = delete;
  void operator=(const svtkXMLPImageDataWriter&) = delete;
};

#endif
