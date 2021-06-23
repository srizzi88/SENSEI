//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/DispatcherPointNeighborhood.h>
#include <svtkm/worklet/WorkletPointNeighborhood.h>

#include <svtkm/worklet/ScatterIdentity.h>
#include <svtkm/worklet/ScatterUniform.h>

#include <svtkm/Math.h>
#include <svtkm/VecAxisAlignedPointCoordinates.h>

#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DeviceAdapterTag.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

namespace test_pointneighborhood
{

struct MaxNeighborValue : public svtkm::worklet::WorkletPointNeighborhood
{

  using ControlSignature = void(FieldInNeighborhood neighbors, CellSetIn, FieldOut maxV);

  using ExecutionSignature = void(Boundary, _1, _3);
  //verify input domain can be something other than first parameter
  using InputDomain = _2;

  template <typename FieldIn, typename FieldOut>
  SVTKM_EXEC void operator()(const svtkm::exec::BoundaryState& boundary,
                            const svtkm::exec::FieldNeighborhood<FieldIn>& inputField,
                            FieldOut& output) const
  {
    using ValueType = typename FieldIn::ValueType;

    auto* nboundary = inputField.Boundary;

    if (!(nboundary->IsRadiusInXBoundary(1) == boundary.IsRadiusInXBoundary(1)))
    {
      this->RaiseError("Got invalid XPos boundary state");
    }

    if (!(nboundary->IsRadiusInYBoundary(1) == boundary.IsRadiusInYBoundary(1)))
    {
      this->RaiseError("Got invalid YPos boundary state");
    }

    if (!(nboundary->IsRadiusInZBoundary(1) == boundary.IsRadiusInZBoundary(1)))
    {
      this->RaiseError("Got invalid ZPos boundary state");
    }

    if (!(nboundary->IsRadiusInBoundary(1) == boundary.IsRadiusInBoundary(1)))
    {
      this->RaiseError("Got invalid boundary state");
    }

    if (nboundary->IsRadiusInXBoundary(1) !=
        (boundary.IsNeighborInXBoundary(-1) && boundary.IsNeighborInXBoundary(1)))
    {
      this->RaiseError("Neighbor/Radius boundary mismatch in X dimension.");
    }

    if (nboundary->IsRadiusInYBoundary(1) !=
        (boundary.IsNeighborInYBoundary(-1) && boundary.IsNeighborInYBoundary(1)))
    {
      this->RaiseError("Neighbor/Radius boundary mismatch in Y dimension.");
    }

    if (nboundary->IsRadiusInZBoundary(1) !=
        (boundary.IsNeighborInZBoundary(-1) && boundary.IsNeighborInZBoundary(1)))
    {
      this->RaiseError("Neighbor/Radius boundary mismatch in Z dimension.");
    }

    if (nboundary->IsRadiusInBoundary(1) !=
        (boundary.IsNeighborInBoundary({ -1 }) && boundary.IsNeighborInBoundary({ 1 })))
    {
      this->RaiseError("Neighbor/Radius boundary mismatch.");
    }


    auto minNeighbors = boundary.MinNeighborIndices(1);
    auto maxNeighbors = boundary.MaxNeighborIndices(1);

    ValueType maxV = inputField.Get(0, 0, 0); //our value
    for (svtkm::IdComponent k = minNeighbors[2]; k <= maxNeighbors[2]; ++k)
    {
      for (svtkm::IdComponent j = minNeighbors[1]; j <= maxNeighbors[1]; ++j)
      {
        for (svtkm::IdComponent i = minNeighbors[0]; i <= maxNeighbors[0]; ++i)
        {
          maxV = svtkm::Max(maxV, inputField.Get(i, j, k));
        }
      }
    }
    output = static_cast<FieldOut>(maxV);
  }
};

struct ScatterIdentityNeighbor : public svtkm::worklet::WorkletPointNeighborhood
{
  using ControlSignature = void(CellSetIn topology, FieldIn pointCoords);
  using ExecutionSignature =
    void(_2, WorkIndex, InputIndex, OutputIndex, ThreadIndices, VisitIndex);

  SVTKM_CONT
  ScatterIdentityNeighbor() {}

  template <typename T>
  SVTKM_EXEC void operator()(
    const svtkm::Vec<T, 3>& svtkmNotUsed(coords),
    const svtkm::Id& workIndex,
    const svtkm::Id& inputIndex,
    const svtkm::Id& outputIndex,
    const svtkm::exec::arg::ThreadIndicesPointNeighborhood& svtkmNotUsed(threadIndices),
    const svtkm::Id& visitIndex) const
  {
    if (workIndex != inputIndex)
    {
      this->RaiseError("Got wrong input value.");
    }
    if (outputIndex != workIndex)
    {
      this->RaiseError("Got work and output index don't match.");
    }
    if (visitIndex != 0)
    {
      this->RaiseError("Got wrong visit value1.");
    }
  }


  using ScatterType = svtkm::worklet::ScatterIdentity;
};

struct ScatterUniformNeighbor : public svtkm::worklet::WorkletPointNeighborhood
{
  using ControlSignature = void(CellSetIn topology, FieldIn pointCoords);
  using ExecutionSignature =
    void(_2, WorkIndex, InputIndex, OutputIndex, ThreadIndices, VisitIndex);

