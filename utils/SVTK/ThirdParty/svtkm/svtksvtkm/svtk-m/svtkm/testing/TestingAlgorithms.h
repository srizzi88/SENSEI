//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_testing_TestingAlgorithms_h
#define svtk_m_testing_TestingAlgorithms_h

#include <svtkm/Algorithms.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>

#include <svtkm/exec/FunctorBase.h>

#include <svtkm/testing/Testing.h>

#include <vector>

namespace
{

using IdArray = svtkm::cont::ArrayHandle<svtkm::Id>;

struct TestBinarySearch
{
  template <typename NeedlesT, typename HayStackT, typename ResultsT>
  struct Impl : public svtkm::exec::FunctorBase
  {
    NeedlesT Needles;
    HayStackT HayStack;
    ResultsT Results;

    SVTKM_CONT
    Impl(const NeedlesT& needles, const HayStackT& hayStack, const ResultsT& results)
      : Needles(needles)
      , HayStack(hayStack)
      , Results(results)
    {
    }

    SVTKM_EXEC
    void operator()(svtkm::Id index) const
    {
      this->Results.Set(index, svtkm::BinarySearch(this->HayStack, this->Needles.Get(index)));
    }
  };

  template <typename Device>
  static void Run()
  {
    using Algo = svtkm::cont::DeviceAdapterAlgorithm<Device>;

    std::vector<svtkm::Id> needlesData{ -4, -3, -2, -1, 0, 1, 2, 3, 4, 5 };
    std::vector<svtkm::Id> hayStackData{ -3, -2, -2, -2, 0, 0, 1, 1, 1, 4, 4 };
    std::vector<bool> expectedFound{
      false, true, true, false, true, true, false, false, true, false
    };

    IdArray needles = svtkm::cont::make_ArrayHandle(needlesData);
    IdArray hayStack = svtkm::cont::make_ArrayHandle(hayStackData);
    IdArray results;

    using Functor = Impl<typename IdArray::ExecutionTypes<Device>::PortalConst,
                         typename IdArray::ExecutionTypes<Device>::PortalConst,
                         typename IdArray::ExecutionTypes<Device>::Portal>;
    Functor functor{ needles.PrepareForInput(Device{}),
                     hayStack.PrepareForInput(Device{}),
                     results.PrepareForOutput(needles.GetNumberOfValues(), Device{}) };

    Algo::Schedule(functor, needles.GetNumberOfValues());

    // Verify:
    auto needlesPortal = needles.GetPortalConstControl();
    auto hayStackPortal = hayStack.GetPortalConstControl();
    auto resultsPortal = results.GetPortalConstControl();
    for (svtkm::Id i = 0; i < needles.GetNumberOfValues(); ++i)
    {
      if (expectedFound[static_cast<size_t>(i)])
      {
        const auto resIdx = resultsPortal.Get(i);
        const auto expVal = needlesPortal.Get(i);
        SVTKM_TEST_ASSERT(resIdx >= 0);
        SVTKM_TEST_ASSERT(hayStackPortal.Get(resIdx) == expVal);
      }
      else
      {
        SVTKM_TEST_ASSERT(resultsPortal.Get(i) == -1);
      }
    }
  }
};

struct TestLowerBound
{
  template <typename NeedlesT, typename HayStackT, typename ResultsT>
  struct Impl : public svtkm::exec::FunctorBase
  {
    NeedlesT Needles;
    HayStackT HayStack;
    ResultsT Results;

    SVTKM_CONT
    Impl(const NeedlesT& needles, const HayStackT& hayStack, const ResultsT& results)
      : Needles(needles)
      , HayStack(hayStack)
      , Results(results)
    {
    }

    SVTKM_EXEC
    void operator()(svtkm::Id index) const
    {
      this->Results.Set(index, svtkm::LowerBound(this->HayStack, this->Needles.Get(index)));
    }
  };

