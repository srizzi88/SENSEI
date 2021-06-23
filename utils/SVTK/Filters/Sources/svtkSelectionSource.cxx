/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSelectionSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSelectionSource.h"

#include "svtkCommand.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStringArray.h"
#include "svtkUnsignedIntArray.h"

#include <set>
#include <vector>

svtkStandardNewMacro(svtkSelectionSource);

class svtkSelectionSourceInternals
{
public:
  typedef std::set<svtkIdType> IDSetType;
  typedef std::vector<IDSetType> IDsType;
  IDsType IDs;

  typedef std::set<svtkStdString> StringIDSetType;
  typedef std::vector<StringIDSetType> StringIDsType;
  StringIDsType StringIDs;

  std::vector<double> Thresholds;
  std::vector<double> Locations;
  IDSetType Blocks;
  double Frustum[32];
};

//----------------------------------------------------------------------------
svtkSelectionSource::svtkSelectionSource()
{
  this->SetNumberOfInputPorts(0);
  this->Internal = new svtkSelectionSourceInternals;

  this->ContentType = svtkSelectionNode::INDICES;
  this->FieldType = svtkSelectionNode::CELL;
  this->ContainingCells = 1;
  this->Inverse = 0;
  this->ArrayName = nullptr;
  this->ArrayComponent = 0;
  for (int cc = 0; cc < 32; cc++)
  {
    this->Internal->Frustum[cc] = 0;
  }
  this->CompositeIndex = -1;
  this->HierarchicalLevel = -1;
  this->HierarchicalIndex = -1;
  this->QueryString = nullptr;
  this->NumberOfLayers = 0;
}

//----------------------------------------------------------------------------
svtkSelectionSource::~svtkSelectionSource()
{
  delete this->Internal;
  delete[] this->ArrayName;
  delete[] this->QueryString;
}