  SVTKM_CONT
  ScatterUniformNeighbor() {}

  template <typename T>
  SVTKM_EXEC void operator()(
    const svtkm::Vec<T, 3>& svtkmNotUsed(coords),
    const svtkm::Id& workIndex,
    const svtkm::Id& inputIndex,
    const svtkm::Id& outputIndex,
    const svtkm::exec::arg::ThreadIndicesPointNeighborhood& svtkmNotUsed(threadIndices),
    const svtkm::Id& visitIndex) const
  {
    if ((workIndex / 3) != inputIndex)
    {
      this->RaiseError("Got wrong input value.");
    }
    if (outputIndex != workIndex)
    {
      this->RaiseError("Got work and output index don't match.");
    }
    if ((workIndex % 3) != visitIndex)
    {
      this->RaiseError("Got wrong visit value2.");
    }
  }


  using ScatterType = svtkm::worklet::ScatterUniform<3>;
};
}

namespace
{

static void TestMaxNeighborValue();
static void TestScatterIdentityNeighbor();
static void TestScatterUnfiormNeighbor();

void TestWorkletPointNeighborhood(svtkm::cont::DeviceAdapterId id)
{
  std::cout << "Testing Point Neighborhood Worklet on device adapter: " << id.GetName()
            << std::endl;

  TestMaxNeighborValue();
  TestScatterIdentityNeighbor();
  TestScatterUnfiormNeighbor();
}

static void TestMaxNeighborValue()
{
  std::cout << "Testing MaxPointOfCell worklet" << std::endl;


  svtkm::cont::testing::MakeTestDataSet testDataSet;

  svtkm::worklet::DispatcherPointNeighborhood<::test_pointneighborhood::MaxNeighborValue> dispatcher;

  svtkm::cont::ArrayHandle<svtkm::Float32> output;

  svtkm::cont::DataSet dataSet3D = testDataSet.Make3DUniformDataSet0();
  dispatcher.Invoke(
    dataSet3D.GetField("pointvar").GetData().ResetTypes(svtkm::TypeListFieldScalar()),
    dataSet3D.GetCellSet(),
    output);

  svtkm::Float32 expected3D[18] = { 110.3f, 120.3f, 120.3f, 110.3f, 120.3f, 120.3f,
                                   170.5f, 180.5f, 180.5f, 170.5f, 180.5f, 180.5f,
                                   170.5f, 180.5f, 180.5f, 170.5f, 180.5f, 180.5f };
  for (int i = 0; i < 18; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(output.GetPortalConstControl().Get(i), expected3D[i]),
                     "Wrong result for MaxNeighborValue worklet");
  }

  svtkm::cont::DataSet dataSet2D = testDataSet.Make2DUniformDataSet1();
  dispatcher.Invoke(
    dataSet2D.GetField("pointvar").GetData().ResetTypes(svtkm::TypeListFieldScalar()),
    dataSet2D.GetCellSet(),
    output);

  svtkm::Float32 expected2D[25] = { 100.0f, 100.0f, 78.0f, 49.0f, 33.0f, 100.0f, 100.0f,
                                   78.0f,  50.0f,  48.0f, 94.0f, 94.0f, 91.0f,  91.0f,
                                   91.0f,  52.0f,  52.0f, 91.0f, 91.0f, 91.0f,  12.0f,
                                   51.0f,  91.0f,  91.0f, 91.0f };

  for (int i = 0; i < 25; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(output.GetPortalConstControl().Get(i), expected2D[i]),
                     "Wrong result for MaxNeighborValue worklet");
  }
}

static void TestScatterIdentityNeighbor()
{
  std::cout << "Testing identity scatter with PointNeighborhood" << std::endl;

  svtkm::cont::testing::MakeTestDataSet testDataSet;

  svtkm::worklet::DispatcherPointNeighborhood<::test_pointneighborhood::ScatterIdentityNeighbor>
    dispatcher;

  svtkm::cont::DataSet dataSet3D = testDataSet.Make3DUniformDataSet0();
  dispatcher.Invoke(dataSet3D.GetCellSet(), dataSet3D.GetCoordinateSystem());

  svtkm::cont::DataSet dataSet2D = testDataSet.Make2DUniformDataSet0();
  dispatcher.Invoke(dataSet2D.GetCellSet(), dataSet2D.GetCoordinateSystem());
}


static void TestScatterUnfiormNeighbor()
{
  std::cout << "Testing uniform scatter with PointNeighborhood" << std::endl;

  svtkm::cont::testing::MakeTestDataSet testDataSet;

  svtkm::worklet::DispatcherPointNeighborhood<::test_pointneighborhood::ScatterUniformNeighbor>
    dispatcher;

  svtkm::cont::DataSet dataSet3D = testDataSet.Make3DUniformDataSet0();
  dispatcher.Invoke(dataSet3D.GetCellSet(), dataSet3D.GetCoordinateSystem());

  svtkm::cont::DataSet dataSet2D = testDataSet.Make2DUniformDataSet0();
  dispatcher.Invoke(dataSet2D.GetCellSet(), dataSet2D.GetCoordinateSystem());
}

} // anonymous namespace

int UnitTestWorkletMapPointNeighborhood(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::RunOnDevice(TestWorkletPointNeighborhood, argc, argv);
}
