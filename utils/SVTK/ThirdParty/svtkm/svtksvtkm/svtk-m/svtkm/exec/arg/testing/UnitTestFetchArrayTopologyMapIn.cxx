//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/exec/arg/FetchTagArrayTopologyMapIn.h>

#include <svtkm/exec/arg/ThreadIndicesTopologyMap.h>

#include <svtkm/internal/FunctionInterface.h>
#include <svtkm/internal/Invocation.h>

#include <svtkm/testing/Testing.h>

namespace
{

static constexpr svtkm::Id ARRAY_SIZE = 10;

template <typename T>
struct TestPortal
{
  using ValueType = T;

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const
  {
    SVTKM_TEST_ASSERT(index >= 0, "Bad portal index.");
    SVTKM_TEST_ASSERT(index < this->GetNumberOfValues(), "Bad portal index.");
    return TestValue(index, ValueType());
  }
};

struct TestIndexPortal
{
  using ValueType = svtkm::Id;

  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const { return index; }
};

struct TestZeroPortal
{
  using ValueType = svtkm::IdComponent;

  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id) const { return 0; }
};

template <svtkm::IdComponent IndexToReplace, typename U>
struct replace
{
  U theReplacement;

  template <typename T, svtkm::IdComponent Index>
  struct ReturnType
  {
    using type = typename std::conditional<Index == IndexToReplace, U, T>::type;
  };

  template <typename T, svtkm::IdComponent Index>
  SVTKM_CONT typename ReturnType<T, Index>::type operator()(T&& x,
                                                           svtkm::internal::IndexTag<Index>) const
  {
    return x;
  }

  template <typename T>
  SVTKM_CONT U operator()(T&&, svtkm::internal::IndexTag<IndexToReplace>) const
  {
    return theReplacement;
  }
};


template <svtkm::IdComponent InputDomainIndex, svtkm::IdComponent ParamIndex, typename T>
struct FetchArrayTopologyMapInTests
{

  template <typename Invocation>
  void TryInvocation(const Invocation& invocation) const
  {
    using ConnectivityType = typename Invocation::InputDomainType;
    using ThreadIndicesType = svtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>;

    using FetchType = svtkm::exec::arg::Fetch<svtkm::exec::arg::FetchTagArrayTopologyMapIn,
                                             svtkm::exec::arg::AspectTagDefault,
                                             ThreadIndicesType,
                                             TestPortal<T>>;

    FetchType fetch;

    const svtkm::Id threadIndex = 0;
    const svtkm::Id outputIndex = invocation.ThreadToOutputMap.Get(threadIndex);
    const svtkm::Id inputIndex = invocation.OutputToInputMap.Get(outputIndex);
    const svtkm::IdComponent visitIndex = invocation.VisitArray.Get(outputIndex);
    ThreadIndicesType indices(
      threadIndex, inputIndex, visitIndex, outputIndex, invocation.GetInputDomain());

    typename FetchType::ValueType value =
      fetch.Load(indices, svtkm::internal::ParameterGet<ParamIndex>(invocation.Parameters));
    SVTKM_TEST_ASSERT(value.GetNumberOfComponents() == 8,
                     "Topology fetch got wrong number of components.");

    SVTKM_TEST_ASSERT(test_equal(value[0], TestValue(0, T())), "Got invalid value from Load.");
    SVTKM_TEST_ASSERT(test_equal(value[1], TestValue(1, T())), "Got invalid value from Load.");
    SVTKM_TEST_ASSERT(test_equal(value[2], TestValue(3, T())), "Got invalid value from Load.");
    SVTKM_TEST_ASSERT(test_equal(value[3], TestValue(2, T())), "Got invalid value from Load.");
    SVTKM_TEST_ASSERT(test_equal(value[4], TestValue(4, T())), "Got invalid value from Load.");
    SVTKM_TEST_ASSERT(test_equal(value[5], TestValue(5, T())), "Got invalid value from Load.");
    SVTKM_TEST_ASSERT(test_equal(value[6], TestValue(7, T())), "Got invalid value from Load.");
    SVTKM_TEST_ASSERT(test_equal(value[7], TestValue(6, T())), "Got invalid value from Load.");
  }