//----------------------------------------------------------------------------
void svtkSelectionSource::RemoveAllIDs()
{
  this->Internal->IDs.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkSelectionSource::RemoveAllStringIDs()
{
  this->Internal->StringIDs.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkSelectionSource::RemoveAllLocations()
{
  this->Internal->Locations.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkSelectionSource::RemoveAllThresholds()
{
  this->Internal->Thresholds.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkSelectionSource::AddID(svtkIdType proc, svtkIdType id)
{
  // proc == -1 means all processes. All other are stored at index proc+1
  proc++;

  if (proc >= (svtkIdType)this->Internal->IDs.size())
  {
    this->Internal->IDs.resize(proc + 1);
  }
  svtkSelectionSourceInternals::IDSetType& idSet = this->Internal->IDs[proc];
  idSet.insert(id);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkSelectionSource::AddStringID(svtkIdType proc, const char* id)
{
  // proc == -1 means all processes. All other are stored at index proc+1
  proc++;

  if (proc >= (svtkIdType)this->Internal->StringIDs.size())
  {
    this->Internal->StringIDs.resize(proc + 1);
  }
  svtkSelectionSourceInternals::StringIDSetType& idSet = this->Internal->StringIDs[proc];
  idSet.insert(id);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkSelectionSource::AddLocation(double x, double y, double z)
{
  this->Internal->Locations.push_back(x);
  this->Internal->Locations.push_back(y);
  this->Internal->Locations.push_back(z);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkSelectionSource::AddThreshold(double min, double max)
{
  this->Internal->Thresholds.push_back(min);
  this->Internal->Thresholds.push_back(max);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkSelectionSource::SetFrustum(double* vertices)
{
  for (int cc = 0; cc < 32; cc++)
  {
    if (vertices[cc] != this->Internal->Frustum[cc])
    {
      memcpy(this->Internal->Frustum, vertices, 32 * sizeof(double));
      this->Modified();
      break;
    }
  }
}

//----------------------------------------------------------------------------
void svtkSelectionSource::AddBlock(svtkIdType block)
{
  this->Internal->Blocks.insert(block);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkSelectionSource::RemoveAllBlocks()
{
  this->Internal->Blocks.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkSelectionSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ContentType: ";
  switch (this->ContentType)
  {
    case svtkSelectionNode::SELECTIONS:
      os << "SELECTIONS";
      break;
    case svtkSelectionNode::GLOBALIDS:
      os << "GLOBALIDS";
      break;
    case svtkSelectionNode::VALUES:
      os << "VALUES";
      break;
    case svtkSelectionNode::INDICES:
      os << "INDICES";
      break;
    case svtkSelectionNode::FRUSTUM:
      os << "FRUSTUM";
      break;
    case svtkSelectionNode::LOCATIONS:
      os << "LOCATIONS";
      break;
    case svtkSelectionNode::THRESHOLDS:
      os << "THRESHOLDS";
      break;
    case svtkSelectionNode::BLOCKS:
      os << "BLOCKS";
      break;
    default:
      os << "UNKNOWN";
  }
  os << endl;

  os << indent << "FieldType: ";
  switch (this->FieldType)
  {
    case svtkSelectionNode::CELL:
      os << "CELL";
      break;
    case svtkSelectionNode::POINT:
      os << "POINT";
      break;
    case svtkSelectionNode::FIELD:
      os << "FIELD";
      break;
    case svtkSelectionNode::VERTEX:
      os << "VERTEX";
      break;
    case svtkSelectionNode::EDGE:
      os << "EDGE";
      break;
    case svtkSelectionNode::ROW:
      os << "ROW";
      break;
    default:
      os << "UNKNOWN";
  }
  os << endl;

  os << indent << "ContainingCells: ";
  os << (this->ContainingCells ? "CELLS" : "POINTS") << endl;
  os << indent << "Inverse: " << this->Inverse << endl;
  os << indent << "ArrayName: " << (this->ArrayName ? this->ArrayName : "nullptr") << endl;
  os << indent << "ArrayComponent: " << this->ArrayComponent << endl;
  os << indent << "CompositeIndex: " << this->CompositeIndex << endl;
  os << indent << "HierarchicalLevel: " << this->HierarchicalLevel << endl;
  os << indent << "HierarchicalIndex: " << this->HierarchicalIndex << endl;
  os << indent << "QueryString: " << (this->QueryString ? this->QueryString : "nullptr") << endl;
  os << indent << "NumberOfLayers: " << this->NumberOfLayers << endl;
}

//----------------------------------------------------------------------------
int svtkSelectionSource::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int svtkSelectionSource::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkSelection* outputSel = svtkSelection::GetData(outputVector);
  svtkNew<svtkSelectionNode> output;
  outputSel->AddNode(output);
  svtkInformation* oProperties = output->GetProperties();

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  int piece = 0;
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
  {
    piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  }

  if (this->CompositeIndex >= 0)
  {
    oProperties->Set(svtkSelectionNode::COMPOSITE_INDEX(), this->CompositeIndex);
  }

  if (this->HierarchicalLevel >= 0 && this->HierarchicalIndex >= 0)
  {
    oProperties->Set(svtkSelectionNode::HIERARCHICAL_LEVEL(), this->HierarchicalLevel);
    oProperties->Set(svtkSelectionNode::HIERARCHICAL_INDEX(), this->HierarchicalIndex);
  }

  // First look for string ids.
  if (((this->ContentType == svtkSelectionNode::GLOBALIDS) ||
        (this->ContentType == svtkSelectionNode::PEDIGREEIDS) ||
        (this->ContentType == svtkSelectionNode::INDICES) ||
        (this->ContentType == svtkSelectionNode::VALUES)) &&
    !this->Internal->StringIDs.empty())
  {
    oProperties->Set(svtkSelectionNode::CONTENT_TYPE(), this->ContentType);
    oProperties->Set(svtkSelectionNode::FIELD_TYPE(), this->FieldType);

    svtkNew<svtkStringArray> selectionList;
    output->SetSelectionList(selectionList);

    // Number of selected items common to all pieces
    svtkIdType numCommonElems = 0;
    if (!this->Internal->StringIDs.empty())
    {
      numCommonElems = static_cast<svtkIdType>(this->Internal->StringIDs[0].size());
    }
    if (piece + 1 >= (int)this->Internal->StringIDs.size() && numCommonElems == 0)
    {
      svtkDebugMacro("No selection for piece: " << piece);
    }
    else
    {
      // idx == 0 is the list for all pieces
      // idx == piece+1 is the list for the current piece
      size_t pids[2];
      pids[0] = 0;
      pids[1] = static_cast<size_t>(piece + 1);
      for (int i = 0; i < 2; i++)
      {
        size_t idx = pids[i];
        if (idx >= this->Internal->StringIDs.size())
        {
          continue;
        }

        svtkSelectionSourceInternals::StringIDSetType& selSet = this->Internal->StringIDs[idx];

        if (!selSet.empty())
        {
          // Create the selection list
          selectionList->SetNumberOfTuples(static_cast<svtkIdType>(selSet.size()));
          // iterate over ids and insert to the selection list
          svtkSelectionSourceInternals::StringIDSetType::iterator iter = selSet.begin();
          for (svtkIdType idx2 = 0; iter != selSet.end(); ++iter, ++idx2)
          {
            selectionList->SetValue(idx2, *iter);
          }
        }
      }
    }
  }

  // If no string ids, use integer ids.
  if (((this->ContentType == svtkSelectionNode::GLOBALIDS) ||
        (this->ContentType == svtkSelectionNode::PEDIGREEIDS) ||
        (this->ContentType == svtkSelectionNode::INDICES) ||
        (this->ContentType == svtkSelectionNode::VALUES)) &&
    this->Internal->StringIDs.empty())
  {
    oProperties->Set(svtkSelectionNode::CONTENT_TYPE(), this->ContentType);
    oProperties->Set(svtkSelectionNode::FIELD_TYPE(), this->FieldType);

    svtkNew<svtkIdTypeArray> selectionList;
    output->SetSelectionList(selectionList);

    // Number of selected items common to all pieces
    svtkIdType numCommonElems = 0;
    if (!this->Internal->IDs.empty())
    {
      numCommonElems = static_cast<svtkIdType>(this->Internal->IDs[0].size());
    }
    if (piece + 1 >= (int)this->Internal->IDs.size() && numCommonElems == 0)
    {
      svtkDebugMacro("No selection for piece: " << piece);
    }
    else
    {
      // idx == 0 is the list for all pieces
      // idx == piece+1 is the list for the current piece
      size_t pids[2] = { static_cast<size_t>(0), static_cast<size_t>(piece + 1) };
      for (int i = 0; i < 2; i++)
      {
        size_t idx = pids[i];
        if (idx >= this->Internal->IDs.size())
        {
          continue;
        }

        svtkSelectionSourceInternals::IDSetType& selSet = this->Internal->IDs[idx];

        if (!selSet.empty())
        {
          // Create the selection list
          selectionList->SetNumberOfTuples(static_cast<svtkIdType>(selSet.size()));
          // iterate over ids and insert to the selection list
          svtkSelectionSourceInternals::IDSetType::iterator iter = selSet.begin();
          for (svtkIdType idx2 = 0; iter != selSet.end(); ++iter, ++idx2)
          {
            selectionList->SetValue(idx2, *iter);
          }
        }
      }
    }
  }

  if (this->ContentType == svtkSelectionNode::LOCATIONS)
  {
    oProperties->Set(svtkSelectionNode::CONTENT_TYPE(), this->ContentType);
    oProperties->Set(svtkSelectionNode::FIELD_TYPE(), this->FieldType);
    // Create the selection list
    svtkNew<svtkDoubleArray> selectionList;
    selectionList->SetNumberOfComponents(3);
    selectionList->SetNumberOfValues(static_cast<svtkIdType>(this->Internal->Locations.size()));

    std::vector<double>::iterator iter = this->Internal->Locations.begin();
    for (svtkIdType cc = 0; iter != this->Internal->Locations.end(); ++iter, ++cc)
    {
      selectionList->SetValue(cc, *iter);
    }

    output->SetSelectionList(selectionList);
  }

  if (this->ContentType == svtkSelectionNode::THRESHOLDS)
  {
    oProperties->Set(svtkSelectionNode::CONTENT_TYPE(), this->ContentType);
    oProperties->Set(svtkSelectionNode::FIELD_TYPE(), this->FieldType);
    oProperties->Set(svtkSelectionNode::COMPONENT_NUMBER(), this->ArrayComponent);
    // Create the selection list
    svtkNew<svtkDoubleArray> selectionList;
    selectionList->SetNumberOfComponents(2);
    selectionList->SetNumberOfValues(static_cast<svtkIdType>(this->Internal->Thresholds.size()));

    std::vector<double>::iterator iter = this->Internal->Thresholds.begin();
    for (svtkIdType cc = 0; iter != this->Internal->Thresholds.end(); ++iter, ++cc)
    {
      selectionList->SetTypedComponent(cc, 0, *iter);
      ++iter;
      selectionList->SetTypedComponent(cc, 1, *iter);
    }

    output->SetSelectionList(selectionList);
  }

  if (this->ContentType == svtkSelectionNode::FRUSTUM)
  {
    oProperties->Set(svtkSelectionNode::CONTENT_TYPE(), this->ContentType);
    oProperties->Set(svtkSelectionNode::FIELD_TYPE(), this->FieldType);
    // Create the selection list
    svtkNew<svtkDoubleArray> selectionList;
    selectionList->SetNumberOfComponents(4);
    selectionList->SetNumberOfTuples(8);
    for (svtkIdType cc = 0; cc < 32; cc++)
    {
      selectionList->SetValue(cc, this->Internal->Frustum[cc]);
    }

    output->SetSelectionList(selectionList);
  }

  if (this->ContentType == svtkSelectionNode::BLOCKS)
  {
    oProperties->Set(svtkSelectionNode::CONTENT_TYPE(), this->ContentType);
    oProperties->Set(svtkSelectionNode::FIELD_TYPE(), this->FieldType);
    svtkNew<svtkUnsignedIntArray> selectionList;
    selectionList->SetNumberOfComponents(1);
    selectionList->SetNumberOfTuples(static_cast<svtkIdType>(this->Internal->Blocks.size()));
    svtkSelectionSourceInternals::IDSetType::iterator iter;
    svtkIdType cc = 0;
    for (iter = this->Internal->Blocks.begin(); iter != this->Internal->Blocks.end(); ++iter, ++cc)
    {
      selectionList->SetValue(cc, *iter);
    }
    output->SetSelectionList(selectionList);
  }

  if (this->ContentType == svtkSelectionNode::QUERY)
  {
    oProperties->Set(svtkSelectionNode::CONTENT_TYPE(), this->ContentType);
    oProperties->Set(svtkSelectionNode::FIELD_TYPE(), this->FieldType);
    output->SetQueryString(this->QueryString);
  }

  if (this->ContentType == svtkSelectionNode::USER)
  {
    svtkErrorMacro("User-supplied, application-specific selections are not supported.");
    return 0;
  }

  oProperties->Set(svtkSelectionNode::CONTAINING_CELLS(), this->ContainingCells);

  oProperties->Set(svtkSelectionNode::INVERSE(), this->Inverse);

  if (output->GetSelectionList())
  {
    output->GetSelectionList()->SetName(this->ArrayName);
  }
  oProperties->Set(svtkSelectionNode::CONNECTED_LAYERS(), this->NumberOfLayers);
  return 1;
}
