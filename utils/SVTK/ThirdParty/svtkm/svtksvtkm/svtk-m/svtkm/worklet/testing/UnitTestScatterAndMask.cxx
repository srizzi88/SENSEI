//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/DispatcherPointNeighborhood.h>
#include <svtkm/worklet/MaskIndices.h>
#include <svtkm/worklet/ScatterUniform.h>

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleCounting.h>

#include <svtkm/Math.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

namespace
{

using FieldType = svtkm::Float32;
#define FieldNull svtkm::Nan32()
constexpr svtkm::IdComponent IdNull = -2;

struct FieldWorklet : svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(WholeCellSetIn<>, // Placeholder for interface consistency
                                FieldIn inputField,
                                FieldInOut fieldCopy,
                                FieldInOut visitCopy);
  using ExecutionSignature = void(_2, VisitIndex, _3, _4);
  using InputDomain = _2;

  using ScatterType = svtkm::worklet::ScatterUniform<2>;
  using MaskType = svtkm::worklet::MaskIndices;

  SVTKM_EXEC void operator()(FieldType inField,
                            svtkm::IdComponent visitIndex,
                            FieldType& fieldCopy,
                            svtkm::IdComponent& visitCopy) const
  {
    fieldCopy = inField;
    visitCopy = visitIndex;
  }
};

struct TopologyWorklet : svtkm::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn,
                                FieldInPoint inputField,
                                FieldInOutPoint fieldCopy,
                                FieldInOutPoint visitCopy);
  using ExecutionSignature = void(_2, VisitIndex, _3, _4);
  using InputDomain = _1;

  using ScatterType = svtkm::worklet::ScatterUniform<2>;
  using MaskType = svtkm::worklet::MaskIndices;

  SVTKM_EXEC void operator()(FieldType inField,
                            svtkm::IdComponent visitIndex,
                            FieldType& fieldCopy,
                            svtkm::IdComponent& visitCopy) const
  {
    fieldCopy = inField;
    visitCopy = visitIndex;
  }
};

struct NeighborhoodWorklet : svtkm::worklet::WorkletPointNeighborhood
{
  using ControlSignature = void(CellSetIn,
                                FieldIn inputField,
                                FieldInOut fieldCopy,
                                FieldInOut visitCopy);
  using ExecutionSignature = void(_2, VisitIndex, _3, _4);
  using InputDomain = _1;

  using ScatterType = svtkm::worklet::ScatterUniform<2>;
  using MaskType = svtkm::worklet::MaskIndices;

  SVTKM_EXEC void operator()(FieldType inField,
                            svtkm::IdComponent visitIndex,
                            FieldType& fieldCopy,
                            svtkm::IdComponent& visitCopy) const
  {
    fieldCopy = inField;
    visitCopy = visitIndex;
  }
};

template <typename DispatcherType>
void TestMapWorklet()
{
  svtkm::cont::testing::MakeTestDataSet builder;
  svtkm::cont::DataSet data = builder.Make3DUniformDataSet1();

  svtkm::cont::CellSetStructured<3> cellSet =
    data.GetCellSet().Cast<svtkm::cont::CellSetStructured<3>>();
  svtkm::Id numPoints = cellSet.GetNumberOfPoints();

  svtkm::cont::ArrayHandle<FieldType> inField;
  inField.Allocate(numPoints);
  SetPortal(inField.GetPortalControl());

  svtkm::cont::ArrayHandle<FieldType> fieldCopy;
  svtkm::cont::ArrayCopy(svtkm::cont::make_ArrayHandleConstant(FieldNull, numPoints * 2), fieldCopy);

  svtkm::cont::ArrayHandle<svtkm::IdComponent> visitCopy;
  svtkm::cont::ArrayCopy(svtkm::cont::make_ArrayHandleConstant(IdNull, numPoints * 2), visitCopy);

  // The scatter is hardcoded to create 2 outputs for every input.
  // Set up the mask to select a range of values in the middle.
  svtkm::Id maskStart = numPoints / 2;
  svtkm::Id maskEnd = (numPoints * 2) / 3;
  svtkm::worklet::MaskIndices mask(
    svtkm::cont::make_ArrayHandleCounting(maskStart, svtkm::Id(1), maskEnd - maskStart));

  DispatcherType dispatcher(mask);
  dispatcher.Invoke(cellSet, inField, fieldCopy, visitCopy);

  // Check outputs
  auto fieldCopyPortal = fieldCopy.GetPortalConstControl();
  auto visitCopyPortal = visitCopy.GetPortalConstControl();
  for (svtkm::Id outputIndex = 0; outputIndex < numPoints * 2; ++outputIndex)
  {
    FieldType fieldValue = fieldCopyPortal.Get(outputIndex);
    svtkm::IdComponent visitValue = visitCopyPortal.Get(outputIndex);
    if ((outputIndex >= maskStart) && (outputIndex < maskEnd))
    {
      svtkm::Id inputIndex = outputIndex / 2;
      FieldType expectedField = TestValue(inputIndex, FieldType());
      SVTKM_TEST_ASSERT(fieldValue == expectedField,
                       outputIndex,
                       ": expected ",
                       expectedField,
                       ", got ",
                       fieldValue);

      svtkm::IdComponent expectedVisit = static_cast<svtkm::IdComponent>(outputIndex % 2);
      SVTKM_TEST_ASSERT(visitValue == expectedVisit,
                       outputIndex,
                       ": expected ",
                       expectedVisit,
                       ", got ",
                       visitValue);
    }
    else
    {
      SVTKM_TEST_ASSERT(svtkm::IsNan(fieldValue), outputIndex, ": expected NaN, got ", fieldValue);
      SVTKM_TEST_ASSERT(
        visitValue == IdNull, outputIndex, ": expected ", IdNull, ", got ", visitValue);
    }
  }
}

void Test()
{
  std::cout << "Try on WorkletMapField" << std::endl;
  TestMapWorklet<svtkm::worklet::DispatcherMapField<FieldWorklet>>();

  std::cout << "Try on WorkletMapCellToPoint" << std::endl;
  TestMapWorklet<svtkm::worklet::DispatcherMapTopology<TopologyWorklet>>();

  std::cout << "Try on WorkletPointNeighborhood" << std::endl;
  TestMapWorklet<svtkm::worklet::DispatcherPointNeighborhood<NeighborhoodWorklet>>();
}

} // anonymous namespace

int UnitTestScatterAndMask(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(Test, argc, argv);
}
