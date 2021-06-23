/*=========================================================================

  Program:   ParaView
  Module:    svtkXMLMultiGroupDataReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLMultiGroupDataReader
 * @brief   Reader for multi-block datasets
 *
 * svtkXMLMultiGroupDataReader is a legacy reader that reads multi group files
 * into multiblock datasets.
 */

#ifndef svtkXMLMultiGroupDataReader_h
#define svtkXMLMultiGroupDataReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLMultiBlockDataReader.h"

class SVTKIOXML_EXPORT svtkXMLMultiGroupDataReader : public svtkXMLMultiBlockDataReader
{
public:
  static svtkXMLMultiGroupDataReader* New();
  svtkTypeMacro(svtkXMLMultiGroupDataReader, svtkXMLMultiBlockDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkXMLMultiGroupDataReader();
  ~svtkXMLMultiGroupDataReader() override;

  // Get the name of the data set being read.
  const char* GetDataSetName() override { return "svtkMultiGroupDataSet"; }

private:
  svtkXMLMultiGroupDataReader(const svtkXMLMultiGroupDataReader&) = delete;
  void operator=(const svtkXMLMultiGroupDataReader&) = delete;
};

#endif
