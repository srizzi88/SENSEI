//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
#include "svtkmGradient.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

#include "svtkmlib/ArrayConverters.h"
#include "svtkmlib/DataSetConverters.h"
#include "svtkmlib/PolyDataConverter.h"

#include "svtkmFilterPolicy.h"

#include <svtkm/filter/Gradient.h>
#include <svtkm/filter/PointAverage.h>
#include <svtkm/filter/PointAverage.hxx>

svtkStandardNewMacro(svtkmGradient);

namespace
{
using GradientTypes = svtkm::List<            //
  svtkm::Float32,                             //
  svtkm::Float64,                             //
  svtkm::Vec<svtkm::Float32, 3>,               //
  svtkm::Vec<svtkm::Float64, 3>,               //
  svtkm::Vec<svtkm::Vec<svtkm::Float32, 3>, 3>, //
  svtkm::Vec<svtkm::Vec<svtkm::Float64, 3>, 3>  //
  >;

//------------------------------------------------------------------------------
class svtkmGradientFilterPolicy : public svtkm::filter::PolicyBase<svtkmGradientFilterPolicy>
{
public:
  using FieldTypeList = GradientTypes;

  using StructuredCellSetList = tosvtkm::CellListStructuredInSVTK;
  using UnstructuredCellSetList = tosvtkm::CellListUnstructuredInSVTK;
  using AllCellSetList = tosvtkm::CellListAllInSVTK;
};

inline svtkm::cont::DataSet CopyDataSetStructure(const svtkm::cont::DataSet& ds)
{
  svtkm::cont::DataSet cp;
  cp.CopyStructure(ds);
  return cp;
}

} // anonymous namespace

//------------------------------------------------------------------------------
svtkmGradient::svtkmGradient() {}

//------------------------------------------------------------------------------
svtkmGradient::~svtkmGradient() {}

