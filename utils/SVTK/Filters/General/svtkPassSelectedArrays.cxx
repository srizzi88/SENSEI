/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPassSelectedArrays.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPassSelectedArrays.h"

#include "svtkCommand.h"
#include "svtkDataArraySelection.h"
#include "svtkDataSetAttributes.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkPassSelectedArrays);
//----------------------------------------------------------------------------
svtkPassSelectedArrays::svtkPassSelectedArrays()
  : Enabled(true)
{
  for (int cc = 0; cc < svtkDataObject::NUMBER_OF_ASSOCIATIONS; ++cc)
  {
    if (cc != svtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS)
    {
      this->ArraySelections[cc] = svtkSmartPointer<svtkDataArraySelection>::New();
      this->ArraySelections[cc]->AddObserver(
        svtkCommand::ModifiedEvent, this, &svtkPassSelectedArrays::Modified);
    }
    else
    {
      this->ArraySelections[cc] = nullptr;
    }
  }
}

//----------------------------------------------------------------------------
svtkPassSelectedArrays::~svtkPassSelectedArrays() {}

//----------------------------------------------------------------------------
svtkDataArraySelection* svtkPassSelectedArrays::GetArraySelection(int association)
{
  if (association >= 0 && association < svtkDataObject::NUMBER_OF_ASSOCIATIONS)
  {
    return this->ArraySelections[association];
  }

  return nullptr;
}

//----------------------------------------------------------------------------
int svtkPassSelectedArrays::FillInputPortInformation(int, svtkInformation* info)
{
  // Skip composite data sets so that executives will treat this as a simple filter
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGenericDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkHyperTreeGrid");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  return 1;
}

//----------------------------------------------------------------------------
int svtkPassSelectedArrays::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  auto input = svtkDataObject::GetData(inputVector[0], 0);
  auto output = svtkDataObject::GetData(outputVector, 0);
  output->ShallowCopy(input);

  if (!this->Enabled)
  {
    return 1;
  }

  // now filter arrays for each of the associations.
  for (int association = 0; association < svtkDataObject::NUMBER_OF_ASSOCIATIONS; ++association)
  {
    if (association == svtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS)
    {
      continue;
    }

    auto inFD = input->GetAttributesAsFieldData(association);
    auto outFD = output->GetAttributesAsFieldData(association);
    auto selection = this->GetArraySelection(association);
    if (!inFD || !outFD || !selection)
    {
      continue;
    }

    auto inDSA = svtkDataSetAttributes::SafeDownCast(inFD);
    auto outDSA = svtkDataSetAttributes::SafeDownCast(outFD);

    outFD->Initialize();
    for (int idx = 0, max = inFD->GetNumberOfArrays(); idx < max; ++idx)
    {
      auto inarray = inFD->GetAbstractArray(idx);
      if (inarray && inarray->GetName())
      {
        if (selection->ArrayIsEnabled(inarray->GetName()) ||
          /** ghost array is passed unless explicitly disabled **/
          (strcmp(inarray->GetName(), svtkDataSetAttributes::GhostArrayName()) == 0 &&
            selection->ArrayExists(svtkDataSetAttributes::GhostArrayName()) == 0))
        {
          outFD->AddArray(inarray);

          // preserve attribute type flags.
          for (int attr = 0; inDSA && outDSA && (attr < svtkDataSetAttributes::NUM_ATTRIBUTES);
               ++attr)
          {
            if (inDSA->GetAbstractAttribute(attr) == inarray)
            {
              outDSA->SetAttribute(inarray, attr);
            }
          }
        }
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkPassSelectedArrays::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "PointDataArraySelection: " << endl;
  this->GetPointDataArraySelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "CellDataArraySelection: " << endl;
  this->GetCellDataArraySelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "FieldDataArraySelection: " << endl;
  this->GetFieldDataArraySelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "VertexDataArraySelection: " << endl;
  this->GetVertexDataArraySelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "EdgeDataArraySelection: " << endl;
  this->GetEdgeDataArraySelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "RowDataArraySelection: " << endl;
  this->GetRowDataArraySelection()->PrintSelf(os, indent.GetNextIndent());
}
