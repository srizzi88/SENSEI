/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLMultiBlockDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLMultiBlockDataWriter.h"

#include "svtkDataObjectTreeIterator.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkXMLDataElement.h"

svtkStandardNewMacro(svtkXMLMultiBlockDataWriter);
//----------------------------------------------------------------------------
svtkXMLMultiBlockDataWriter::svtkXMLMultiBlockDataWriter() = default;

//----------------------------------------------------------------------------
svtkXMLMultiBlockDataWriter::~svtkXMLMultiBlockDataWriter() = default;

//----------------------------------------------------------------------------
int svtkXMLMultiBlockDataWriter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkMultiBlockDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLMultiBlockDataWriter::WriteComposite(
  svtkCompositeDataSet* compositeData, svtkXMLDataElement* parent, int& writerIdx)
{
  if (!(compositeData->IsA("svtkMultiBlockDataSet") || compositeData->IsA("svtkMultiPieceDataSet")))
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
    const char* name = nullptr;
    if (iter->HasCurrentMetaData())
    {
      name = iter->GetCurrentMetaData()->Get(svtkCompositeDataSet::NAME());
    }

    if (curDO && curDO->IsA("svtkCompositeDataSet"))
    // if node is a supported composite dataset
    // note in structure file and recurse.
    {
      svtkXMLDataElement* tag = svtkXMLDataElement::New();
      if (name)
      {
        tag->SetAttribute("name", name);
      }

      if (curDO->IsA("svtkMultiPieceDataSet"))
      {
        tag->SetName("Piece");
        tag->SetIntAttribute("index", index);
      }
      else if (curDO->IsA("svtkMultiBlockDataSet"))
      {
        tag->SetName("Block");
        tag->SetIntAttribute("index", index);
      }
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
      if (name)
      {
        datasetXML->SetAttribute("name", name);
      }
      svtkStdString fileName = this->CreatePieceFileName(writerIdx);

      this->SetProgressRange(progressRange, writerIdx, toBeWritten);
      if (this->WriteNonCompositeData(curDO, datasetXML, writerIdx, fileName.c_str()))
      {
        parent->AddNestedElement(datasetXML);
        RetVal = 1;
      }
      datasetXML->Delete();
    }
  }
  return RetVal;
}

//----------------------------------------------------------------------------
void svtkXMLMultiBlockDataWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