//------------------------------------------------------------------------------
void svtkmGradient::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int svtkmGradient::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  output->ShallowCopy(input);

  // grab the input array to process to determine the field want to compute
  // the gradient for
  int association = this->GetInputArrayAssociation(0, inputVector);
  svtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputVector);
  if (inputArray == nullptr || inputArray->GetName() == nullptr || inputArray->GetName()[0] == '\0')
  {
    svtkErrorMacro("Invalid input array.");
    return 0;
  }

  try
  {
    // convert the input dataset to a svtkm::cont::DataSet. We explicitly drop
    // all arrays from the conversion as this algorithm doesn't change topology
    // and therefore doesn't need input fields converted through the SVTK-m filter
    auto in = tosvtkm::Convert(input, tosvtkm::FieldsFlag::None);
    svtkm::cont::Field field = tosvtkm::Convert(inputArray, association);
    in.AddField(field);

    const bool fieldIsPoint = field.GetAssociation() == svtkm::cont::Field::Association::POINTS;
    const bool fieldIsCell = field.GetAssociation() == svtkm::cont::Field::Association::CELL_SET;
    const bool fieldIsVec = (inputArray->GetNumberOfComponents() == 3);
    const bool fieldIsScalar =
      inputArray->GetDataType() == SVTK_FLOAT || inputArray->GetDataType() == SVTK_DOUBLE;
    const bool fieldValid =
      (fieldIsPoint || fieldIsCell) && fieldIsScalar && (field.GetName() != std::string());

    if (!fieldValid)
    {
      svtkWarningMacro(<< "Unsupported field type\n"
                      << "Falling back to svtkGradientFilter.");
      return this->Superclass::RequestData(request, inputVector, outputVector);
    }

    svtkmGradientFilterPolicy policy;
    auto passNoFields = svtkm::filter::FieldSelection(svtkm::filter::FieldSelection::MODE_NONE);
    svtkm::filter::Gradient filter;
    filter.SetFieldsToPass(passNoFields);
    filter.SetColumnMajorOrdering();

    if (fieldIsVec)
    { // this properties are only valid when processing a vec<3> field
      filter.SetComputeDivergence(this->ComputeDivergence != 0);
      filter.SetComputeVorticity(this->ComputeVorticity != 0);
      filter.SetComputeQCriterion(this->ComputeQCriterion != 0);
    }

    if (this->ResultArrayName)
    {
      filter.SetOutputFieldName(this->ResultArrayName);
    }

    if (this->DivergenceArrayName)
    {
      filter.SetDivergenceName(this->DivergenceArrayName);
    }

    if (this->VorticityArrayName)
    {
      filter.SetVorticityName(this->VorticityArrayName);
    }

    if (this->QCriterionArrayName)
    {
      filter.SetQCriterionName(this->QCriterionArrayName);
    }
    else
    {
      filter.SetQCriterionName("Q-criterion");
    }

    // Run the SVTK-m Gradient Filter
    // -----------------------------
    svtkm::cont::DataSet result;
    if (fieldIsPoint)
    {
      filter.SetComputePointGradient(!this->FasterApproximation);
      filter.SetActiveField(field.GetName(), svtkm::cont::Field::Association::POINTS);
      result = filter.Execute(in, policy);

      // When we have faster approximation enabled the SVTK-m gradient will output
      // a cell field not a point field. So at that point we will need to convert
      // back to a point field
      if (this->FasterApproximation)
      {
        svtkm::filter::PointAverage cellToPoint;
        cellToPoint.SetFieldsToPass(passNoFields);

        auto c2pIn = result;
        result = CopyDataSetStructure(result);

        if (this->ComputeGradient)
        {
          cellToPoint.SetActiveField(
            filter.GetOutputFieldName(), svtkm::cont::Field::Association::CELL_SET);
          auto ds = cellToPoint.Execute(c2pIn, policy);
          result.AddField(ds.GetField(0));
        }
        if (this->ComputeDivergence && fieldIsVec)
        {
          cellToPoint.SetActiveField(
            filter.GetDivergenceName(), svtkm::cont::Field::Association::CELL_SET);
          auto ds = cellToPoint.Execute(c2pIn, policy);
          result.AddField(ds.GetField(0));
        }
        if (this->ComputeVorticity && fieldIsVec)
        {
          cellToPoint.SetActiveField(
            filter.GetVorticityName(), svtkm::cont::Field::Association::CELL_SET);
          auto ds = cellToPoint.Execute(c2pIn, policy);
          result.AddField(ds.GetField(0));
        }
        if (this->ComputeQCriterion && fieldIsVec)
        {
          cellToPoint.SetActiveField(
            filter.GetQCriterionName(), svtkm::cont::Field::Association::CELL_SET);
          auto ds = cellToPoint.Execute(c2pIn, policy);
          result.AddField(ds.GetField(0));
        }
      }
    }
    else
    {
      // we need to convert the field to be a point field
      svtkm::filter::PointAverage cellToPoint;
      cellToPoint.SetFieldsToPass(passNoFields);
      cellToPoint.SetActiveField(field.GetName(), field.GetAssociation());
      cellToPoint.SetOutputFieldName(field.GetName());
      in = cellToPoint.Execute(in, policy);

      filter.SetComputePointGradient(false);
      filter.SetActiveField(field.GetName(), svtkm::cont::Field::Association::POINTS);
      result = filter.Execute(in, policy);
    }

    // Remove gradient field from result if it was not requested.
    auto requestedResult = result;
    if (!this->ComputeGradient)
    {
      requestedResult = CopyDataSetStructure(result);
      svtkm::Id numOfFields = static_cast<svtkm::Id>(result.GetNumberOfFields());
      for (svtkm::Id i = 0; i < numOfFields; ++i)
      {
        if (result.GetField(i).GetName() != filter.GetOutputFieldName())
        {
          requestedResult.AddField(result.GetField(i));
        }
      }
    }

    // convert arrays back to SVTK
    if (!fromsvtkm::ConvertArrays(result, output))
    {
      svtkErrorMacro(<< "Unable to convert SVTKm DataSet back to SVTK");
      return 0;
    }
  }
  catch (const svtkm::cont::Error& e)
  {
    svtkWarningMacro(<< "SVTK-m error: " << e.GetMessage()
                    << "Falling back to serial implementation.");
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  return 1;
}