  void operator()() const
  {
    std::cout << "Trying ArrayTopologyMapIn fetch on parameter " << ParamIndex << " with type "
              << svtkm::testing::TypeName<T>::Name() << std::endl;

    svtkm::internal::ConnectivityStructuredInternals<3> connectivityInternals;
    connectivityInternals.SetPointDimensions(svtkm::Id3(2, 2, 2));
    svtkm::exec::ConnectivityStructured<svtkm::TopologyElementTagCell,
                                       svtkm::TopologyElementTagPoint,
                                       3>
      connectivity(connectivityInternals);

    using NullType = svtkm::internal::NullType;
    auto baseFunctionInterface = svtkm::internal::make_FunctionInterface<void>(
      NullType{}, NullType{}, NullType{}, NullType{}, NullType{});

    replace<InputDomainIndex, decltype(connectivity)> connReplaceFunctor{ connectivity };
    replace<ParamIndex, TestPortal<T>> portalReplaceFunctor{ TestPortal<T>{} };

    auto updatedInterface = baseFunctionInterface.StaticTransformCont(connReplaceFunctor)
                              .StaticTransformCont(portalReplaceFunctor);

    this->TryInvocation(svtkm::internal::make_Invocation<InputDomainIndex>(updatedInterface,
                                                                          baseFunctionInterface,
                                                                          baseFunctionInterface,
                                                                          TestIndexPortal(),
                                                                          TestZeroPortal(),
                                                                          TestIndexPortal()));
  }
};


struct TryType
{
  template <typename T>
  void operator()(T) const
  {
    FetchArrayTopologyMapInTests<3, 1, T>()();
    FetchArrayTopologyMapInTests<1, 2, T>()();
    FetchArrayTopologyMapInTests<2, 3, T>()();
    FetchArrayTopologyMapInTests<1, 4, T>()();
    FetchArrayTopologyMapInTests<1, 5, T>()();
  }
};

template <svtkm::IdComponent NumDimensions, svtkm::IdComponent ParamIndex, typename Invocation>
void TryStructuredPointCoordinatesInvocation(const Invocation& invocation)
{
  using ConnectivityType = typename Invocation::InputDomainType;
  using ThreadIndicesType = svtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>;

  svtkm::exec::arg::Fetch<svtkm::exec::arg::FetchTagArrayTopologyMapIn,
                         svtkm::exec::arg::AspectTagDefault,
                         ThreadIndicesType,
                         svtkm::internal::ArrayPortalUniformPointCoordinates>
    fetch;

  svtkm::Vec3f origin = TestValue(0, svtkm::Vec3f());
  svtkm::Vec3f spacing = TestValue(1, svtkm::Vec3f());

  {
    const svtkm::Id threadIndex = 0;
    const svtkm::Id outputIndex = invocation.ThreadToOutputMap.Get(threadIndex);
    const svtkm::Id inputIndex = invocation.OutputToInputMap.Get(outputIndex);
    const svtkm::IdComponent visitIndex = invocation.VisitArray.Get(outputIndex);
    svtkm::VecAxisAlignedPointCoordinates<NumDimensions> value =
      fetch.Load(ThreadIndicesType(
                   threadIndex, inputIndex, visitIndex, outputIndex, invocation.GetInputDomain()),
                 svtkm::internal::ParameterGet<ParamIndex>(invocation.Parameters));
    SVTKM_TEST_ASSERT(test_equal(value.GetOrigin(), origin), "Bad origin.");
    SVTKM_TEST_ASSERT(test_equal(value.GetSpacing(), spacing), "Bad spacing.");
  }

  origin[0] += spacing[0];
  {
    const svtkm::Id threadIndex = 1;
    const svtkm::Id outputIndex = invocation.ThreadToOutputMap.Get(threadIndex);
    const svtkm::Id inputIndex = invocation.OutputToInputMap.Get(outputIndex);
    const svtkm::IdComponent visitIndex = invocation.VisitArray.Get(outputIndex);
    svtkm::VecAxisAlignedPointCoordinates<NumDimensions> value =
      fetch.Load(ThreadIndicesType(
                   threadIndex, inputIndex, visitIndex, outputIndex, invocation.GetInputDomain()),
                 svtkm::internal::ParameterGet<ParamIndex>(invocation.Parameters));
    SVTKM_TEST_ASSERT(test_equal(value.GetOrigin(), origin), "Bad origin.");
    SVTKM_TEST_ASSERT(test_equal(value.GetSpacing(), spacing), "Bad spacing.");
  }
}

