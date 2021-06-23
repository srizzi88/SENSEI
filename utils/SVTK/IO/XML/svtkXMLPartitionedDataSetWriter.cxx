/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPartitionedDataSetWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPartitionedDataSetWriter.h"

#include "svtkDataObjectTreeIterator.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkXMLDataElement.h"

svtkStandardNewMacro(svtkXMLPartitionedDataSetWriter);
//----------------------------------------------------------------------------
svtkXMLPartitionedDataSetWriter::svtkXMLPartitionedDataSetWriter() {}

//----------------------------------------------------------------------------
svtkXMLPartitionedDataSetWriter::~svtkXMLPartitionedDataSetWriter() {}

//----------------------------------------------------------------------------
int svtkXMLPartitionedDataSetWriter::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPartitionedDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLPartitionedDataSetWriter::WriteComposite(
  svtkCompositeDataSet* compositeData, svtkXMLDataElement* parent, int& writerIdx)
{
  if (!compositeData->IsA("svtkPartitionedDataSet"))
  {
    svtkErrorMacro("Unsupported composite dataset type: " << compositeData->GetClassName() << ".");
    return 0;
  }

  // Write each input.
  svtkSmartPointer<svtkDataObjectTreeIterator> iter;
  iter.TakeReference(svtkDataObjectTree::SafeDownCast(compositeData)->NewTreeIterator());
  iter->VisitOnlyLeavesOff();
  iter->TraverseSubTreeOff();
  iter->SkipEmptyNodesOff();
  int toBeWritten = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    toBeWritten++;
  }

  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);

  int index = 0;
  int RetVal = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), index++)
  {
    svtkDataObject* curDO = iter->GetCurrentDataObject();

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
  return RetVal;
}

//----------------------------------------------------------------------------
void svtkXMLPartitionedDataSetWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
