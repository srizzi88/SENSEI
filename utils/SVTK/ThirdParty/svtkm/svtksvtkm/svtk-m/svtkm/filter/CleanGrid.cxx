
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#define svtkm_filter_CleanGrid_cxx
#include <svtkm/filter/CleanGrid.h>
#include <svtkm/filter/CleanGrid.hxx>

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
CleanGrid::CleanGrid()
  : CompactPointFields(true)
  , MergePoints(true)
  , Tolerance(1.0e-6)
  , ToleranceIsAbsolute(false)
  , RemoveDegenerateCells(true)
  , FastMerge(true)
{
}

//-----------------------------------------------------------------------------
svtkm::cont::DataSet CleanGrid::GenerateOutput(const svtkm::cont::DataSet& inData,
                                              svtkm::cont::CellSetExplicit<>& outputCellSet)
{
  using VecId = std::size_t;
  const VecId activeCoordIndex = static_cast<VecId>(this->GetActiveCoordinateSystemIndex());
  const VecId numCoordSystems = static_cast<VecId>(inData.GetNumberOfCoordinateSystems());

  std::vector<svtkm::cont::CoordinateSystem> outputCoordinateSystems(numCoordSystems);

  // Start with a shallow copy of the coordinate systems
  for (VecId coordSystemIndex = 0; coordSystemIndex < numCoordSystems; ++coordSystemIndex)
  {
    outputCoordinateSystems[coordSystemIndex] =
      inData.GetCoordinateSystem(static_cast<svtkm::IdComponent>(coordSystemIndex));
  }

  // Optionally adjust the cell set indices to remove all unused points
  if (this->GetCompactPointFields())
  {
    this->PointCompactor.FindPointsStart();
    this->PointCompactor.FindPoints(outputCellSet);
    this->PointCompactor.FindPointsEnd();

    outputCellSet = this->PointCompactor.MapCellSet(outputCellSet);

    for (VecId coordSystemIndex = 0; coordSystemIndex < numCoordSystems; ++coordSystemIndex)
    {
      outputCoordinateSystems[coordSystemIndex] =
        svtkm::cont::CoordinateSystem(outputCoordinateSystems[coordSystemIndex].GetName(),
                                     this->PointCompactor.MapPointFieldDeep(
                                       outputCoordinateSystems[coordSystemIndex].GetData()));
    }
  }

  // Optionally find and merge coincident points
  if (this->GetMergePoints())
  {
    svtkm::cont::CoordinateSystem activeCoordSystem = outputCoordinateSystems[activeCoordIndex];
    svtkm::Bounds bounds = activeCoordSystem.GetBounds();

    svtkm::Float64 delta = this->GetTolerance();
    if (!this->GetToleranceIsAbsolute())
    {
      delta *=
        svtkm::Magnitude(svtkm::make_Vec(bounds.X.Length(), bounds.Y.Length(), bounds.Z.Length()));
    }

    auto coordArray = activeCoordSystem.GetData();
    this->PointMerger.Run(delta, this->GetFastMerge(), bounds, coordArray);
    activeCoordSystem = svtkm::cont::CoordinateSystem(activeCoordSystem.GetName(), coordArray);

    for (VecId coordSystemIndex = 0; coordSystemIndex < numCoordSystems; ++coordSystemIndex)
    {
      if (coordSystemIndex == activeCoordIndex)
      {
        outputCoordinateSystems[coordSystemIndex] = activeCoordSystem;
      }
      else
      {
        outputCoordinateSystems[coordSystemIndex] = svtkm::cont::CoordinateSystem(
          outputCoordinateSystems[coordSystemIndex].GetName(),
          this->PointMerger.MapPointField(outputCoordinateSystems[coordSystemIndex].GetData()));
      }
    }

    outputCellSet = this->PointMerger.MapCellSet(outputCellSet);
  }

  // Optionally remove degenerate cells
  if (this->GetRemoveDegenerateCells())
  {
    outputCellSet = this->CellCompactor.Run(outputCellSet);
  }

  // Construct resulting data set with new cell sets
  svtkm::cont::DataSet outData;
  outData.SetCellSet(outputCellSet);

  // Pass the coordinate systems
  for (VecId coordSystemIndex = 0; coordSystemIndex < numCoordSystems; ++coordSystemIndex)
  {
    outData.AddCoordinateSystem(outputCoordinateSystems[coordSystemIndex]);
  }

  return outData;
}

//-----------------------------------------------------------------------------
SVTKM_FILTER_INSTANTIATE_EXECUTE_METHOD(CleanGrid);
}
}