template <svtkm::IdComponent NumDimensions>
void TryStructuredPointCoordinates(
  const svtkm::exec::ConnectivityStructured<svtkm::TopologyElementTagCell,
                                           svtkm::TopologyElementTagPoint,
                                           NumDimensions>& connectivity,
  const svtkm::internal::ArrayPortalUniformPointCoordinates& coordinates)
{
  using NullType = svtkm::internal::NullType;

  auto baseFunctionInterface = svtkm::internal::make_FunctionInterface<void>(
    NullType{}, NullType{}, NullType{}, NullType{}, NullType{});

  auto firstFunctionInterface = svtkm::internal::make_FunctionInterface<void>(
    connectivity, coordinates, NullType{}, NullType{}, NullType{});

  // Try with topology in argument 1 and point coordinates in argument 2
  TryStructuredPointCoordinatesInvocation<NumDimensions, 2>(
    svtkm::internal::make_Invocation<1>(firstFunctionInterface,
                                       baseFunctionInterface,
                                       baseFunctionInterface,
                                       TestIndexPortal(),
                                       TestZeroPortal(),
                                       TestIndexPortal()));

  // Try again with topology in argument 3 and point coordinates in argument 1
  auto secondFunctionInterface = svtkm::internal::make_FunctionInterface<void>(
    coordinates, NullType{}, connectivity, NullType{}, NullType{});

  TryStructuredPointCoordinatesInvocation<NumDimensions, 1>(
    svtkm::internal::make_Invocation<3>(secondFunctionInterface,
                                       baseFunctionInterface,
                                       baseFunctionInterface,
                                       TestIndexPortal(),
                                       TestZeroPortal(),
                                       TestIndexPortal()));
}

void TryStructuredPointCoordinates()
{
  std::cout << "*** Fetching special case of uniform point coordinates. *****" << std::endl;

  svtkm::internal::ArrayPortalUniformPointCoordinates coordinates(
    svtkm::Id3(3, 2, 2), TestValue(0, svtkm::Vec3f()), TestValue(1, svtkm::Vec3f()));

  std::cout << "3D" << std::endl;
  svtkm::internal::ConnectivityStructuredInternals<3> connectivityInternals3d;
  connectivityInternals3d.SetPointDimensions(svtkm::Id3(3, 2, 2));
  svtkm::exec::ConnectivityStructured<svtkm::TopologyElementTagCell, svtkm::TopologyElementTagPoint, 3>
    connectivity3d(connectivityInternals3d);
  TryStructuredPointCoordinates(connectivity3d, coordinates);

  std::cout << "2D" << std::endl;
  svtkm::internal::ConnectivityStructuredInternals<2> connectivityInternals2d;
  connectivityInternals2d.SetPointDimensions(svtkm::Id2(3, 2));
  svtkm::exec::ConnectivityStructured<svtkm::TopologyElementTagCell, svtkm::TopologyElementTagPoint, 2>
    connectivity2d(connectivityInternals2d);
  TryStructuredPointCoordinates(connectivity2d, coordinates);

  std::cout << "1D" << std::endl;
  svtkm::internal::ConnectivityStructuredInternals<1> connectivityInternals1d;
  connectivityInternals1d.SetPointDimensions(3);
  svtkm::exec::ConnectivityStructured<svtkm::TopologyElementTagCell, svtkm::TopologyElementTagPoint, 1>
    connectivity1d(connectivityInternals1d);
  TryStructuredPointCoordinates(connectivity1d, coordinates);
}

void TestArrayTopologyMapIn()
{
  svtkm::testing::Testing::TryTypes(TryType(), svtkm::TypeListCommon());

  TryStructuredPointCoordinates();
}

} // anonymous namespace

int UnitTestFetchArrayTopologyMapIn(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestArrayTopologyMapIn, argc, argv);
}
