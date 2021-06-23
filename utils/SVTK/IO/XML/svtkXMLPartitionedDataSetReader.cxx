/*=========================================================================

  Program:   ParaView
  Module:    svtkXMLPartitionedDataSetReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPartitionedDataSetReader.h"

#include "svtkCompositeDataPipeline.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPartitionedDataSet.h"
#include "svtkSmartPointer.h"
#include "svtkXMLDataElement.h"

svtkStandardNewMacro(svtkXMLPartitionedDataSetReader);

//----------------------------------------------------------------------------
svtkXMLPartitionedDataSetReader::svtkXMLPartitionedDataSetReader() {}

//----------------------------------------------------------------------------
svtkXMLPartitionedDataSetReader::~svtkXMLPartitionedDataSetReader() {}

//----------------------------------------------------------------------------
void svtkXMLPartitionedDataSetReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int svtkXMLPartitionedDataSetReader::FillOutputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkPartitionedDataSet");
  return 1;
}

//----------------------------------------------------------------------------
const char* svtkXMLPartitionedDataSetReader::GetDataSetName()
{
  return "svtkPartitionedDataSet";
}

//----------------------------------------------------------------------------
void svtkXMLPartitionedDataSetReader::ReadComposite(svtkXMLDataElement* element,
  svtkCompositeDataSet* composite, const char* filePath, unsigned int& dataSetIndex)
{
  svtkPartitionedDataSet* pds = svtkPartitionedDataSet::SafeDownCast(composite);
  if (!pds)
  {
    svtkErrorMacro("Unsupported composite dataset.");
    return;
  }

  unsigned int maxElems = element->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < maxElems; ++cc)
  {
    svtkXMLDataElement* childXML = element->GetNestedElement(cc);
    if (!childXML || !childXML->GetName())
    {
      continue;
    }

    int index = pds->GetNumberOfPartitions();

    // child is a leaf node, read and insert.
    const char* tagName = childXML->GetName();
    if (strcmp(tagName, "DataSet") == 0)
    {
      svtkSmartPointer<svtkDataObject> childDS;
      if (this->ShouldReadDataSet(dataSetIndex))
      {
        // Read
        childDS.TakeReference(this->ReadDataObject(childXML, filePath));
      }
      pds->SetPartition(index, childDS);
      dataSetIndex++;
    }
    else
    {
      svtkErrorMacro("Syntax error in file.");
      return;
    }
  }
}
