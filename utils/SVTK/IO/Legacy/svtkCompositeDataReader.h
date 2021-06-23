/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCompositeDataReader
 * @brief   read svtkCompositeDataSet data file.
 *
 * @warning
 * This is an experimental format. Use XML-based formats for writing composite
 * datasets. Saving composite dataset in legacy SVTK format is expected to change
 * in future including changes to the file layout.
 */

#ifndef svtkCompositeDataReader_h
#define svtkCompositeDataReader_h

#include "svtkDataReader.h"
#include "svtkIOLegacyModule.h" // For export macro

class svtkCompositeDataSet;
class svtkHierarchicalBoxDataSet;
class svtkMultiBlockDataSet;
class svtkMultiPieceDataSet;
class svtkNonOverlappingAMR;
class svtkOverlappingAMR;
class svtkPartitionedDataSet;
class svtkPartitionedDataSetCollection;

class SVTKIOLEGACY_EXPORT svtkCompositeDataReader : public svtkDataReader
{
public:
  static svtkCompositeDataReader* New();
  svtkTypeMacro(svtkCompositeDataReader, svtkDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output of this reader.
   */
  svtkCompositeDataSet* GetOutput();
  svtkCompositeDataSet* GetOutput(int idx);
  void SetOutput(svtkCompositeDataSet* output);
  //@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(const std::string& fname, svtkDataObject* output) override;

protected:
  svtkCompositeDataReader();
  ~svtkCompositeDataReader() override;

  svtkDataObject* CreateOutput(svtkDataObject* currentOutput) override;

  int FillOutputPortInformation(int, svtkInformation*) override;

  /**
   * Read the output type information.
   */
  int ReadOutputType();

  bool ReadCompositeData(svtkMultiPieceDataSet*);
  bool ReadCompositeData(svtkMultiBlockDataSet*);
  bool ReadCompositeData(svtkHierarchicalBoxDataSet*);
  bool ReadCompositeData(svtkOverlappingAMR*);
  bool ReadCompositeData(svtkPartitionedDataSet*);
  bool ReadCompositeData(svtkPartitionedDataSetCollection*);
  bool ReadCompositeData(svtkNonOverlappingAMR*);
  svtkDataObject* ReadChild();

private:
  svtkCompositeDataReader(const svtkCompositeDataReader&) = delete;
  void operator=(const svtkCompositeDataReader&) = delete;
};

#endif
