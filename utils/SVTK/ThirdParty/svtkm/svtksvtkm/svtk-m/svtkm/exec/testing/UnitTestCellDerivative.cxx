//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/exec/CellDerivative.h>
#include <svtkm/exec/FunctorBase.h>
#include <svtkm/exec/ParametricCoordinates.h>
#include <svtkm/exec/internal/ErrorMessageBuffer.h>

#include <svtkm/CellTraits.h>
#include <svtkm/StaticAssert.h>
#include <svtkm/VecVariable.h>

#include <svtkm/testing/Testing.h>

#include <ctime>
#include <random>

namespace
{

std::mt19937 g_RandomGenerator;

// Establish simple mapping between world and parametric coordinates.
// Actual world/parametric coordinates are in a different test.
template <typename T>
svtkm::Vec<T, 3> ParametricToWorld(const svtkm::Vec<T, 3>& pcoord)
{
  return T(2) * pcoord - svtkm::Vec<T, 3>(0.25f);
}
template <typename T>
svtkm::Vec<T, 3> WorldToParametric(const svtkm::Vec<T, 3>& wcoord)
{
  return T(0.5) * (wcoord + svtkm::Vec<T, 3>(0.25f));
}

/// Simple structure describing a linear field.  Has a convenience class
/// for getting values.
template <typename FieldType>
struct LinearField
{
  svtkm::Vec<FieldType, 3> Gradient;
  FieldType OriginValue;

  template <typename T>
  FieldType GetValue(svtkm::Vec<T, 3> coordinates) const
  {
    return static_cast<FieldType>((coordinates[0] * this->Gradient[0] +
                                   coordinates[1] * this->Gradient[1] +
                                   coordinates[2] * this->Gradient[2]) +
                                  this->OriginValue);
  }
};

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
struct TestDerivativeFunctor
{
  template <typename CellShapeTag, typename WCoordsVecType>
  void DoTestWithWCoords(CellShapeTag shape,
                         const WCoordsVecType worldCoordinates,
                         LinearField<FieldType> field,
                         svtkm::Vec<FieldType, 3> expectedGradient) const
  {
    // Stuff to fake running in the execution environment.
    char messageBuffer[256];
    messageBuffer[0] = '\0';
    svtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
    svtkm::exec::FunctorBase workletProxy;
    workletProxy.SetErrorMessageBuffer(errorMessage);

    svtkm::IdComponent numPoints = worldCoordinates.GetNumberOfComponents();

    svtkm::VecVariable<FieldType, MAX_POINTS> fieldValues;
    for (svtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      svtkm::Vec3f wcoords = worldCoordinates[pointIndex];
      FieldType value = static_cast<FieldType>(field.GetValue(wcoords));
      fieldValues.Append(value);
    }

    std::cout << "    Expected: " << expectedGradient << std::endl;

    std::uniform_real_distribution<svtkm::FloatDefault> randomDist;

    for (svtkm::IdComponent trial = 0; trial < 5; trial++)
    {
      // Generate a random pcoords that we know is in the cell.
      svtkm::Vec3f pcoords(0);
      svtkm::FloatDefault totalWeight = 0;
      for (svtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
      {
        svtkm::Vec3f pointPcoords =
          svtkm::exec::ParametricCoordinatesPoint(numPoints, pointIndex, shape, workletProxy);
        SVTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);
        svtkm::FloatDefault weight = randomDist(g_RandomGenerator);
        pcoords = pcoords + weight * pointPcoords;
        totalWeight += weight;
      }
      pcoords = (1 / totalWeight) * pcoords;

      std::cout << "    Test derivative at " << pcoords << std::endl;

      svtkm::Vec<FieldType, 3> computedGradient =
        svtkm::exec::CellDerivative(fieldValues, worldCoordinates, pcoords, shape, workletProxy);
      SVTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);

      std::cout << "     Computed: " << computedGradient << std::endl;
      // Note that some gradients (particularly those near the center of
      // polygons with 5 or more points) are not very precise. Thus the
      // tolarance of the test_equal is raised.
      SVTKM_TEST_ASSERT(test_equal(computedGradient, expectedGradient, 0.01),
                       "Gradient is not as expected.");
    }
  }

  template <typename CellShapeTag>
  void DoTest(CellShapeTag shape,
              svtkm::IdComponent numPoints,
              LinearField<FieldType> field,
              svtkm::Vec<FieldType, 3> expectedGradient) const
  {
    // Stuff to fake running in the execution environment.
    char messageBuffer[256];
    messageBuffer[0] = '\0';
    svtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
    svtkm::exec::FunctorBase workletProxy;
    workletProxy.SetErrorMessageBuffer(errorMessage);

    svtkm::VecVariable<svtkm::Vec3f, MAX_POINTS> worldCoordinates;
    for (svtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      svtkm::Vec3f pcoords =
        svtkm::exec::ParametricCoordinatesPoint(numPoints, pointIndex, shape, workletProxy);
      SVTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);
      svtkm::Vec3f wcoords = ParametricToWorld(pcoords);
      SVTKM_TEST_ASSERT(test_equal(pcoords, WorldToParametric(wcoords)),
                       "Test world/parametric conversion broken.");
      worldCoordinates.Append(wcoords);
    }

