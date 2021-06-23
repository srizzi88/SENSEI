/*=========================================================================

  Program:   ParaView
  Module:    svtkXMLPartitionedDataSetCollectionReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPartitionedDataSetCollectionReader.h"

#include "svtkCompositeDataPipeline.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPartitionedDataSet.h"
#include "svtkPartitionedDataSetCollection.h"
#include "svtkSmartPointer.h"
#include "svtkXMLDataElement.h"

svtkStandardNewMacro(svtkXMLPartitionedDataSetCollectionReader);

//----------------------------------------------------------------------------
svtkXMLPartitionedDataSetCollectionReader::svtkXMLPartitionedDataSetCollectionReader() {}

//----------------------------------------------------------------------------
svtkXMLPartitionedDataSetCollectionReader::~svtkXMLPartitionedDataSetCollectionReader() {}

//----------------------------------------------------------------------------
void svtkXMLPartitionedDataSetCollectionReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int svtkXMLPartitionedDataSetCollectionReader::FillOutputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkPartitionedDataSetCollection");
  return 1;
}

//----------------------------------------------------------------------------
const char* svtkXMLPartitionedDataSetCollectionReader::GetDataSetName()
{
  return "svtkPartitionedDataSetCollection";
}

//----------------------------------------------------------------------------
void svtkXMLPartitionedDataSetCollectionReader::ReadComposite(svtkXMLDataElement* element,
  svtkCompositeDataSet* composite, const char* filePath, unsigned int& dataSetIndex)
{
  svtkPartitionedDataSetCollection* col = svtkPartitionedDataSetCollection::SafeDownCast(composite);
  svtkPartitionedDataSet* ds = svtkPartitionedDataSet::SafeDownCast(composite);
  if (!col && !ds)
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

    int index = 0;
    if (!childXML->GetScalarAttribute("index", index))
    // if index not in the structure file, then
    // set up to add at the end
    {
      if (col)
      {
        index = col->GetNumberOfPartitionedDataSets();
      }
      else if (ds)
      {
        index = ds->GetNumberOfPartitions();
      }
    }
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
      // insert
      ds->SetPartition(index, childDS);
      dataSetIndex++;
    }
    else if (col != nullptr && strcmp(tagName, "Partitions") == 0)
    {
      svtkPartitionedDataSet* childDS = svtkPartitionedDataSet::New();
      this->ReadComposite(childXML, childDS, filePath, dataSetIndex);
      col->SetPartitionedDataSet(index, childDS);
      childDS->Delete();
    }
    else
    {
      svtkErrorMacro("Syntax error in file.");
      return;
    }
  }
}
