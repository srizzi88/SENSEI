/*=========================================================================

  Program:   ParaView
  Module:    svtkXMLHierarchicalDataReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLHierarchicalDataReader
 * @brief   Reader for hierarchical datasets
 *
 * svtkXMLHierarchicalDataReader reads the SVTK XML hierarchical data file
 * format. XML hierarchical data files are meta-files that point to a list
 * of serial SVTK XML files. When reading in parallel, it will distribute
 * sub-blocks among processor. If the number of sub-blocks is less than
 * the number of processors, some processors will not have any sub-blocks
 * for that level. If the number of sub-blocks is larger than the
 * number of processors, each processor will possibly have more than
 * 1 sub-block.
 */

#ifndef svtkXMLHierarchicalDataReader_h
#define svtkXMLHierarchicalDataReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLMultiGroupDataReader.h"

class svtkHierarchicalDataSet;

class SVTKIOXML_EXPORT svtkXMLHierarchicalDataReader : public svtkXMLMultiGroupDataReader
{
public:
  static svtkXMLHierarchicalDataReader* New();
  svtkTypeMacro(svtkXMLHierarchicalDataReader, svtkXMLMultiGroupDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkXMLHierarchicalDataReader();
  ~svtkXMLHierarchicalDataReader() override;

  // Get the name of the data set being read.
  const char* GetDataSetName() override { return "svtkHierarchicalDataSet"; }

private:
  svtkXMLHierarchicalDataReader(const svtkXMLHierarchicalDataReader&) = delete;
  void operator=(const svtkXMLHierarchicalDataReader&) = delete;
};

#endif
