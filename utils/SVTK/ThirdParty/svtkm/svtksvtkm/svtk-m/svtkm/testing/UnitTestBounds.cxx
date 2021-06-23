//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/Bounds.h>

#include <svtkm/testing/Testing.h>

namespace
{

void TestBounds()
{
  using Vec3 = svtkm::Vec3f_64;

  std::cout << "Empty bounds." << std::endl;
  svtkm::Bounds emptyBounds;
  SVTKM_TEST_ASSERT(!emptyBounds.IsNonEmpty(), "Non empty bounds not empty.");

  svtkm::Bounds emptyBounds2;
  SVTKM_TEST_ASSERT(!emptyBounds2.IsNonEmpty(), "2nd empty bounds not empty.");
  SVTKM_TEST_ASSERT(!emptyBounds.Union(emptyBounds2).IsNonEmpty(),
                   "Union of empty bounds not empty.");
  emptyBounds2.Include(emptyBounds);
  SVTKM_TEST_ASSERT(!emptyBounds2.IsNonEmpty(), "Include empty in empty is not empty.");

  std::cout << "Single value bounds." << std::endl;
  svtkm::Bounds singleValueBounds(1.0, 1.0, 2.0, 2.0, 3.0, 3.0);
  SVTKM_TEST_ASSERT(singleValueBounds.IsNonEmpty(), "Empty?");
  SVTKM_TEST_ASSERT(test_equal(singleValueBounds.Center(), Vec3(1, 2, 3)), "Bad center");
  SVTKM_TEST_ASSERT(singleValueBounds.Contains(Vec3(1, 2, 3)), "Contains fail");
  SVTKM_TEST_ASSERT(!singleValueBounds.Contains(Vec3(0, 0, 0)), "Contains fail");
  SVTKM_TEST_ASSERT(!singleValueBounds.Contains(Vec3(2, 2, 2)), "contains fail");
  SVTKM_TEST_ASSERT(!singleValueBounds.Contains(Vec3(5, 5, 5)), "contains fail");

  svtkm::Bounds unionBounds = emptyBounds + singleValueBounds;
  SVTKM_TEST_ASSERT(unionBounds.IsNonEmpty(), "Empty?");
  SVTKM_TEST_ASSERT(test_equal(unionBounds.Center(), Vec3(1, 2, 3)), "Bad center");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(1, 2, 3)), "Contains fail");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(0, 0, 0)), "Contains fail");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(2, 2, 2)), "contains fail");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(5, 5, 5)), "contains fail");
  SVTKM_TEST_ASSERT(singleValueBounds == unionBounds, "Union not equal");

  std::cout << "Low bounds." << std::endl;
  svtkm::Bounds lowBounds(Vec3(-10, -5, -1), Vec3(-5, -2, 0));
  SVTKM_TEST_ASSERT(lowBounds.IsNonEmpty(), "Empty?");
  SVTKM_TEST_ASSERT(test_equal(lowBounds.Center(), Vec3(-7.5, -3.5, -0.5)), "Bad center");
  SVTKM_TEST_ASSERT(!lowBounds.Contains(Vec3(-20)), "Contains fail");
  SVTKM_TEST_ASSERT(!lowBounds.Contains(Vec3(-2)), "Contains fail");
  SVTKM_TEST_ASSERT(lowBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  SVTKM_TEST_ASSERT(!lowBounds.Contains(Vec3(0)), "Contains fail");
  SVTKM_TEST_ASSERT(!lowBounds.Contains(Vec3(10)), "Contains fail");

  unionBounds = singleValueBounds + lowBounds;
  SVTKM_TEST_ASSERT(unionBounds.IsNonEmpty(), "Empty?");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-20)), "Contains fail");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-2)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(0)), "Contains fail");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(10)), "Contains fail");

  std::cout << "High bounds." << std::endl;
  svtkm::Float64 highBoundsArray[6] = { 15.0, 20.0, 2.0, 5.0, 5.0, 10.0 };
  svtkm::Bounds highBounds(highBoundsArray);
  SVTKM_TEST_ASSERT(highBounds.IsNonEmpty(), "Empty?");
  SVTKM_TEST_ASSERT(!highBounds.Contains(Vec3(-20)), "Contains fail");
  SVTKM_TEST_ASSERT(!highBounds.Contains(Vec3(-2)), "Contains fail");
  SVTKM_TEST_ASSERT(!highBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  SVTKM_TEST_ASSERT(!highBounds.Contains(Vec3(0)), "Contains fail");
  SVTKM_TEST_ASSERT(!highBounds.Contains(Vec3(4)), "Contains fail");
  SVTKM_TEST_ASSERT(highBounds.Contains(Vec3(17, 3, 7)), "Contains fail");
  SVTKM_TEST_ASSERT(!highBounds.Contains(Vec3(25)), "Contains fail");

  unionBounds = highBounds.Union(singleValueBounds);
  SVTKM_TEST_ASSERT(unionBounds.IsNonEmpty(), "Empty?");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-20)), "Contains fail");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-2)), "Contains fail");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(0)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(4)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(17, 3, 7)), "Contains fail");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(25)), "Contains fail");

  unionBounds.Include(Vec3(-1));
  SVTKM_TEST_ASSERT(unionBounds.IsNonEmpty(), "Empty?");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-20)), "Contains fail");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-2)), "Contains fail");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(0)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(4)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(17, 3, 7)), "Contains fail");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(25)), "Contains fail");

  unionBounds.Include(lowBounds);
  SVTKM_TEST_ASSERT(unionBounds.IsNonEmpty(), "Empty?");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-20)), "Contains fail");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-2)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(0)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(4)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(17, 3, 7)), "Contains fail");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(25)), "Contains fail");

  std::cout << "Try adding infinity." << std::endl;
  unionBounds.Include(Vec3(svtkm::Infinity64()));
  SVTKM_TEST_ASSERT(unionBounds.IsNonEmpty(), "Empty?");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-20)), "Contains fail");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-2)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(0)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(4)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(17, 3, 7)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(25)), "Contains fail");

  std::cout << "Try adding NaN." << std::endl;
  unionBounds.Include(Vec3(svtkm::Nan64()));
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-20)), "Contains fail");
  SVTKM_TEST_ASSERT(!unionBounds.Contains(Vec3(-2)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(-7, -2, -0.5)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(0)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(4)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(17, 3, 7)), "Contains fail");
  SVTKM_TEST_ASSERT(unionBounds.Contains(Vec3(25)), "Contains fail");
}

} // anonymous namespace

int UnitTestBounds(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestBounds, argc, argv);
}
