/*=========================================================================

  Program:   ParaView
  Module:    svtkXMLMultiBlockDataReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLMultiBlockDataReader
 * @brief   Reader for multi-block datasets
 *
 * svtkXMLMultiBlockDataReader reads the SVTK XML multi-block data file
 * format. XML multi-block data files are meta-files that point to a list
 * of serial SVTK XML files. When reading in parallel, it will distribute
 * sub-blocks among processor. If the number of sub-blocks is less than
 * the number of processors, some processors will not have any sub-blocks
 * for that block. If the number of sub-blocks is larger than the
 * number of processors, each processor will possibly have more than
 * 1 sub-block.
 */

#ifndef svtkXMLMultiBlockDataReader_h
#define svtkXMLMultiBlockDataReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLCompositeDataReader.h"

class svtkMultiBlockDataSet;

class SVTKIOXML_EXPORT svtkXMLMultiBlockDataReader : public svtkXMLCompositeDataReader
{
public:
  static svtkXMLMultiBlockDataReader* New();
  svtkTypeMacro(svtkXMLMultiBlockDataReader, svtkXMLCompositeDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkXMLMultiBlockDataReader();
  ~svtkXMLMultiBlockDataReader() override;

  // Read the XML element for the subtree of a the composite dataset.
  // dataSetIndex is used to rank the leaf nodes in an inorder traversal.
  void ReadComposite(svtkXMLDataElement* element, svtkCompositeDataSet* composite,
    const char* filePath, unsigned int& dataSetIndex) override;

  // Reads file version < 1.0.
  virtual void ReadVersion0(svtkXMLDataElement* element, svtkCompositeDataSet* composite,
    const char* filePath, unsigned int& dataSetIndex);

  // Get the name of the data set being read.
  const char* GetDataSetName() override;

  int FillOutputPortInformation(int, svtkInformation* info) override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  virtual int FillMetaData(svtkCompositeDataSet* metadata, svtkXMLDataElement* element,
    const std::string& filePath, unsigned int& dataSetIndex);

private:
  svtkXMLMultiBlockDataReader(const svtkXMLMultiBlockDataReader&) = delete;
  void operator=(const svtkXMLMultiBlockDataReader&) = delete;
};

#endif
