//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/exec/CellInterpolate.h>
#include <svtkm/exec/FunctorBase.h>
#include <svtkm/exec/ParametricCoordinates.h>
#include <svtkm/exec/internal/ErrorMessageBuffer.h>

#include <svtkm/CellTraits.h>
#include <svtkm/StaticAssert.h>
#include <svtkm/VecAxisAlignedPointCoordinates.h>
#include <svtkm/VecVariable.h>

#include <svtkm/testing/Testing.h>

namespace
{

static constexpr svtkm::IdComponent MAX_POINTS = 8;

template <typename CellShapeTag>
void GetMinMaxPoints(CellShapeTag,
                     svtkm::CellTraitsTagSizeFixed,
                     svtkm::IdComponent& minPoints,
                     svtkm::IdComponent& maxPoints)
{
  // If this line fails, then MAX_POINTS is not large enough to support all
  // cell shapes.
  SVTKM_STATIC_ASSERT((svtkm::CellTraits<CellShapeTag>::NUM_POINTS <= MAX_POINTS));
  minPoints = maxPoints = svtkm::CellTraits<CellShapeTag>::NUM_POINTS;
}

template <typename CellShapeTag>
void GetMinMaxPoints(CellShapeTag,
                     svtkm::CellTraitsTagSizeVariable,
                     svtkm::IdComponent& minPoints,
                     svtkm::IdComponent& maxPoints)
{
  minPoints = 1;
  maxPoints = MAX_POINTS;
}

template <typename FieldType>
struct TestInterpolateFunctor
{
  using ComponentType = typename svtkm::VecTraits<FieldType>::ComponentType;

  template <typename CellShapeTag, typename FieldVecType>
  void DoTestWithField(CellShapeTag shape, const FieldVecType& fieldValues) const
  {
    svtkm::IdComponent numPoints = fieldValues.GetNumberOfComponents();
    FieldType averageValue = svtkm::TypeTraits<FieldType>::ZeroInitialization();
    for (svtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      averageValue = averageValue + fieldValues[pointIndex];
    }
    averageValue = static_cast<ComponentType>(1.0 / numPoints) * averageValue;

    // Stuff to fake running in the execution environment.
    char messageBuffer[256];
    messageBuffer[0] = '\0';
    svtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
    svtkm::exec::FunctorBase workletProxy;
    workletProxy.SetErrorMessageBuffer(errorMessage);

    std::cout << "  Test interpolated value at each cell node." << std::endl;
    for (svtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      svtkm::Vec3f pcoord =
        svtkm::exec::ParametricCoordinatesPoint(numPoints, pointIndex, shape, workletProxy);
      SVTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);
      FieldType interpolatedValue =
        svtkm::exec::CellInterpolate(fieldValues, pcoord, shape, workletProxy);
      SVTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);

      SVTKM_TEST_ASSERT(test_equal(fieldValues[pointIndex], interpolatedValue),
                       "Interpolation at point not point value.");
    }

    if (numPoints > 0)
    {
      std::cout << "  Test interpolated value at cell center." << std::endl;
      svtkm::Vec3f pcoord = svtkm::exec::ParametricCoordinatesCenter(numPoints, shape, workletProxy);
      SVTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);
      FieldType interpolatedValue =
        svtkm::exec::CellInterpolate(fieldValues, pcoord, shape, workletProxy);
      SVTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);

      std::cout << "AVG= " << averageValue << " interp= " << interpolatedValue << std::endl;
      SVTKM_TEST_ASSERT(test_equal(averageValue, interpolatedValue),
                       "Interpolation at center not average value.");
    }
  }

  template <typename CellShapeTag>
  void DoTest(CellShapeTag shape, svtkm::IdComponent numPoints) const
  {
    svtkm::VecVariable<FieldType, MAX_POINTS> fieldValues;
    for (svtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      FieldType value = TestValue(pointIndex + 1, FieldType());
      fieldValues.Append(value);
    }

    this->DoTestWithField(shape, fieldValues);
  }

  template <typename CellShapeTag>
  void operator()(CellShapeTag) const
  {
    svtkm::IdComponent minPoints;
    svtkm::IdComponent maxPoints;
    GetMinMaxPoints(
      CellShapeTag(), typename svtkm::CellTraits<CellShapeTag>::IsSizeFixed(), minPoints, maxPoints);

    std::cout << "--- Test shape tag directly" << std::endl;
    for (svtkm::IdComponent numPoints = minPoints; numPoints <= maxPoints; numPoints++)
    {
      std::cout << numPoints << " points" << std::endl;
      this->DoTest(CellShapeTag(), numPoints);
    }

    std::cout << "--- Test generic shape tag" << std::endl;
    svtkm::CellShapeTagGeneric genericShape(CellShapeTag::Id);
    for (svtkm::IdComponent numPoints = minPoints; numPoints <= maxPoints; numPoints++)
    {
      std::cout << numPoints << " points" << std::endl;
      this->DoTest(genericShape, numPoints);
    }
  }
};

void TestInterpolate()
{
  std::cout << "======== Float32 ==========================" << std::endl;
  svtkm::testing::Testing::TryAllCellShapes(TestInterpolateFunctor<svtkm::Float32>());
  std::cout << "======== Float64 ==========================" << std::endl;
  svtkm::testing::Testing::TryAllCellShapes(TestInterpolateFunctor<svtkm::Float64>());
  std::cout << "======== Vec<Float32,3> ===================" << std::endl;
  svtkm::testing::Testing::TryAllCellShapes(TestInterpolateFunctor<svtkm::Vec3f_32>());
  std::cout << "======== Vec<Float64,3> ===================" << std::endl;
  svtkm::testing::Testing::TryAllCellShapes(TestInterpolateFunctor<svtkm::Vec3f_64>());

  TestInterpolateFunctor<svtkm::Vec3f> testFunctor;
  svtkm::Vec3f origin = TestValue(0, svtkm::Vec3f());
  svtkm::Vec3f spacing = TestValue(1, svtkm::Vec3f());
  std::cout << "======== Uniform Point Coordinates 1D =====" << std::endl;
  testFunctor.DoTestWithField(svtkm::CellShapeTagLine(),
                              svtkm::VecAxisAlignedPointCoordinates<1>(origin, spacing));
  std::cout << "======== Uniform Point Coordinates 2D =====" << std::endl;
  testFunctor.DoTestWithField(svtkm::CellShapeTagQuad(),
                              svtkm::VecAxisAlignedPointCoordinates<2>(origin, spacing));
  std::cout << "======== Uniform Point Coordinates 3D =====" << std::endl;
  testFunctor.DoTestWithField(svtkm::CellShapeTagHexahedron(),
                              svtkm::VecAxisAlignedPointCoordinates<3>(origin, spacing));
}

} // anonymous namespace

int UnitTestCellInterpolate(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestInterpolate, argc, argv);
}
