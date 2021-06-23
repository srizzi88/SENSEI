//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_GhostCellClassify_hxx
#define svtk_m_filter_GhostCellClassify_hxx

#include <svtkm/CellClassification.h>
#include <svtkm/RangeId.h>
#include <svtkm/RangeId2.h>
#include <svtkm/RangeId3.h>
#include <svtkm/Types.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>

#include <svtkm/worklet/WorkletPointNeighborhood.h>

namespace
{
struct TypeUInt8 : svtkm::List<svtkm::UInt8>
{
};

class SetStructuredGhostCells1D : public svtkm::worklet::WorkletPointNeighborhood
{
public:
  SetStructuredGhostCells1D(svtkm::IdComponent numLayers = 1)
    : NumLayers(numLayers)
  {
  }

  using ControlSignature = void(CellSetIn, FieldOut);
  using ExecutionSignature = void(Boundary, _2);

  SVTKM_EXEC void operator()(const svtkm::exec::BoundaryState& boundary, svtkm::UInt8& value) const
  {
    const bool notOnBoundary = boundary.IsRadiusInXBoundary(this->NumLayers);
    value = (notOnBoundary) ? svtkm::CellClassification::NORMAL : svtkm::CellClassification::GHOST;
  }

private:
  svtkm::IdComponent NumLayers;
};

class SetStructuredGhostCells2D : public svtkm::worklet::WorkletPointNeighborhood
{
public:
  SetStructuredGhostCells2D(svtkm::IdComponent numLayers = 1)
    : NumLayers(numLayers)
  {
  }

  using ControlSignature = void(CellSetIn, FieldOut);
  using ExecutionSignature = void(Boundary, _2);

  SVTKM_EXEC void operator()(const svtkm::exec::BoundaryState& boundary, svtkm::UInt8& value) const
  {
    const bool notOnBoundary = boundary.IsRadiusInXBoundary(this->NumLayers) &&
      boundary.IsRadiusInYBoundary(this->NumLayers);
    value = (notOnBoundary) ? svtkm::CellClassification::NORMAL : svtkm::CellClassification::GHOST;
  }

private:
  svtkm::IdComponent NumLayers;
};

class SetStructuredGhostCells3D : public svtkm::worklet::WorkletPointNeighborhood
{
public:
  SetStructuredGhostCells3D(svtkm::IdComponent numLayers = 1)
    : NumLayers(numLayers)
  {
  }

  using ControlSignature = void(CellSetIn, FieldOut);
  using ExecutionSignature = void(Boundary, _2);

  SVTKM_EXEC void operator()(const svtkm::exec::BoundaryState& boundary, svtkm::UInt8& value) const
  {
    const bool notOnBoundary = boundary.IsRadiusInBoundary(this->NumLayers);
    value = (notOnBoundary) ? svtkm::CellClassification::NORMAL : svtkm::CellClassification::GHOST;
  }

private:
  svtkm::IdComponent NumLayers;
};
};

namespace svtkm
{
namespace filter
{

inline SVTKM_CONT GhostCellClassify::GhostCellClassify()
{
}

template <typename Policy>
inline SVTKM_CONT svtkm::cont::DataSet GhostCellClassify::DoExecute(const svtkm::cont::DataSet& input,
                                                                  svtkm::filter::PolicyBase<Policy>)
{
  const svtkm::cont::DynamicCellSet& cellset = input.GetCellSet();
  svtkm::cont::ArrayHandle<svtkm::UInt8> ghosts;
  const svtkm::Id numCells = cellset.GetNumberOfCells();

  //Structured cases are easy...
  if (cellset.template IsType<svtkm::cont::CellSetStructured<1>>())
  {
    if (numCells <= 2)
      throw svtkm::cont::ErrorFilterExecution("insufficient number of cells for GhostCellClassify.");

    svtkm::cont::CellSetStructured<1> cellset1d = cellset.Cast<svtkm::cont::CellSetStructured<1>>();

    //We use the dual of the cellset since we are using the PointNeighborhood worklet
    svtkm::cont::CellSetStructured<3> dual;
    const auto dim = cellset1d.GetCellDimensions();
    dual.SetPointDimensions(svtkm::Id3{ dim, 1, 1 });
    this->Invoke(SetStructuredGhostCells1D{}, dual, ghosts);
  }
  else if (cellset.template IsType<svtkm::cont::CellSetStructured<2>>())
  {
    if (numCells <= 4)
      throw svtkm::cont::ErrorFilterExecution("insufficient number of cells for GhostCellClassify.");

    svtkm::cont::CellSetStructured<2> cellset2d = cellset.Cast<svtkm::cont::CellSetStructured<2>>();

    //We use the dual of the cellset since we are using the PointNeighborhood worklet
    svtkm::cont::CellSetStructured<3> dual;
    const auto dims = cellset2d.GetCellDimensions();
    dual.SetPointDimensions(svtkm::Id3{ dims[0], dims[1], 1 });
    this->Invoke(SetStructuredGhostCells2D{}, dual, ghosts);
  }
  else if (cellset.template IsType<svtkm::cont::CellSetStructured<3>>())
  {
    if (numCells <= 8)
      throw svtkm::cont::ErrorFilterExecution("insufficient number of cells for GhostCellClassify.");

    svtkm::cont::CellSetStructured<3> cellset3d = cellset.Cast<svtkm::cont::CellSetStructured<3>>();

    //We use the dual of the cellset since we are using the PointNeighborhood worklet
    svtkm::cont::CellSetStructured<3> dual;
    dual.SetPointDimensions(cellset3d.GetCellDimensions());
    this->Invoke(SetStructuredGhostCells3D{}, dual, ghosts);
  }
  else
  {
    throw svtkm::cont::ErrorFilterExecution("Unsupported cellset type for GhostCellClassify.");
  }

  return CreateResultFieldCell(input, ghosts, "svtkmGhostCells");
}
}
}
#endif