  template <typename Device>
  static void Run()
  {
    using Algo = svtkm::cont::DeviceAdapterAlgorithm<Device>;

    std::vector<svtkm::Id> needlesData{ -4, -3, -2, -1, 0, 1, 2, 3, 4, 5 };
    std::vector<svtkm::Id> hayStackData{ -3, -2, -2, -2, 0, 0, 1, 1, 1, 4, 4 };
    std::vector<svtkm::Id> expected{ 0, 0, 1, 4, 4, 6, 9, 9, 9, 11 };

    IdArray needles = svtkm::cont::make_ArrayHandle(needlesData);
    IdArray hayStack = svtkm::cont::make_ArrayHandle(hayStackData);
    IdArray results;

    using Functor = Impl<typename IdArray::ExecutionTypes<Device>::PortalConst,
                         typename IdArray::ExecutionTypes<Device>::PortalConst,
                         typename IdArray::ExecutionTypes<Device>::Portal>;
    Functor functor{ needles.PrepareForInput(Device{}),
                     hayStack.PrepareForInput(Device{}),
                     results.PrepareForOutput(needles.GetNumberOfValues(), Device{}) };

    Algo::Schedule(functor, needles.GetNumberOfValues());

    // Verify:
    auto resultsPortal = results.GetPortalConstControl();
    for (svtkm::Id i = 0; i < needles.GetNumberOfValues(); ++i)
    {
      SVTKM_TEST_ASSERT(resultsPortal.Get(i) == expected[static_cast<size_t>(i)]);
    }
  }
};

struct TestUpperBound
{
  template <typename NeedlesT, typename HayStackT, typename ResultsT>
  struct Impl : public svtkm::exec::FunctorBase
  {
    NeedlesT Needles;
    HayStackT HayStack;
    ResultsT Results;

    SVTKM_CONT
    Impl(const NeedlesT& needles, const HayStackT& hayStack, const ResultsT& results)
      : Needles(needles)
      , HayStack(hayStack)
      , Results(results)
    {
    }

    SVTKM_EXEC
    void operator()(svtkm::Id index) const
    {
      this->Results.Set(index, svtkm::UpperBound(this->HayStack, this->Needles.Get(index)));
    }
  };

  template <typename Device>
  static void Run()
  {
    using Algo = svtkm::cont::DeviceAdapterAlgorithm<Device>;

    std::vector<svtkm::Id> needlesData{ -4, -3, -2, -1, 0, 1, 2, 3, 4, 5 };
    std::vector<svtkm::Id> hayStackData{ -3, -2, -2, -2, 0, 0, 1, 1, 1, 4, 4 };
    std::vector<svtkm::Id> expected{ 0, 1, 4, 4, 6, 9, 9, 9, 11, 11 };

    IdArray needles = svtkm::cont::make_ArrayHandle(needlesData);
    IdArray hayStack = svtkm::cont::make_ArrayHandle(hayStackData);
    IdArray results;

    using Functor = Impl<typename IdArray::ExecutionTypes<Device>::PortalConst,
                         typename IdArray::ExecutionTypes<Device>::PortalConst,
                         typename IdArray::ExecutionTypes<Device>::Portal>;
    Functor functor{ needles.PrepareForInput(Device{}),
                     hayStack.PrepareForInput(Device{}),
                     results.PrepareForOutput(needles.GetNumberOfValues(), Device{}) };

    Algo::Schedule(functor, needles.GetNumberOfValues());

    // Verify:
    auto resultsPortal = results.GetPortalConstControl();
    for (svtkm::Id i = 0; i < needles.GetNumberOfValues(); ++i)
    {
      SVTKM_TEST_ASSERT(resultsPortal.Get(i) == expected[static_cast<size_t>(i)]);
    }
  }
};

} // anon namespace

template <typename Device>
void RunAlgorithmsTests()
{
  std::cout << "Testing binary search." << std::endl;
  TestBinarySearch::Run<Device>();
  std::cout << "Testing lower bound." << std::endl;
  TestLowerBound::Run<Device>();
  std::cout << "Testing upper bound." << std::endl;
  TestUpperBound::Run<Device>();
}

#endif //svtk_m_testing_TestingAlgorithms_h