    this->DoTestWithWCoords(shape, worldCoordinates, field, expectedGradient);
  }

  template <typename CellShapeTag>
  void DoTest(CellShapeTag shape, svtkm::IdComponent numPoints, svtkm::IdComponent topDim) const
  {
    LinearField<FieldType> field;
    svtkm::Vec<FieldType, 3> expectedGradient;

    using FieldTraits = svtkm::VecTraits<FieldType>;
    using FieldComponentType = typename FieldTraits::ComponentType;

    // Correct topDim for polygons with fewer than 3 points.
    if (topDim > numPoints - 1)
    {
      topDim = numPoints - 1;
    }

    std::cout << "Simple field, " << numPoints << " points" << std::endl;
    for (svtkm::IdComponent fieldComponent = 0;
         fieldComponent < FieldTraits::GetNumberOfComponents(FieldType());
         fieldComponent++)
    {
      FieldTraits::SetComponent(field.OriginValue, fieldComponent, 0.0);
    }
    field.Gradient = svtkm::make_Vec(FieldType(1.0f), FieldType(1.0f), FieldType(1.0f));
    expectedGradient[0] = ((topDim > 0) ? field.Gradient[0] : FieldType(0));
    expectedGradient[1] = ((topDim > 1) ? field.Gradient[1] : FieldType(0));
    expectedGradient[2] = ((topDim > 2) ? field.Gradient[2] : FieldType(0));
    this->DoTest(shape, numPoints, field, expectedGradient);

    std::cout << "Uneven gradient, " << numPoints << " points" << std::endl;
    for (svtkm::IdComponent fieldComponent = 0;
         fieldComponent < FieldTraits::GetNumberOfComponents(FieldType());
         fieldComponent++)
    {
      FieldTraits::SetComponent(field.OriginValue, fieldComponent, FieldComponentType(-7.0f));
    }
    field.Gradient = svtkm::make_Vec(FieldType(0.25f), FieldType(14.0f), FieldType(11.125f));
    expectedGradient[0] = ((topDim > 0) ? field.Gradient[0] : FieldType(0));
    expectedGradient[1] = ((topDim > 1) ? field.Gradient[1] : FieldType(0));
    expectedGradient[2] = ((topDim > 2) ? field.Gradient[2] : FieldType(0));
    this->DoTest(shape, numPoints, field, expectedGradient);

    std::cout << "Negative gradient directions, " << numPoints << " points" << std::endl;
    for (svtkm::IdComponent fieldComponent = 0;
         fieldComponent < FieldTraits::GetNumberOfComponents(FieldType());
         fieldComponent++)
    {
      FieldTraits::SetComponent(field.OriginValue, fieldComponent, FieldComponentType(5.0f));
    }
    field.Gradient = svtkm::make_Vec(FieldType(-11.125f), FieldType(-0.25f), FieldType(14.0f));
    expectedGradient[0] = ((topDim > 0) ? field.Gradient[0] : FieldType(0));
    expectedGradient[1] = ((topDim > 1) ? field.Gradient[1] : FieldType(0));
    expectedGradient[2] = ((topDim > 2) ? field.Gradient[2] : FieldType(0));
    this->DoTest(shape, numPoints, field, expectedGradient);

    std::cout << "Random linear field, " << numPoints << " points" << std::endl;
    std::uniform_real_distribution<FieldComponentType> randomDist(-20.0f, 20.0f);
    for (svtkm::IdComponent fieldComponent = 0;
         fieldComponent < FieldTraits::GetNumberOfComponents(FieldType());
         fieldComponent++)
    {
      FieldTraits::SetComponent(field.OriginValue, fieldComponent, randomDist(g_RandomGenerator));
      FieldTraits::SetComponent(field.Gradient[0], fieldComponent, randomDist(g_RandomGenerator));
      FieldTraits::SetComponent(field.Gradient[1], fieldComponent, randomDist(g_RandomGenerator));
      FieldTraits::SetComponent(field.Gradient[2], fieldComponent, randomDist(g_RandomGenerator));
    }
    expectedGradient[0] = ((topDim > 0) ? field.Gradient[0] : FieldType(0));
    expectedGradient[1] = ((topDim > 1) ? field.Gradient[1] : FieldType(0));
    expectedGradient[2] = ((topDim > 2) ? field.Gradient[2] : FieldType(0));
    this->DoTest(shape, numPoints, field, expectedGradient);
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
      this->DoTest(
        CellShapeTag(), numPoints, svtkm::CellTraits<CellShapeTag>::TOPOLOGICAL_DIMENSIONS);
    }

    std::cout << "--- Test generic shape tag" << std::endl;
    svtkm::CellShapeTagGeneric genericShape(CellShapeTag::Id);
    for (svtkm::IdComponent numPoints = minPoints; numPoints <= maxPoints; numPoints++)
    {
      this->DoTest(genericShape, numPoints, svtkm::CellTraits<CellShapeTag>::TOPOLOGICAL_DIMENSIONS);
    }
  }

  void operator()(svtkm::CellShapeTagEmpty) const
  {
    std::cout << "Skipping empty cell shape. No derivative." << std::endl;
  }
};

