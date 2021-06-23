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

#include <svtkm/worklet/connectivities/GraphConnectivity.h>

class TestGraphConnectivity
{
public:
  void operator()() const
  {
    std::vector<svtkm::Id> counts{ 1, 1, 2, 2, 2 };
    std::vector<svtkm::Id> offsets{ 0, 1, 2, 4, 6 };
    std::vector<svtkm::Id> conn{ 2, 4, 0, 3, 2, 4, 1, 3 };

    svtkm::cont::ArrayHandle<svtkm::Id> counts_h = svtkm::cont::make_ArrayHandle(counts);
    svtkm::cont::ArrayHandle<svtkm::Id> offsets_h = svtkm::cont::make_ArrayHandle(offsets);
    svtkm::cont::ArrayHandle<svtkm::Id> conn_h = svtkm::cont::make_ArrayHandle(conn);
    svtkm::cont::ArrayHandle<svtkm::Id> comps;

    svtkm::worklet::connectivity::GraphConnectivity().Run(counts_h, offsets_h, conn_h, comps);

    for (int i = 0; i < comps.GetNumberOfValues(); i++)
    {
      SVTKM_TEST_ASSERT(comps.GetPortalConstControl().Get(i) == 0,
                       "Components has unexpected value.");
    }
  }
};

int UnitTestGraphConnectivity(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestGraphConnectivity(), argc, argv);
}
