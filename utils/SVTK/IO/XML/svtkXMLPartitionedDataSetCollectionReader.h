/*=========================================================================

  Program:   ParaView
  Module:    svtkXMLPartitionedDataSetCollectionReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPartitionedDataSetCollectionReader
 * @brief   Reader for partitioned dataset collections
 *
 * svtkXMLPartitionedDataSetCollectionReader reads the SVTK XML partitioned
 * dataset collection file format. These are meta-files that point to a list
 * of serial SVTK XML files. When reading in parallel, it will distribute
 * sub-blocks among processor. If the number of sub-blocks is less than
 * the number of processors, some processors will not have any sub-blocks
 * for that block. If the number of sub-blocks is larger than the
 * number of processors, each processor will possibly have more than
 * 1 sub-block.
 */

#ifndef svtkXMLPartitionedDataSetCollectionReader_h
#define svtkXMLPartitionedDataSetCollectionReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLCompositeDataReader.h"

class svtkMultiBlockDataSet;

class SVTKIOXML_EXPORT svtkXMLPartitionedDataSetCollectionReader : public svtkXMLCompositeDataReader
{
public:
  static svtkXMLPartitionedDataSetCollectionReader* New();
  svtkTypeMacro(svtkXMLPartitionedDataSetCollectionReader, svtkXMLCompositeDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkXMLPartitionedDataSetCollectionReader();
  ~svtkXMLPartitionedDataSetCollectionReader() override;

  // Read the XML element for the subtree of a the composite dataset.
  // dataSetIndex is used to rank the leaf nodes in an inorder traversal.
  void ReadComposite(svtkXMLDataElement* element, svtkCompositeDataSet* composite,
    const char* filePath, unsigned int& dataSetIndex) override;

  // Get the name of the data set being read.
  const char* GetDataSetName() override;

  int FillOutputPortInformation(int, svtkInformation* info) override;

private:
  svtkXMLPartitionedDataSetCollectionReader(
    const svtkXMLPartitionedDataSetCollectionReader&) = delete;
  void operator=(const svtkXMLPartitionedDataSetCollectionReader&) = delete;
};

#endif
