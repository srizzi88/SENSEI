/*=========================================================================

  Program:   ParaView
  Module:    svtkXMLUniformGridAMRReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLUniformGridAMRReader
 * @brief   Reader for amr datasets (svtkOverlappingAMR
 * or svtkNonOverlappingAMR).
 *
 * svtkXMLUniformGridAMRReader reads the SVTK XML data files for all types of amr
 * datasets including svtkOverlappingAMR, svtkNonOverlappingAMR and the legacy
 * svtkHierarchicalBoxDataSet. The reader uses information in the file to
 * determine what type of dataset is actually being read and creates the
 * output-data object accordingly.
 *
 * This reader can only read files with version 1.1 or greater.
 * Older versions can be converted to the newer versions using
 * svtkXMLHierarchicalBoxDataFileConverter.
 */

#ifndef svtkXMLUniformGridAMRReader_h
#define svtkXMLUniformGridAMRReader_h

#include "svtkIOXMLModule.h"  // For export macro
#include "svtkSmartPointer.h" // needed for svtkSmartPointer.
#include "svtkXMLCompositeDataReader.h"

class svtkOverlappingAMR;
class svtkUniformGridAMR;

class SVTKIOXML_EXPORT svtkXMLUniformGridAMRReader : public svtkXMLCompositeDataReader
{
public:
  static svtkXMLUniformGridAMRReader* New();
  svtkTypeMacro(svtkXMLUniformGridAMRReader, svtkXMLCompositeDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * This reader supports demand-driven heavy data reading i.e. downstream
   * pipeline can request specific blocks from the AMR using
   * svtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES() key in
   * RequestUpdateExtent() pass. However, when down-stream doesn't provide any
   * specific keys, the default behavior can be setup to read at-most N levels
   * by default. The number of levels read can be set using this method.
   * Set this to 0 to imply no limit. Default is 0.
   */
  svtkSetMacro(MaximumLevelsToReadByDefault, unsigned int);
  svtkGetMacro(MaximumLevelsToReadByDefault, unsigned int);
  //@}

protected:
  svtkXMLUniformGridAMRReader();
  ~svtkXMLUniformGridAMRReader() override;

  /**
   * This method is used by CanReadFile() to check if the reader can read an XML
   * with the primary element with the given name. Default implementation
   * compares the name with the text returned by this->GetDataSetName().
   * Overridden to support all AMR types.
   */
  int CanReadFileWithDataType(const char* dsname) override;

  /**
   * Read the top-level element from the file.  This is always the
   * SVTKFile element.
   * Overridden to read the "type" information specified in the XML. The "type"
   * attribute helps us identify the output data type.
   */
  int ReadSVTKFile(svtkXMLDataElement* eSVTKFile) override;

  /**
   * Read the meta-data from the AMR from the file. Note that since
   * ReadPrimaryElement() is only called when the filename changes, we are
   * technically not supporting time-varying AMR datasets in this format right
   * now.
   */
  int ReadPrimaryElement(svtkXMLDataElement* ePrimary) override;

  /**
   * Overridden to create an output data object based on the type in the file.
   * Since this reader can handle all subclasses of svtkUniformGrid, we need to
   * check in the file to decide what type to create.
   */
  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Overridden to put svtkOverlappingAMR in the pipeline if
   * available/applicable.
   */
  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  // Get the name of the data set being read.
  const char* GetDataSetName() override;

  // Read the XML element for the subtree of a the composite dataset.
  // dataSetIndex is used to rank the leaf nodes in an inorder traversal.
  void ReadComposite(svtkXMLDataElement* element, svtkCompositeDataSet* composite,
    const char* filePath, unsigned int& dataSetIndex) override;

  // Read the svtkDataSet (a leaf) in the composite dataset.
  svtkDataSet* ReadDataset(svtkXMLDataElement* xmlElem, const char* filePath) override;

  svtkSmartPointer<svtkOverlappingAMR> Metadata;
  unsigned int MaximumLevelsToReadByDefault;

private:
  svtkXMLUniformGridAMRReader(const svtkXMLUniformGridAMRReader&) = delete;
  void operator=(const svtkXMLUniformGridAMRReader&) = delete;

  char* OutputDataType;
  svtkSetStringMacro(OutputDataType);
};

#endif
