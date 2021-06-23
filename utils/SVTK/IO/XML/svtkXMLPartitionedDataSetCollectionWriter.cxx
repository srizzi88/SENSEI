/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPartitionedDataSetCollectionWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPartitionedDataSetCollectionWriter.h"

#include "svtkDataObjectTreeRange.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkXMLDataElement.h"

svtkStandardNewMacro(svtkXMLPartitionedDataSetCollectionWriter);
//----------------------------------------------------------------------------
svtkXMLPartitionedDataSetCollectionWriter::svtkXMLPartitionedDataSetCollectionWriter() {}

//----------------------------------------------------------------------------
svtkXMLPartitionedDataSetCollectionWriter::~svtkXMLPartitionedDataSetCollectionWriter() {}

//----------------------------------------------------------------------------
int svtkXMLPartitionedDataSetCollectionWriter::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPartitionedDataSetCollection");
  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLPartitionedDataSetCollectionWriter::WriteComposite(
  svtkCompositeDataSet* compositeData, svtkXMLDataElement* parent, int& writerIdx)
{
  if (!(compositeData->IsA("svtkPartitionedDataSet") ||
        compositeData->IsA("svtkPartitionedDataSetCollection")))
  {
    svtkErrorMacro("Unsupported composite dataset type: " << compositeData->GetClassName() << ".");
    return 0;
  }

  auto* dObjTree = static_cast<svtkDataObjectTree*>(compositeData);

  // Write each input.
  using Opts = svtk::DataObjectTreeOptions;
  const auto dObjRange = svtk::Range(dObjTree, Opts::None);
  int toBeWritten = static_cast<int>(dObjRange.size());

  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);

  int index = 0;
  int RetVal = 0;
  for (svtkDataObject* curDO : dObjRange)
  {
    if (curDO && curDO->IsA("svtkCompositeDataSet"))
    // if node is a supported composite dataset
    // note in structure file and recurse.
    {
      svtkXMLDataElement* tag = svtkXMLDataElement::New();
      tag->SetName("Partitions");
      tag->SetIntAttribute("index", index);
      svtkCompositeDataSet* curCD = svtkCompositeDataSet::SafeDownCast(curDO);
      if (!this->WriteComposite(curCD, tag, writerIdx))
      {
        tag->Delete();
        return 0;
      }
      RetVal = 1;
      parent->AddNestedElement(tag);
      tag->Delete();
    }
    else
    // this node is not a composite data set.
    {
      svtkXMLDataElement* datasetXML = svtkXMLDataElement::New();
      datasetXML->SetName("DataSet");
      datasetXML->SetIntAttribute("index", index);
      svtkStdString fileName = this->CreatePieceFileName(writerIdx);

      this->SetProgressRange(progressRange, writerIdx, toBeWritten);
      if (this->WriteNonCompositeData(curDO, datasetXML, writerIdx, fileName.c_str()))
      {
        parent->AddNestedElement(datasetXML);
        RetVal = 1;
      }
      datasetXML->Delete();
    }

    index++;
  }

  return RetVal;
}

//----------------------------------------------------------------------------
void svtkXMLPartitionedDataSetCollectionWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
