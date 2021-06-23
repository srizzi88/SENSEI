//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/VecAxisAlignedPointCoordinates.h>

#include <svtkm/testing/Testing.h>

namespace
{

using Vec3 = svtkm::Vec3f;

static const Vec3 g_Origin = Vec3(1.0f, 2.0f, 3.0f);
static const Vec3 g_Spacing = Vec3(4.0f, 5.0f, 6.0f);

static const Vec3 g_Coords[8] = { Vec3(1.0f, 2.0f, 3.0f), Vec3(5.0f, 2.0f, 3.0f),
                                  Vec3(5.0f, 7.0f, 3.0f), Vec3(1.0f, 7.0f, 3.0f),
                                  Vec3(1.0f, 2.0f, 9.0f), Vec3(5.0f, 2.0f, 9.0f),
                                  Vec3(5.0f, 7.0f, 9.0f), Vec3(1.0f, 7.0f, 9.0f) };

// You will get a compile fail if this does not pass
void CheckNumericTag(svtkm::TypeTraitsRealTag)
{
  std::cout << "NumericTag pass" << std::endl;
}

// You will get a compile fail if this does not pass
void CheckDimensionalityTag(svtkm::TypeTraitsVectorTag)
{
  std::cout << "VectorTag pass" << std::endl;
}

// You will get a compile fail if this does not pass
void CheckComponentType(Vec3)
{
  std::cout << "ComponentType pass" << std::endl;
}

// You will get a compile fail if this does not pass
void CheckHasMultipleComponents(svtkm::VecTraitsTagMultipleComponents)
{
  std::cout << "MultipleComponents pass" << std::endl;
}

// You will get a compile fail if this does not pass
void CheckVariableSize(svtkm::VecTraitsTagSizeStatic)
{
  std::cout << "StaticSize" << std::endl;
}

template <typename VecCoordsType>
void CheckCoordsValues(const VecCoordsType& coords)
{
  for (svtkm::IdComponent pointIndex = 0; pointIndex < VecCoordsType::NUM_COMPONENTS; pointIndex++)
  {
    SVTKM_TEST_ASSERT(test_equal(coords[pointIndex], g_Coords[pointIndex]),
                     "Incorrect point coordinate.");
  }
}

template <svtkm::IdComponent NumDimensions>
void TryVecAxisAlignedPointCoordinates(
  const svtkm::VecAxisAlignedPointCoordinates<NumDimensions>& coords)
{
  using VecCoordsType = svtkm::VecAxisAlignedPointCoordinates<NumDimensions>;
  using TTraits = svtkm::TypeTraits<VecCoordsType>;
  using VTraits = svtkm::VecTraits<VecCoordsType>;

  std::cout << "Check traits tags." << std::endl;
  CheckNumericTag(typename TTraits::NumericTag());
  CheckDimensionalityTag(typename TTraits::DimensionalityTag());
  CheckComponentType(typename VTraits::ComponentType());
  CheckHasMultipleComponents(typename VTraits::HasMultipleComponents());
  CheckVariableSize(typename VTraits::IsSizeStatic());

  std::cout << "Check size." << std::endl;
  SVTKM_TEST_ASSERT(coords.GetNumberOfComponents() == VecCoordsType::NUM_COMPONENTS,
                   "Wrong number of components.");
  SVTKM_TEST_ASSERT(VTraits::GetNumberOfComponents(coords) == VecCoordsType::NUM_COMPONENTS,
                   "Wrong number of components.");

  std::cout << "Check contents." << std::endl;
  CheckCoordsValues(coords);

  std::cout << "Check CopyInto." << std::endl;
  svtkm::Vec<svtkm::Vec3f, VecCoordsType::NUM_COMPONENTS> copy1;
  coords.CopyInto(copy1);
  CheckCoordsValues(copy1);

  svtkm::Vec<svtkm::Vec3f, VecCoordsType::NUM_COMPONENTS> copy2;
  VTraits::CopyInto(coords, copy2);
  CheckCoordsValues(copy2);

  std::cout << "Check origin and spacing." << std::endl;
  SVTKM_TEST_ASSERT(test_equal(coords.GetOrigin(), g_Origin), "Wrong origin.");
  SVTKM_TEST_ASSERT(test_equal(coords.GetSpacing(), g_Spacing), "Wrong spacing");
}

void TestVecAxisAlignedPointCoordinates()
{
  std::cout << "***** 1D Coordinates *****************" << std::endl;
  svtkm::VecAxisAlignedPointCoordinates<1> coords1d(g_Origin, g_Spacing);
  SVTKM_TEST_ASSERT(coords1d.NUM_COMPONENTS == 2, "Wrong number of components");
  SVTKM_TEST_ASSERT(svtkm::VecAxisAlignedPointCoordinates<1>::NUM_COMPONENTS == 2,
                   "Wrong number of components");
  SVTKM_TEST_ASSERT(svtkm::VecTraits<svtkm::VecAxisAlignedPointCoordinates<1>>::NUM_COMPONENTS == 2,
                   "Wrong number of components");
  TryVecAxisAlignedPointCoordinates(coords1d);

  std::cout << "***** 2D Coordinates *****************" << std::endl;
  svtkm::VecAxisAlignedPointCoordinates<2> coords2d(g_Origin, g_Spacing);
  SVTKM_TEST_ASSERT(coords2d.NUM_COMPONENTS == 4, "Wrong number of components");
  SVTKM_TEST_ASSERT(svtkm::VecAxisAlignedPointCoordinates<2>::NUM_COMPONENTS == 4,
                   "Wrong number of components");
  SVTKM_TEST_ASSERT(svtkm::VecTraits<svtkm::VecAxisAlignedPointCoordinates<2>>::NUM_COMPONENTS == 4,
                   "Wrong number of components");
  TryVecAxisAlignedPointCoordinates(coords2d);

  std::cout << "***** 3D Coordinates *****************" << std::endl;
  svtkm::VecAxisAlignedPointCoordinates<3> coords3d(g_Origin, g_Spacing);
  SVTKM_TEST_ASSERT(coords3d.NUM_COMPONENTS == 8, "Wrong number of components");
  SVTKM_TEST_ASSERT(svtkm::VecAxisAlignedPointCoordinates<3>::NUM_COMPONENTS == 8,
                   "Wrong number of components");
  SVTKM_TEST_ASSERT(svtkm::VecTraits<svtkm::VecAxisAlignedPointCoordinates<3>>::NUM_COMPONENTS == 8,
                   "Wrong number of components");
  TryVecAxisAlignedPointCoordinates(coords3d);
}

} // anonymous namespace

int UnitTestVecAxisAlignedPointCoordinates(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestVecAxisAlignedPointCoordinates, argc, argv);
}
