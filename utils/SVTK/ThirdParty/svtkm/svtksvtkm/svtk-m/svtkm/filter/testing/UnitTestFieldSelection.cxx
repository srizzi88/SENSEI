//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/testing/Testing.h>
#include <svtkm/filter/FieldSelection.h>

namespace
{
void TestFieldSelection()
{
  {
    std::cout << "empty field selection,  everything should be false." << std::endl;
    svtkm::filter::FieldSelection selection;
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == false, "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar", svtkm::cont::Field::Association::POINTS) ==
                       false,
                     "field selection failed.");
  }

  {
    std::cout << "field selection with select all,  everything should be true." << std::endl;
    svtkm::filter::FieldSelection selection(svtkm::filter::FieldSelection::MODE_ALL);
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar", svtkm::cont::Field::Association::POINTS) ==
                       true,
                     "field selection failed.");
  }

  {
    std::cout << "field selection with select none,  everything should be false." << std::endl;
    svtkm::filter::FieldSelection selection(svtkm::filter::FieldSelection::MODE_NONE);
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == false, "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar", svtkm::cont::Field::Association::POINTS) ==
                       false,
                     "field selection failed.");
  }

  {
    std::cout << "field selection of one field" << std::endl;
    svtkm::filter::FieldSelection selection("foo");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("foo", svtkm::cont::Field::Association::POINTS) ==
                       true,
                     "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("foo", svtkm::cont::Field::Association::CELL_SET) ==
                       true,
                     "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar") == false, "field selection failed.");
  }

  {
    std::cout << "field selection of one field/association" << std::endl;
    svtkm::filter::FieldSelection selection("foo", svtkm::cont::Field::Association::POINTS);
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("foo", svtkm::cont::Field::Association::POINTS) ==
                       true,
                     "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("foo", svtkm::cont::Field::Association::CELL_SET) ==
                       false,
                     "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar") == false, "field selection failed.");
  }

  {
    std::cout << "field selection with specific fields selected (AddField)." << std::endl;
    svtkm::filter::FieldSelection selection;
    selection.AddField("foo");
    selection.AddField("bar", svtkm::cont::Field::Association::CELL_SET);
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("foo", svtkm::cont::Field::Association::POINTS) ==
                       true,
                     "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar", svtkm::cont::Field::Association::POINTS) ==
                       false,
                     "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar", svtkm::cont::Field::Association::CELL_SET) ==
                       true,
                     "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar") == true, "field selection failed.");
  }

  {
    std::cout << "field selection with specific fields selected (initializer list)." << std::endl;
    svtkm::filter::FieldSelection selection{ "foo", "bar" };
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("foo", svtkm::cont::Field::Association::POINTS) ==
                       true,
                     "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar", svtkm::cont::Field::Association::POINTS) ==
                       true,
                     "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar", svtkm::cont::Field::Association::CELL_SET) ==
                       true,
                     "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar") == true, "field selection failed.");
  }

  {
    std::cout << "field selection with specific fields selected (std::pair initializer list)."
              << std::endl;
    using pair_type = std::pair<std::string, svtkm::cont::Field::Association>;
    svtkm::filter::FieldSelection selection{ pair_type{ "foo", svtkm::cont::Field::Association::ANY },
                                            pair_type{ "bar",
                                                       svtkm::cont::Field::Association::CELL_SET } };
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("foo", svtkm::cont::Field::Association::POINTS) ==
                       true,
                     "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar", svtkm::cont::Field::Association::POINTS) ==
                       false,
                     "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar", svtkm::cont::Field::Association::CELL_SET) ==
                       true,
                     "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar") == true, "field selection failed.");
  }

  {
    std::cout << "field selection with specific fields selected (svtkm::Pair initializer list)."
              << std::endl;
    using pair_type = svtkm::Pair<std::string, svtkm::cont::Field::Association>;
    svtkm::filter::FieldSelection selection{ pair_type{ "foo", svtkm::cont::Field::Association::ANY },
                                            pair_type{ "bar",
                                                       svtkm::cont::Field::Association::CELL_SET } };
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("foo", svtkm::cont::Field::Association::POINTS) ==
                       true,
                     "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar", svtkm::cont::Field::Association::POINTS) ==
                       false,
                     "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar", svtkm::cont::Field::Association::CELL_SET) ==
                       true,
                     "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar") == true, "field selection failed.");
  }

  {
    std::cout << "field selection with specific fields excluded." << std::endl;
    using pair_type = std::pair<std::string, svtkm::cont::Field::Association>;
    svtkm::filter::FieldSelection selection(
      { pair_type{ "foo", svtkm::cont::Field::Association::ANY },
        pair_type{ "bar", svtkm::cont::Field::Association::CELL_SET } },
      svtkm::filter::FieldSelection::MODE_EXCLUDE);
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == false, "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("foo", svtkm::cont::Field::Association::POINTS) ==
                       false,
                     "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar", svtkm::cont::Field::Association::POINTS) ==
                       true,
                     "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar", svtkm::cont::Field::Association::CELL_SET) ==
                       false,
                     "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("bar") == false, "field selection failed.");
    SVTKM_TEST_ASSERT(selection.IsFieldSelected("baz") == true, "field selection failed.");
  }
}
}

int UnitTestFieldSelection(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestFieldSelection, argc, argv);
}
