/*=========================================================================

  Program:   ParaView
  Module:    svtkXMLPartitionedDataSetReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPartitionedDataSetReader
 * @brief   Reader for partitioned datasets
 *
 * svtkXMLPartitionedDataSetReader reads the SVTK XML partitioned dataset file
 * format. XML partitioned dataset files are meta-files that point to a list
 * of serial SVTK XML files. When reading in parallel, it will distribute
 * sub-blocks among processors. If the number of sub-blocks is less than
 * the number of processors, some processors will not have any sub-blocks
 * for that block. If the number of sub-blocks is larger than the
 * number of processors, each processor will possibly have more than
 * 1 sub-block.
 */

#ifndef svtkXMLPartitionedDataSetReader_h
#define svtkXMLPartitionedDataSetReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLCompositeDataReader.h"

class svtkMultiBlockDataSet;

class SVTKIOXML_EXPORT svtkXMLPartitionedDataSetReader : public svtkXMLCompositeDataReader
{
public:
  static svtkXMLPartitionedDataSetReader* New();
  svtkTypeMacro(svtkXMLPartitionedDataSetReader, svtkXMLCompositeDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkXMLPartitionedDataSetReader();
  ~svtkXMLPartitionedDataSetReader() override;

  // Read the XML element for the subtree of a the composite dataset.
  // dataSetIndex is used to rank the leaf nodes in an inorder traversal.
  void ReadComposite(svtkXMLDataElement* element, svtkCompositeDataSet* composite,
    const char* filePath, unsigned int& dataSetIndex) override;

  // Get the name of the data set being read.
  const char* GetDataSetName() override;

  int FillOutputPortInformation(int, svtkInformation* info) override;

private:
  svtkXMLPartitionedDataSetReader(const svtkXMLPartitionedDataSetReader&) = delete;
  void operator=(const svtkXMLPartitionedDataSetReader&) = delete;
};

#endif
