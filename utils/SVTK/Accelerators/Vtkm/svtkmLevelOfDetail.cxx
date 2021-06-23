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
#include "svtkmLevelOfDetail.h"
#include "svtkmConfig.h"

#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"

#include "svtkmlib/ArrayConverters.h"
#include "svtkmlib/DataSetConverters.h"
#include "svtkmlib/PolyDataConverter.h"

#include "svtkmFilterPolicy.h"

#include <svtkm/filter/VertexClustering.h>
// To handle computing custom coordinate sets bounds we need to include
// the following
#include <svtkm/cont/ArrayRangeCompute.hxx>

svtkStandardNewMacro(svtkmLevelOfDetail);

//------------------------------------------------------------------------------
svtkmLevelOfDetail::svtkmLevelOfDetail()
{
  this->NumberOfDivisions[0] = 512;
  this->NumberOfDivisions[1] = 512;
  this->NumberOfDivisions[2] = 512;
}

//------------------------------------------------------------------------------
svtkmLevelOfDetail::~svtkmLevelOfDetail() {}

//------------------------------------------------------------------------------
void svtkmLevelOfDetail::SetNumberOfXDivisions(int num)
{
  this->Modified();
  this->NumberOfDivisions[0] = num;
}

//------------------------------------------------------------------------------
void svtkmLevelOfDetail::SetNumberOfYDivisions(int num)
{
  this->Modified();
  this->NumberOfDivisions[1] = num;
}

//------------------------------------------------------------------------------
void svtkmLevelOfDetail::SetNumberOfZDivisions(int num)
{
  this->Modified();
  this->NumberOfDivisions[2] = num;
}

//------------------------------------------------------------------------------
int svtkmLevelOfDetail::GetNumberOfXDivisions()
{
  return this->NumberOfDivisions[0];
}

//------------------------------------------------------------------------------
int svtkmLevelOfDetail::GetNumberOfYDivisions()
{
  return this->NumberOfDivisions[1];
}

//------------------------------------------------------------------------------
int svtkmLevelOfDetail::GetNumberOfZDivisions()
{
  return this->NumberOfDivisions[2];
}

//------------------------------------------------------------------------------
void svtkmLevelOfDetail::SetNumberOfDivisions(int div0, int div1, int div2)
{
  this->Modified();
  this->NumberOfDivisions[0] = div0;
  this->NumberOfDivisions[1] = div1;
  this->NumberOfDivisions[2] = div2;
}

//------------------------------------------------------------------------------
const int* svtkmLevelOfDetail::GetNumberOfDivisions()
{
  return this->NumberOfDivisions;
}

//------------------------------------------------------------------------------
void svtkmLevelOfDetail::GetNumberOfDivisions(int div[3])
{
  div[0] = this->NumberOfDivisions[0];
  div[1] = this->NumberOfDivisions[1];
  div[2] = this->NumberOfDivisions[1];
}

//------------------------------------------------------------------------------
int svtkmLevelOfDetail::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (!input || input->GetNumberOfPoints() == 0)
  {
    // empty output for empty inputs
    return 1;
  }

  try
  {
    // convert the input dataset to a svtkm::cont::DataSet
    auto in = tosvtkm::Convert(input, tosvtkm::FieldsFlag::PointsAndCells);
    if (in.GetNumberOfCells() == 0 || in.GetNumberOfPoints() == 0)
    {
      return 0;
    }

    svtkmInputFilterPolicy policy;
    svtkm::filter::VertexClustering filter;
    filter.SetNumberOfDivisions(svtkm::make_Vec(
      this->NumberOfDivisions[0], this->NumberOfDivisions[1], this->NumberOfDivisions[2]));

    auto result = filter.Execute(in, policy);

    // convert back the dataset to SVTK
    if (!fromsvtkm::Convert(result, output, input))
    {
      svtkErrorMacro(<< "Unable to convert SVTKm DataSet back to SVTK");
      return 0;
    }
  }
  catch (const svtkm::cont::Error& e)
  {
    svtkErrorMacro(<< "SVTK-m error: " << e.GetMessage());
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
void svtkmLevelOfDetail::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number of X Divisions: " << this->NumberOfDivisions[0] << "\n";
  os << indent << "Number of Y Divisions: " << this->NumberOfDivisions[1] << "\n";
  os << indent << "Number of Z Divisions: " << this->NumberOfDivisions[2] << "\n";
}
