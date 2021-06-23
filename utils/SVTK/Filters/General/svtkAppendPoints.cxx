/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAppendPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAppendPoints.h"

#include "svtkDataSetAttributes.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"

#include <set>
#include <vector>

svtkStandardNewMacro(svtkAppendPoints);

//----------------------------------------------------------------------------
svtkAppendPoints::svtkAppendPoints()
{
  this->InputIdArrayName = nullptr;
  this->OutputPointsPrecision = svtkAlgorithm::DEFAULT_PRECISION;
}

//----------------------------------------------------------------------------
svtkAppendPoints::~svtkAppendPoints()
{
  this->SetInputIdArrayName(nullptr);
}

//----------------------------------------------------------------------------
// This method is much too long, and has to be broken up!
// Append data sets into single polygonal data set.
int svtkAppendPoints::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Find common array names.
  svtkIdType totalPoints = 0;
  int numInputs = this->GetNumberOfInputConnections(0);
  bool first = true;
  std::set<std::string> arrayNames;
  for (int idx = 0; idx < numInputs; ++idx)
  {
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(idx);
    svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
    if (input && input->GetNumberOfPoints() > 0)
    {
      totalPoints += input->GetNumberOfPoints();
      if (first)
      {
        int numArrays = input->GetPointData()->GetNumberOfArrays();
        for (int a = 0; a < numArrays; ++a)
        {
          arrayNames.insert(input->GetPointData()->GetAbstractArray(a)->GetName());
        }
        first = false;
      }
      else
      {
        std::set<std::string> toErase;
        std::set<std::string>::iterator it, itEnd;
        itEnd = arrayNames.end();
        for (it = arrayNames.begin(); it != itEnd; ++it)
        {
          if (!input->GetPointData()->GetAbstractArray(it->c_str()))
          {
            toErase.insert(*it);
          }
        }
        itEnd = toErase.end();
        for (it = toErase.begin(); it != itEnd; ++it)
        {
          arrayNames.erase(*it);
        }
      }
    }
  }

  std::vector<svtkSmartPointer<svtkPolyData> > inputs;
  for (int idx = 0; idx < numInputs; ++idx)
  {
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(idx);
    svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
    if (input && input->GetNumberOfPoints() > 0)
    {
      svtkSmartPointer<svtkPolyData> copy = svtkSmartPointer<svtkPolyData>::New();
      copy->SetPoints(input->GetPoints());
      std::set<std::string>::iterator it, itEnd;
      itEnd = arrayNames.end();
      for (it = arrayNames.begin(); it != itEnd; ++it)
      {
        copy->GetPointData()->AddArray(input->GetPointData()->GetAbstractArray(it->c_str()));
      }
      inputs.push_back(copy);
    }
    else
    {
      inputs.push_back(nullptr);
    }
  }

  svtkPointData* pd = nullptr;
  svtkIdType index = 0;
  svtkSmartPointer<svtkPoints> pts = svtkSmartPointer<svtkPoints>::New();

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    // The points in distinct inputs may be of differing precisions.
    pts->SetDataType(SVTK_FLOAT);
    for (size_t idx = 0; idx < inputs.size(); ++idx)
    {
      svtkPolyData* input = inputs[idx];

      // Set the desired precision to SVTK_DOUBLE if the precision of the
      // points in any of the inputs is SVTK_DOUBLE.
      if (input && input->GetPoints() && input->GetPoints()->GetDataType() == SVTK_DOUBLE)
      {
        pts->SetDataType(SVTK_DOUBLE);
        break;
      }
    }
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    pts->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    pts->SetDataType(SVTK_DOUBLE);
  }

  pts->SetNumberOfPoints(totalPoints);
  svtkSmartPointer<svtkIntArray> idArr;
  if (this->InputIdArrayName)
  {
    idArr = svtkSmartPointer<svtkIntArray>::New();
    idArr->SetName(this->InputIdArrayName);
    idArr->SetNumberOfTuples(totalPoints);
  }
  for (size_t idx = 0; idx < inputs.size(); ++idx)
  {
    svtkPolyData* input = inputs[idx];
    if (input)
    {
      svtkPointData* ipd = input->GetPointData();
      if (!pd)
      {
        pd = output->GetPointData();
        pd->CopyAllocate(ipd, totalPoints);
      }
      svtkIdType numPoints = input->GetNumberOfPoints();
      for (svtkIdType i = 0; i < numPoints; ++i)
      {
        pd->CopyData(ipd, i, index);
        pts->InsertPoint(index, input->GetPoint(i));
        if (this->InputIdArrayName)
        {
          idArr->InsertValue(index, static_cast<int>(idx));
        }
        ++index;
      }
    }
  }
  output->SetPoints(pts);
  if (this->InputIdArrayName)
  {
    output->GetPointData()->AddArray(idArr);
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkAppendPoints::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent
     << "InputIdArrayName: " << (this->InputIdArrayName ? this->InputIdArrayName : "(none)") << endl
     << indent << "Output Points Precision: " << this->OutputPointsPrecision << endl;
}

//----------------------------------------------------------------------------
int svtkAppendPoints::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}
