//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/exec/FunctorBase.h>
#include <svtkm/exec/ParametricCoordinates.h>

#include <svtkm/CellTraits.h>
#include <svtkm/StaticAssert.h>
#include <svtkm/VecVariable.h>

#include <svtkm/testing/Testing.h>

#include <ctime>
#include <random>

namespace
{

std::mt19937 g_RandomGenerator;

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

template <typename PointWCoordsType, typename T, typename CellShapeTag>
static void CompareCoordinates(const PointWCoordsType& pointWCoords,
                               svtkm::Vec<T, 3> truePCoords,
                               svtkm::Vec<T, 3> trueWCoords,
                               CellShapeTag shape)
{
  using Vector3 = svtkm::Vec<T, 3>;

  // Stuff to fake running in the execution environment.
  char messageBuffer[256];
  messageBuffer[0] = '\0';
  svtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
  svtkm::exec::FunctorBase workletProxy;
  workletProxy.SetErrorMessageBuffer(errorMessage);

  Vector3 computedWCoords = svtkm::exec::ParametricCoordinatesToWorldCoordinates(
    pointWCoords, truePCoords, shape, workletProxy);
  SVTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);
  SVTKM_TEST_ASSERT(test_equal(computedWCoords, trueWCoords, 0.01),
                   "Computed wrong world coords from parametric coords.");

  bool success = false;
  Vector3 computedPCoords = svtkm::exec::WorldCoordinatesToParametricCoordinates(
    pointWCoords, trueWCoords, shape, success, workletProxy);
  SVTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);
  SVTKM_TEST_ASSERT(test_equal(computedPCoords, truePCoords, 0.01),
                   "Computed wrong parametric coords from world coords.");
}

template <typename PointWCoordsType, typename CellShapeTag>
void TestPCoordsSpecial(const PointWCoordsType& pointWCoords, CellShapeTag shape)
{
  using Vector3 = typename PointWCoordsType::ComponentType;
  using T = typename Vector3::ComponentType;

  // Stuff to fake running in the execution environment.
  char messageBuffer[256];
  messageBuffer[0] = '\0';
  svtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
  svtkm::exec::FunctorBase workletProxy;
  workletProxy.SetErrorMessageBuffer(errorMessage);

  const svtkm::IdComponent numPoints = pointWCoords.GetNumberOfComponents();

  std::cout << "    Test parametric coordinates at cell nodes." << std::endl;
  for (svtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
  {
    Vector3 pcoords;
    svtkm::exec::ParametricCoordinatesPoint(numPoints, pointIndex, pcoords, shape, workletProxy);
    SVTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);
    Vector3 wcoords = pointWCoords[pointIndex];
    CompareCoordinates(pointWCoords, pcoords, wcoords, shape);
  }

  {
    std::cout << "    Test parametric coordinates at cell center." << std::endl;
    Vector3 wcoords = pointWCoords[0];
    for (svtkm::IdComponent pointIndex = 1; pointIndex < numPoints; pointIndex++)
    {
      wcoords = wcoords + pointWCoords[pointIndex];
    }
    wcoords = wcoords / Vector3(T(numPoints));

    Vector3 pcoords;
    svtkm::exec::ParametricCoordinatesCenter(numPoints, pcoords, shape, workletProxy);
    CompareCoordinates(pointWCoords, pcoords, wcoords, shape);
  }
}

template <typename PointWCoordsType, typename CellShapeTag>
void TestPCoordsSample(const PointWCoordsType& pointWCoords, CellShapeTag shape)
{
  using Vector3 = typename PointWCoordsType::ComponentType;

  // Stuff to fake running in the execution environment.
  char messageBuffer[256];
  messageBuffer[0] = '\0';
  svtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
  svtkm::exec::FunctorBase workletProxy;
  workletProxy.SetErrorMessageBuffer(errorMessage);

  const svtkm::IdComponent numPoints = pointWCoords.GetNumberOfComponents();

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

    std::cout << "    Test parametric coordinates at " << pcoords << std::endl;

    // If you convert to world coordinates and back, you should get the
    // same value.
    Vector3 wcoords = svtkm::exec::ParametricCoordinatesToWorldCoordinates(
      pointWCoords, pcoords, shape, workletProxy);
    SVTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);
    bool success = false;
    Vector3 computedPCoords = svtkm::exec::WorldCoordinatesToParametricCoordinates(
      pointWCoords, wcoords, shape, success, workletProxy);
    SVTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);

    SVTKM_TEST_ASSERT(test_equal(pcoords, computedPCoords, 0.05),
                     "pcoord/wcoord transform not symmetrical");
  }
}

