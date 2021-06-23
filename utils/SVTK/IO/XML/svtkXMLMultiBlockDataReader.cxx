/*=========================================================================

  Program:   ParaView
  Module:    svtkXMLMultiBlockDataReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLMultiBlockDataReader.h"

#include "svtkCompositeDataPipeline.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataArraySelection.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkXMLDataElement.h"

svtkStandardNewMacro(svtkXMLMultiBlockDataReader);

//----------------------------------------------------------------------------
svtkXMLMultiBlockDataReader::svtkXMLMultiBlockDataReader() = default;

//----------------------------------------------------------------------------
svtkXMLMultiBlockDataReader::~svtkXMLMultiBlockDataReader() = default;

//----------------------------------------------------------------------------
void svtkXMLMultiBlockDataReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int svtkXMLMultiBlockDataReader::FillOutputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMultiBlockDataSet");
  return 1;
}

//----------------------------------------------------------------------------
const char* svtkXMLMultiBlockDataReader::GetDataSetName()
{
  return "svtkMultiBlockDataSet";
}

//----------------------------------------------------------------------------
// This version does not support multiblock of multiblocks, so our work is
// simple.
void svtkXMLMultiBlockDataReader::ReadVersion0(svtkXMLDataElement* element,
  svtkCompositeDataSet* composite, const char* filePath, unsigned int& dataSetIndex)
{
  svtkMultiBlockDataSet* mblock = svtkMultiBlockDataSet::SafeDownCast(composite);
  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < numElems; ++cc)
  {
    svtkXMLDataElement* childXML = element->GetNestedElement(cc);
    if (!childXML || !childXML->GetName() || strcmp(childXML->GetName(), "DataSet") != 0)
    {
      continue;
    }
    int group = 0;
    int index = 0;
    if (childXML->GetScalarAttribute("group", group) &&
      childXML->GetScalarAttribute("dataset", index))
    {
      svtkSmartPointer<svtkDataSet> dataset;
      if (this->ShouldReadDataSet(dataSetIndex))
      {
        dataset.TakeReference(this->ReadDataset(childXML, filePath));
      }
      svtkMultiBlockDataSet* block = svtkMultiBlockDataSet::SafeDownCast(mblock->GetBlock(group));
      if (!block)
      {
        block = svtkMultiBlockDataSet::New();
        mblock->SetBlock(group, block);
        block->Delete();
      }
      block->SetBlock(index, dataset);
    }
    dataSetIndex++;
  }
}

//----------------------------------------------------------------------------
void svtkXMLMultiBlockDataReader::ReadComposite(svtkXMLDataElement* element,
  svtkCompositeDataSet* composite, const char* filePath, unsigned int& dataSetIndex)
{
  svtkMultiBlockDataSet* mblock = svtkMultiBlockDataSet::SafeDownCast(composite);
  svtkMultiPieceDataSet* mpiece = svtkMultiPieceDataSet::SafeDownCast(composite);
  if (!mblock && !mpiece)
  {
    svtkErrorMacro("Unsupported composite dataset.");
    return;
  }

  if (this->GetFileMajorVersion() < 1)
  {
    // Read legacy file.
    this->ReadVersion0(element, composite, filePath, dataSetIndex);
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
      if (mblock)
      {
        index = mblock->GetNumberOfBlocks();
      }
      else if (mpiece)
      {
        index = mpiece->GetNumberOfPieces();
      }
    }
    // child is a leaf node, read and insert.
    const char* tagName = childXML->GetName();
    if (strcmp(tagName, "DataSet") == 0)
    {
      svtkSmartPointer<svtkDataObject> childDS;
      const char* name = nullptr;
      if (this->ShouldReadDataSet(dataSetIndex))
      {
        // Read
        childDS.TakeReference(this->ReadDataObject(childXML, filePath));
        name = childXML->GetAttribute("name");
      }
      // insert
      if (mblock)
      {
        mblock->SetBlock(index, childDS);
        mblock->GetMetaData(index)->Set(svtkCompositeDataSet::NAME(), name);
      }
      else if (mpiece)
      {
        mpiece->SetPiece(index, childDS);
        mpiece->GetMetaData(index)->Set(svtkCompositeDataSet::NAME(), name);
      }
      dataSetIndex++;
    }
    // Child is a multiblock dataset itself. Create it.
    else if (mblock != nullptr && strcmp(tagName, "Block") == 0)
    {
      svtkMultiBlockDataSet* childDS = svtkMultiBlockDataSet::New();
      this->ReadComposite(childXML, childDS, filePath, dataSetIndex);
      const char* name = childXML->GetAttribute("name");
      mblock->SetBlock(index, childDS);
      mblock->GetMetaData(index)->Set(svtkCompositeDataSet::NAME(), name);
      childDS->Delete();
    }
    // Child is a multipiece dataset. Create it.
    else if (mblock != nullptr && strcmp(tagName, "Piece") == 0)
    {
      // Look ahead to see if there is a nested Piece structure, which can happen when
      // the dataset pieces in a svtkMultiPieceDataSet are themselves split into
      // svtkMultiPieceDataSets when saved in parallel.
      svtkCompositeDataSet* childDS;
      if (childXML->FindNestedElementWithName("Piece"))
      {
        // Create a multiblock to handle a multipiece child
        childDS = svtkMultiBlockDataSet::New();
      }
      else
      {
        // Child is not multipiece, so it is safe to create a svtkMultiPieceDataSet
        childDS = svtkMultiPieceDataSet::New();
      }
      this->ReadComposite(childXML, childDS, filePath, dataSetIndex);
      const char* name = childXML->GetAttribute("name");
      mblock->SetBlock(index, childDS);
      mblock->GetMetaData(index)->Set(svtkCompositeDataSet::NAME(), name);
      childDS->Delete();
    }
    else
    {
      svtkErrorMacro("Syntax error in file.");
      return;
    }
  }
}

namespace
{
svtkInformation* CreateMetaDataIfNecessary(
  svtkMultiBlockDataSet* mblock, svtkMultiPieceDataSet* mpiece, int index)
{
  svtkInformation* piece_metadata = nullptr;
  if (mblock)
  {
    mblock->SetBlock(index, nullptr);
    piece_metadata = mblock->GetMetaData(index);
  }
  else if (mpiece)
  {
    mpiece->SetPiece(index, nullptr);
    piece_metadata = mpiece->GetMetaData(index);
  }
  return piece_metadata;
}

}

//----------------------------------------------------------------------------
int svtkXMLMultiBlockDataReader::FillMetaData(svtkCompositeDataSet* metadata,
  svtkXMLDataElement* element, const std::string& filePath, unsigned int& dataSetIndex)
{
  svtkMultiBlockDataSet* mblock = svtkMultiBlockDataSet::SafeDownCast(metadata);
  svtkMultiPieceDataSet* mpiece = svtkMultiPieceDataSet::SafeDownCast(metadata);

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
      if (mblock)
      {
        index = mblock->GetNumberOfBlocks();
      }
      else if (mpiece)
      {
        index = mpiece->GetNumberOfPieces();
      }
    }
    // child is a leaf node, read and insert.
    const char* tagName = childXML->GetName();
    if (strcmp(tagName, "DataSet") == 0)
    {
      svtkInformation* piece_metadata = CreateMetaDataIfNecessary(mblock, mpiece, index);
      double bounding_box[6];
      if (childXML->GetVectorAttribute("bounding_box", 6, bounding_box) == 6)
      {
        if (piece_metadata)
        {
          piece_metadata->Set(svtkDataObject::BOUNDING_BOX(), bounding_box, 6);
        }
      }
      int extent[6];
      if (childXML->GetVectorAttribute("extent", 6, extent) == 6)
      {
        if (piece_metadata)
        {
          piece_metadata->Set(svtkDataObject::PIECE_EXTENT(), extent, 6);
        }
      }
      if (this->ShouldReadDataSet(dataSetIndex))
      {
        this->SyncDataArraySelections(this, childXML, filePath);
      }
      dataSetIndex++;
    }
    // Child is a multiblock dataset itself. Create it.
    else if (mblock != nullptr && strcmp(tagName, "Block") == 0)
    {
      svtkMultiBlockDataSet* childDS = svtkMultiBlockDataSet::New();
      this->FillMetaData(childDS, childXML, filePath, dataSetIndex);
      if (mblock)
      {
        mblock->SetBlock(index, childDS);
      }
      else if (mpiece)
      {
        svtkErrorMacro("Multipiece data can't have composite children.");
        return 0;
      }
      childDS->Delete();
    }
    // Child is a multipiece dataset. Create it.
    else if (mblock != nullptr && strcmp(tagName, "Piece") == 0)
    {
      // Look ahead to see if there is a nested Piece structure, which can happen when
      // the dataset pieces in a svtkMultiPieceDataSet are themselves split into
      // svtkMultiPieceDataSets when saved in parallel.
      svtkCompositeDataSet* childDS;
      if (childXML->FindNestedElementWithName("Piece"))
      {
        // Create a multiblock to handle a multipiece child
        childDS = svtkMultiBlockDataSet::New();
      }
      else
      {
        // Child is not multipiece, so it is safe to create a svtkMultiPieceDataSet
        childDS = svtkMultiPieceDataSet::New();
      }

      this->FillMetaData(childDS, childXML, filePath, dataSetIndex);
      mblock->SetBlock(index, childDS);
      childDS->Delete();
      int whole_extent[6];
      if (childXML->GetVectorAttribute("whole_extent", 6, whole_extent) == 6)
      {
        svtkInformation* piece_metadata = mblock->GetMetaData(index);
        piece_metadata->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), whole_extent, 6);
      }
    }
    else
    {
      svtkErrorMacro("Syntax error in file.");
      return 0;
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLMultiBlockDataReader::RequestInformation(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  this->Superclass::RequestInformation(request, inputVector, outputVector);

  if (this->GetFileMajorVersion() < 1)
  {
    return 1;
  }

  const std::string filePath = this->GetFilePath();
  svtkInformation* info = outputVector->GetInformationObject(0);
  svtkSmartPointer<svtkMultiBlockDataSet> metadata = svtkSmartPointer<svtkMultiBlockDataSet>::New();
  unsigned int dataSetIndex = 0;
  if (!this->FillMetaData(metadata, this->GetPrimaryElement(), filePath, dataSetIndex))
  {
    return 0;
  }
  info->Set(svtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA(), metadata);

  return 1;
}
