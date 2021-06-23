/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPartitionedDataSetCollectionWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPartitionedDataSetCollectionWriter
 * @brief   writer for svtkPartitionedDataSetCollection.
 *
 * svtkXMLPartitionedDataSetCollectionWriter is a svtkXMLCompositeDataWriter
 * subclass to handle svtkPartitionedDataSetCollection.
 */

#ifndef svtkXMLPartitionedDataSetCollectionWriter_h
#define svtkXMLPartitionedDataSetCollectionWriter_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLCompositeDataWriter.h"

class SVTKIOXML_EXPORT svtkXMLPartitionedDataSetCollectionWriter : public svtkXMLCompositeDataWriter
{
public:
  static svtkXMLPartitionedDataSetCollectionWriter* New();
  svtkTypeMacro(svtkXMLPartitionedDataSetCollectionWriter, svtkXMLCompositeDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override { return "vtpc"; }

protected:
  svtkXMLPartitionedDataSetCollectionWriter();
  ~svtkXMLPartitionedDataSetCollectionWriter() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  // Internal method called recursively to create the xml tree for the children
  // of compositeData.
  int WriteComposite(
    svtkCompositeDataSet* compositeData, svtkXMLDataElement* parent, int& writerIdx) override;

private:
  svtkXMLPartitionedDataSetCollectionWriter(
    const svtkXMLPartitionedDataSetCollectionWriter&) = delete;
  void operator=(const svtkXMLPartitionedDataSetCollectionWriter&) = delete;
};

#endif
