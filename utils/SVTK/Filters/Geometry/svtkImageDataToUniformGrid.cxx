/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageDataToUniformGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkImageDataToUniformGrid.h"

#include "svtkCellData.h"
#include "svtkDataObjectTree.h"
#include "svtkDataObjectTreeIterator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUniformGrid.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkImageDataToUniformGrid);

//----------------------------------------------------------------------------
svtkImageDataToUniformGrid::svtkImageDataToUniformGrid()
{
  this->Reverse = 0;
}

//----------------------------------------------------------------------------
svtkImageDataToUniformGrid::~svtkImageDataToUniformGrid() = default;

//----------------------------------------------------------------------------
void svtkImageDataToUniformGrid::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Reverse: " << this->Reverse << "\n";
}

//----------------------------------------------------------------------------
int svtkImageDataToUniformGrid::RequestDataObject(
  svtkInformation*, svtkInformationVector** inV, svtkInformationVector* outV)
{
  svtkInformation* inInfo = inV[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return SVTK_ERROR;
  }

  svtkInformation* outInfo = outV->GetInformationObject(0);

  if (svtkDataObjectTree* input = svtkDataObjectTree::GetData(inInfo))
  { // multiblock data sets
    svtkDataObjectTree* output = svtkDataObjectTree::GetData(outInfo);
    if (!output)
    {
      output = input->NewInstance();
      outInfo->Set(svtkDataObject::DATA_OBJECT(), output);
      this->GetOutputPortInformation(0)->Set(
        svtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->Delete();
    }
    return SVTK_OK;
  }
  if (svtkImageData::GetData(inInfo) != nullptr)
  {
    svtkUniformGrid* output = svtkUniformGrid::New();
    outInfo->Set(svtkDataObject::DATA_OBJECT(), output);
    this->GetOutputPortInformation(0)->Set(
      svtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
    output->Delete();

    return SVTK_OK;
  }

  svtkErrorMacro(
    "Don't know how to handle input of type " << svtkDataObject::GetData(inInfo)->GetClassName());
  return SVTK_ERROR;
}

//----------------------------------------------------------------------------
int svtkImageDataToUniformGrid::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outV)
{
  svtkDataObject* input = this->GetInput();
  svtkInformation* outInfo = outV->GetInformationObject(0);
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());

  svtkInformation* inArrayInfo = this->GetInputArrayInformation(0);
  if (!inArrayInfo)
  {
    svtkErrorMacro("Problem getting array to process.");
    return 0;
  }
  int association = -1;
  if (!inArrayInfo->Has(svtkDataObject::FIELD_ASSOCIATION()))
  {
    svtkErrorMacro("Unable to query field association for the scalar.");
    return 0;
  }
  association = inArrayInfo->Get(svtkDataObject::FIELD_ASSOCIATION());

  const char* arrayName = inArrayInfo->Get(svtkDataObject::FIELD_NAME());
  if (!arrayName)
  {
    svtkErrorMacro("Problem getting array name to process.");
    return 0;
  }

  if (svtkImageData* inImageData = svtkImageData::SafeDownCast(input))
  {
    return this->Process(inImageData, association, arrayName, svtkUniformGrid::SafeDownCast(output));
  }
  svtkDataObjectTree* inMB = svtkDataObjectTree::SafeDownCast(input);
  svtkDataObjectTree* outMB = svtkDataObjectTree::SafeDownCast(output);
  outMB->CopyStructure(inMB);
  svtkDataObjectTreeIterator* iter = inMB->NewTreeIterator();
  iter->VisitOnlyLeavesOn();
  iter->TraverseSubTreeOn();
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    if (svtkImageData* inImageData = svtkImageData::SafeDownCast(iter->GetCurrentDataObject()))
    {
      svtkNew<svtkUniformGrid> outUniformGrid;
      if (this->Process(inImageData, association, arrayName, outUniformGrid) != SVTK_OK)
      {
        iter->Delete();
        return SVTK_ERROR;
      }
      outMB->SetDataSetFrom(iter, outUniformGrid);
    }
    else
    { // not a uniform grid so we just shallow copy from input to output
      outMB->SetDataSetFrom(iter, iter->GetCurrentDataObject());
    }
  }
  iter->Delete();

  return SVTK_OK;
}

//-----------------------------------------------------------------------------
int svtkImageDataToUniformGrid::FillInputPortInformation(int port, svtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);

  // According to the documentation this is the way to append additional
  // input data set type since SVTK 5.2.
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObjectTree");
  return SVTK_OK;
}

//----------------------------------------------------------------------------
int svtkImageDataToUniformGrid::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataObject");

  return SVTK_OK;
}

//----------------------------------------------------------------------------
int svtkImageDataToUniformGrid::Process(
  svtkImageData* input, int association, const char* arrayName, svtkUniformGrid* output)
{
  if (svtkUniformGrid* uniformGrid = svtkUniformGrid::SafeDownCast(input))
  {
    output->ShallowCopy(uniformGrid);
  }
  else
  {
    output->ShallowCopy(input);
  }

  svtkDataArray* inScalars = nullptr;
  if (association == svtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    inScalars = input->GetPointData()->GetArray(arrayName);
  }
  else if (association == svtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    inScalars = input->GetCellData()->GetArray(arrayName);
  }
  else
  {
    svtkErrorMacro("Wrong association type: " << association);
    return SVTK_ERROR;
  }

  if (!inScalars)
  {
    svtkErrorMacro("No scalar data to use for blanking.");
    return SVTK_ERROR;
  }
  else if (inScalars->GetNumberOfComponents() != 1)
  {
    svtkErrorMacro("Scalar data must be a single component array.");
    return SVTK_ERROR;
  }

  svtkNew<svtkUnsignedCharArray> blankingArray;
  blankingArray->SetNumberOfTuples(inScalars->GetNumberOfTuples());
  blankingArray->SetNumberOfComponents(1);
  blankingArray->FillValue(0);
  blankingArray->SetName(svtkDataSetAttributes::GhostArrayName());

  unsigned char value1;
  unsigned char value2;
  if (association == svtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    if (this->Reverse)
    {
      value1 = 0;
      value2 = svtkDataSetAttributes::HIDDENCELL;
    }
    else
    {
      value1 = svtkDataSetAttributes::HIDDENCELL;
      value2 = 0;
    }
  }
  else
  {
    if (this->Reverse)
    {
      value1 = 0;
      value2 = svtkDataSetAttributes::HIDDENPOINT;
    }
    else
    {
      value1 = svtkDataSetAttributes::HIDDENPOINT;
      value2 = 0;
    }
  }

  for (svtkIdType i = 0; i < blankingArray->GetNumberOfTuples(); i++)
  {
    double scalarValue = inScalars->GetTuple1(i);
    char value = ((scalarValue > -1) && (scalarValue < 1)) ? value1 : value2;
    blankingArray->SetValue(i, value);
  }

  if (association == svtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    output->GetPointData()->AddArray(blankingArray);
  }
  else
  {
    output->GetCellData()->AddArray(blankingArray);
  }

  return SVTK_OK;
}