void TestDerivative()
{
  svtkm::UInt32 seed = static_cast<svtkm::UInt32>(std::time(nullptr));
  std::cout << "Seed: " << seed << std::endl;
  g_RandomGenerator.seed(seed);

  std::cout << "======== Float32 ==========================" << std::endl;
  svtkm::testing::Testing::TryAllCellShapes(TestDerivativeFunctor<svtkm::Float32>());
  std::cout << "======== Float64 ==========================" << std::endl;
  svtkm::testing::Testing::TryAllCellShapes(TestDerivativeFunctor<svtkm::Float64>());
  std::cout << "======== Vec<Float32,3> ===================" << std::endl;
  svtkm::testing::Testing::TryAllCellShapes(TestDerivativeFunctor<svtkm::Vec3f_32>());
  std::cout << "======== Vec<Float64,3> ===================" << std::endl;
  svtkm::testing::Testing::TryAllCellShapes(TestDerivativeFunctor<svtkm::Vec3f_64>());

  std::uniform_real_distribution<svtkm::Float64> randomDist(-20.0, 20.0);
  svtkm::Vec3f origin = svtkm::Vec3f(0.25f, 0.25f, 0.25f);
  svtkm::Vec3f spacing = svtkm::Vec3f(2.0f, 2.0f, 2.0f);

  LinearField<svtkm::Float64> scalarField;
  scalarField.OriginValue = randomDist(g_RandomGenerator);
  scalarField.Gradient = svtkm::make_Vec(
    randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator));
  svtkm::Vec3f_64 expectedScalarGradient = scalarField.Gradient;

  TestDerivativeFunctor<svtkm::Float64> testFunctorScalar;
  std::cout << "======== Uniform Point Coordinates 3D =====" << std::endl;
  testFunctorScalar.DoTestWithWCoords(svtkm::CellShapeTagHexahedron(),
                                      svtkm::VecAxisAlignedPointCoordinates<3>(origin, spacing),
                                      scalarField,
                                      expectedScalarGradient);
  std::cout << "======== Uniform Point Coordinates 2D =====" << std::endl;
  expectedScalarGradient[2] = 0.0;
  testFunctorScalar.DoTestWithWCoords(svtkm::CellShapeTagQuad(),
                                      svtkm::VecAxisAlignedPointCoordinates<2>(origin, spacing),
                                      scalarField,
                                      expectedScalarGradient);
  std::cout << "======== Uniform Point Coordinates 1D =====" << std::endl;
  expectedScalarGradient[1] = 0.0;
  testFunctorScalar.DoTestWithWCoords(svtkm::CellShapeTagLine(),
                                      svtkm::VecAxisAlignedPointCoordinates<1>(origin, spacing),
                                      scalarField,
                                      expectedScalarGradient);

  LinearField<svtkm::Vec3f_64> vectorField;
  vectorField.OriginValue = svtkm::make_Vec(
    randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator));
  vectorField.Gradient = svtkm::make_Vec(
    svtkm::make_Vec(
      randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator)),
    svtkm::make_Vec(
      randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator)),
    svtkm::make_Vec(
      randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator)));
  svtkm::Vec<svtkm::Vec3f_64, 3> expectedVectorGradient = vectorField.Gradient;

  TestDerivativeFunctor<svtkm::Vec3f_64> testFunctorVector;
  std::cout << "======== Uniform Point Coordinates 3D =====" << std::endl;
  testFunctorVector.DoTestWithWCoords(svtkm::CellShapeTagHexahedron(),
                                      svtkm::VecAxisAlignedPointCoordinates<3>(origin, spacing),
                                      vectorField,
                                      expectedVectorGradient);
  std::cout << "======== Uniform Point Coordinates 2D =====" << std::endl;
  expectedVectorGradient[2] = svtkm::Vec3f_64(0.0);
  testFunctorVector.DoTestWithWCoords(svtkm::CellShapeTagQuad(),
                                      svtkm::VecAxisAlignedPointCoordinates<2>(origin, spacing),
                                      vectorField,
                                      expectedVectorGradient);
  std::cout << "======== Uniform Point Coordinates 1D =====" << std::endl;
  expectedVectorGradient[1] = svtkm::Vec3f_64(0.0);
  testFunctorVector.DoTestWithWCoords(svtkm::CellShapeTagLine(),
                                      svtkm::VecAxisAlignedPointCoordinates<1>(origin, spacing),
                                      vectorField,
                                      expectedVectorGradient);
}

} // anonymous namespace

int UnitTestCellDerivative(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestDerivative, argc, argv);
}
