
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#define svtkm_filter_ExternalFaces_cxx
#include <svtkm/filter/ExternalFaces.h>

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
ExternalFaces::ExternalFaces()
  : svtkm::filter::FilterDataSet<ExternalFaces>()
  , CompactPoints(false)
  , Worklet()
{
  this->SetPassPolyData(true);
}

//-----------------------------------------------------------------------------
svtkm::cont::DataSet ExternalFaces::GenerateOutput(const svtkm::cont::DataSet& input,
                                                  svtkm::cont::CellSetExplicit<>& outCellSet)
{
  //This section of ExternalFaces is independent of any input so we can build it
  //into the svtkm_filter library

  //3. Check the fields of the dataset to see what kinds of fields are present so
  //   we can free the cell mapping array if it won't be needed.
  const svtkm::Id numFields = input.GetNumberOfFields();
  bool hasCellFields = false;
  for (svtkm::Id fieldIdx = 0; fieldIdx < numFields && !hasCellFields; ++fieldIdx)
  {
    auto f = input.GetField(fieldIdx);
    hasCellFields = f.IsFieldCell();
  }

  if (!hasCellFields)
  {
    this->Worklet.ReleaseCellMapArrays();
  }

  //4. create the output dataset
  svtkm::cont::DataSet output;
  output.SetCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  if (this->CompactPoints)
  {
    this->Compactor.SetCompactPointFields(true);
    this->Compactor.SetMergePoints(false);
    return this->Compactor.Execute(output, PolicyDefault{});
  }
  else
  {
    return output;
  }
}

//-----------------------------------------------------------------------------
SVTKM_FILTER_INSTANTIATE_EXECUTE_METHOD(ExternalFaces);
}
}