template <typename PointWCoordsType, typename CellShellTag>
static void TestPCoords(const PointWCoordsType& pointWCoords, CellShellTag shape)
{
  TestPCoordsSpecial(pointWCoords, shape);
  TestPCoordsSample(pointWCoords, shape);
}

template <typename T>
struct TestPCoordsFunctor
{
  using Vector3 = svtkm::Vec<T, 3>;
  using PointWCoordType = svtkm::VecVariable<Vector3, MAX_POINTS>;

  template <typename CellShapeTag>
  PointWCoordType MakePointWCoords(CellShapeTag, svtkm::IdComponent numPoints) const
  {
    // Stuff to fake running in the execution environment.
    char messageBuffer[256];
    messageBuffer[0] = '\0';
    svtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
    svtkm::exec::FunctorBase workletProxy;
    workletProxy.SetErrorMessageBuffer(errorMessage);

    std::uniform_real_distribution<T> randomDist(-1, 1);

    Vector3 sheerVec(randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), 0);

    PointWCoordType pointWCoords;
    for (svtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      Vector3 pcoords;
      svtkm::exec::ParametricCoordinatesPoint(
        numPoints, pointIndex, pcoords, CellShapeTag(), workletProxy);
      SVTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);

      Vector3 wCoords = Vector3(pcoords[0], pcoords[1], pcoords[2] + svtkm::Dot(pcoords, sheerVec));
      pointWCoords.Append(wCoords);
    }

    return pointWCoords;
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
      TestPCoords(this->MakePointWCoords(CellShapeTag(), numPoints), CellShapeTag());
    }

    std::cout << "--- Test generic shape tag" << std::endl;
    svtkm::CellShapeTagGeneric genericShape(CellShapeTag::Id);
    for (svtkm::IdComponent numPoints = minPoints; numPoints <= maxPoints; numPoints++)
    {
      TestPCoords(this->MakePointWCoords(CellShapeTag(), numPoints), genericShape);
    }
  }

  void operator()(svtkm::CellShapeTagEmpty) const
  {
    std::cout << "Skipping empty cell shape. No points." << std::endl;
  }
};

void TestAllPCoords()
{
  svtkm::UInt32 seed = static_cast<svtkm::UInt32>(std::time(nullptr));
  std::cout << "Seed: " << seed << std::endl;
  g_RandomGenerator.seed(seed);

  std::cout << "======== Float32 ==========================" << std::endl;
  svtkm::testing::Testing::TryAllCellShapes(TestPCoordsFunctor<svtkm::Float32>());
  std::cout << "======== Float64 ==========================" << std::endl;
  svtkm::testing::Testing::TryAllCellShapes(TestPCoordsFunctor<svtkm::Float64>());

  std::cout << "======== Rectilinear Shapes ===============" << std::endl;
  std::uniform_real_distribution<svtkm::FloatDefault> randomDist(0.01f, 1.0f);
  svtkm::Vec3f origin(
    randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator));
  svtkm::Vec3f spacing(
    randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator));

  TestPCoords(svtkm::VecAxisAlignedPointCoordinates<3>(origin, spacing),
              svtkm::CellShapeTagHexahedron());
  TestPCoords(svtkm::VecAxisAlignedPointCoordinates<2>(origin, spacing), svtkm::CellShapeTagQuad());
  TestPCoords(svtkm::VecAxisAlignedPointCoordinates<1>(origin, spacing), svtkm::CellShapeTagLine());
}

} // Anonymous namespace

int UnitTestParametricCoordinates(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestAllPCoords, argc, argv);
}
