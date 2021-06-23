/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLMultiBlockDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLMultiBlockDataWriter
 * @brief   writer for svtkMultiBlockDataSet.
 *
 * svtkXMLMultiBlockDataWriter is a svtkXMLCompositeDataWriter subclass to handle
 * svtkMultiBlockDataSet.
 */

#ifndef svtkXMLMultiBlockDataWriter_h
#define svtkXMLMultiBlockDataWriter_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLCompositeDataWriter.h"

class SVTKIOXML_EXPORT svtkXMLMultiBlockDataWriter : public svtkXMLCompositeDataWriter
{
public:
  static svtkXMLMultiBlockDataWriter* New();
  svtkTypeMacro(svtkXMLMultiBlockDataWriter, svtkXMLCompositeDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override { return "vtm"; }

protected:
  svtkXMLMultiBlockDataWriter();
  ~svtkXMLMultiBlockDataWriter() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  // Internal method called recursively to create the xml tree for the children
  // of compositeData.
  int WriteComposite(
    svtkCompositeDataSet* compositeData, svtkXMLDataElement* parent, int& writerIdx) override;

private:
  svtkXMLMultiBlockDataWriter(const svtkXMLMultiBlockDataWriter&) = delete;
  void operator=(const svtkXMLMultiBlockDataWriter&) = delete;
};

#endif
