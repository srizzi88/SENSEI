//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/arg/TransportTagCellSetIn.h>

#include <svtkm/cont/CellSetExplicit.h>

#include <svtkm/exec/FunctorBase.h>

#include <svtkm/cont/serial/DeviceAdapterSerial.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

namespace
{

template <typename CellSetInType>
struct TestKernel : public svtkm::exec::FunctorBase
{
  CellSetInType CellSet;

  SVTKM_EXEC
  void operator()(svtkm::Id) const
  {
    if (this->CellSet.GetNumberOfElements() != 2)
    {
      this->RaiseError("Got bad number of shapes in exec cellset object.");
    }

    if (this->CellSet.GetIndices(0).GetNumberOfComponents() != 3 ||
        this->CellSet.GetIndices(1).GetNumberOfComponents() != 4)
    {
      this->RaiseError("Got bad number of Indices in exec cellset object.");
    }

    if (this->CellSet.GetCellShape(0).Id != 5 || this->CellSet.GetCellShape(1).Id != 9)
    {
      this->RaiseError("Got bad cell shape in exec cellset object.");
    }
  }
};

template <typename Device>
void TransportWholeCellSetIn(Device)
{
  //build a fake cell set
  const int nVerts = 5;
  svtkm::cont::CellSetExplicit<> contObject;
  contObject.PrepareToAddCells(2, 7);
  contObject.AddCell(svtkm::CELL_SHAPE_TRIANGLE, 3, svtkm::make_Vec<svtkm::Id>(0, 1, 2));
  contObject.AddCell(svtkm::CELL_SHAPE_QUAD, 4, svtkm::make_Vec<svtkm::Id>(2, 1, 3, 4));
  contObject.CompleteAddingCells(nVerts);

  using IncidentTopology = svtkm::TopologyElementTagPoint;
  using VisitTopology = svtkm::TopologyElementTagCell;

  using ExecObjectType = typename svtkm::cont::CellSetExplicit<>::
    template ExecutionTypes<Device, VisitTopology, IncidentTopology>::ExecObjectType;

  svtkm::cont::arg::Transport<
    svtkm::cont::arg::TransportTagCellSetIn<VisitTopology, IncidentTopology>,
    svtkm::cont::CellSetExplicit<>,
    Device>
    transport;

  TestKernel<ExecObjectType> kernel;
  kernel.CellSet = transport(contObject, nullptr, 1, 1);

  svtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, 1);
}

void UnitTestCellSetIn()
{
  std::cout << "Trying CellSetIn transport with serial device." << std::endl;
  TransportWholeCellSetIn(svtkm::cont::DeviceAdapterTagSerial());
}

} // Anonymous namespace

int UnitTestTransportCellSetIn(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(UnitTestCellSetIn, argc, argv);
}
