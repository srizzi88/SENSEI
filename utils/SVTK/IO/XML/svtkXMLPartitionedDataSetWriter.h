/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPartitionedDataSetWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPartitionedDataSetWriter
 * @brief   writer for svtkPartitionedDataSet.
 *
 * svtkXMLPartitionedDataSetWriter is a svtkXMLCompositeDataWriter subclass to handle
 * svtkPartitionedDataSet.
 */

#ifndef svtkXMLPartitionedDataSetWriter_h
#define svtkXMLPartitionedDataSetWriter_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLCompositeDataWriter.h"

class SVTKIOXML_EXPORT svtkXMLPartitionedDataSetWriter : public svtkXMLCompositeDataWriter
{
public:
  static svtkXMLPartitionedDataSetWriter* New();
  svtkTypeMacro(svtkXMLPartitionedDataSetWriter, svtkXMLCompositeDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override { return "vtpd"; }

protected:
  svtkXMLPartitionedDataSetWriter();
  ~svtkXMLPartitionedDataSetWriter() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  // Internal method called recursively to create the xml tree for the children
  // of compositeData.
  int WriteComposite(
    svtkCompositeDataSet* compositeData, svtkXMLDataElement* parent, int& writerIdx) override;

private:
  svtkXMLPartitionedDataSetWriter(const svtkXMLPartitionedDataSetWriter&) = delete;
  void operator=(const svtkXMLPartitionedDataSetWriter&) = delete;
};

#endif
