/*=========================================================================

  Program:   ParaView
  Module:    svtkXMLCompositeDataReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLCompositeDataReader
 * @brief   Reader for multi-group datasets
 *
 * svtkXMLCompositeDataReader reads the SVTK XML multi-group data file
 * format. XML multi-group data files are meta-files that point to a list
 * of serial SVTK XML files. When reading in parallel, it will distribute
 * sub-blocks among processor. If the number of sub-blocks is less than
 * the number of processors, some processors will not have any sub-blocks
 * for that group. If the number of sub-blocks is larger than the
 * number of processors, each processor will possibly have more than
 * 1 sub-block.
 */

#ifndef svtkXMLCompositeDataReader_h
#define svtkXMLCompositeDataReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLReader.h"

class svtkCompositeDataSet;
class svtkInformationIntegerKey;
class svtkInformationIntegerVectorKey;

struct svtkXMLCompositeDataReaderInternals;

class SVTKIOXML_EXPORT svtkXMLCompositeDataReader : public svtkXMLReader
{
public:
  svtkTypeMacro(svtkXMLCompositeDataReader, svtkXMLReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum
  {
    Block,
    Interleave
  };

  /**
   * Set the strategy for assigning files to parallel readers. The default is
   * @a Block.
   *
   * Let @a X be the rank of a specific reader, and @a N be the number of
   * reader, then:
   * @arg @c Block Each processor is assigned a contiguous block of files,
   *      [@a X * @a N, ( @a X + 1) * @a N ).
   * @arg @c Interleave The files are interleaved across readers,
   * @a i * @a N + @a X.
   * @{
   */
  svtkSetClampMacro(PieceDistribution, int, Block, Interleave);
  svtkGetMacro(PieceDistribution, int);
  /**@}*/

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  svtkCompositeDataSet* GetOutput();
  svtkCompositeDataSet* GetOutput(int);
  //@}

protected:
  svtkXMLCompositeDataReader();
  ~svtkXMLCompositeDataReader() override;

  // Get the name of the data set being read.
  const char* GetDataSetName() override;

  // Returns the primary element pass to ReadPrimaryElement().
  svtkXMLDataElement* GetPrimaryElement();

  void ReadXMLData() override;
  int ReadPrimaryElement(svtkXMLDataElement* ePrimary) override;

  // Setup the output with no data available.  Used in error cases.
  void SetupEmptyOutput() override;

  int FillOutputPortInformation(int, svtkInformation* info) override;

  // Create a default executive.
  svtkExecutive* CreateDefaultExecutive() override;

  // Find the path to this file in case the internal files are
  // specified as relative paths.
  std::string GetFilePath();

  std::string GetFileNameFromXML(svtkXMLDataElement* xmlElem, const std::string& filePath);

  svtkXMLReader* GetReaderOfType(const char* type);
  svtkXMLReader* GetReaderForFile(const std::string& filename);

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void SyncDataArraySelections(
    svtkXMLReader* accum, svtkXMLDataElement* xmlElem, const std::string& filePath);

  // Adds a child data object to the composite parent. childXML is the XML for
  // the child data object need to obtain certain meta-data about the child.
  void AddChild(svtkCompositeDataSet* parent, svtkDataObject* child, svtkXMLDataElement* childXML);

  // Read the XML element for the subtree of a the composite dataset.
  // dataSetIndex is used to rank the leaf nodes in an inorder traversal.
  virtual void ReadComposite(svtkXMLDataElement* element, svtkCompositeDataSet* composite,
    const char* filePath, unsigned int& dataSetIndex) = 0;

  // Read the svtkDataSet (a leaf) in the composite dataset.
  virtual svtkDataSet* ReadDataset(svtkXMLDataElement* xmlElem, const char* filePath);

  // Read the svtkDataObject (a leaf) in the composite dataset.
  virtual svtkDataObject* ReadDataObject(svtkXMLDataElement* xmlElem, const char* filePath);

  // Counts "DataSet" elements in the subtree.
  unsigned int CountLeaves(svtkXMLDataElement* elem);

  /**
   * Given the inorder index for a leaf node, this method tells if the current
   * process should read the dataset.
   */
  int ShouldReadDataSet(unsigned int datasetIndex);

  bool DataSetIsValidForBlockStrategy(unsigned int datasetIndex);
  bool DataSetIsValidForInterleaveStrategy(unsigned int datasetIndex);

private:
  svtkXMLCompositeDataReader(const svtkXMLCompositeDataReader&) = delete;
  void operator=(const svtkXMLCompositeDataReader&) = delete;

  int PieceDistribution;

  svtkXMLCompositeDataReaderInternals* Internal;
};

#endif
