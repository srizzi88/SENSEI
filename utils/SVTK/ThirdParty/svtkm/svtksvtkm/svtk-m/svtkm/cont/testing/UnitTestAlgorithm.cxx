//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/Algorithm.h>

#include <svtkm/TypeTraits.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{
// The goal of this unit test is not to verify the correctness
// of the various algorithms. Since Algorithm is a header, we
// need to ensure we instantiate each algorithm in a source
// file to verify compilation.
//
static constexpr svtkm::Id ARRAY_SIZE = 10;

void FillTest()
{
  svtkm::cont::BitField bits;
  svtkm::cont::ArrayHandle<svtkm::Id> array;

  bits.Allocate(ARRAY_SIZE);
  array.Allocate(ARRAY_SIZE);

  svtkm::cont::Algorithm::Fill(bits, true);
  svtkm::cont::Algorithm::Fill(bits, true, 5);
  svtkm::cont::Algorithm::Fill(bits, svtkm::UInt8(0xab));
  svtkm::cont::Algorithm::Fill(bits, svtkm::UInt8(0xab), 5);
  svtkm::cont::Algorithm::Fill(array, svtkm::Id(5));
  svtkm::cont::Algorithm::Fill(array, svtkm::Id(5), 5);
}

void CopyTest()
{
  svtkm::cont::ArrayHandle<svtkm::Id> input;
  svtkm::cont::ArrayHandle<svtkm::Id> output;
  svtkm::cont::ArrayHandle<svtkm::Id> stencil;

  input.Allocate(ARRAY_SIZE);
  output.Allocate(ARRAY_SIZE);
  stencil.Allocate(ARRAY_SIZE);

  svtkm::cont::Algorithm::Copy(input, output);
  svtkm::cont::Algorithm::CopyIf(input, stencil, output);
  svtkm::cont::Algorithm::CopyIf(input, stencil, output, svtkm::LogicalNot());
  svtkm::cont::Algorithm::CopySubRange(input, 2, 1, output);
}

void BoundsTest()
{

  svtkm::cont::ArrayHandle<svtkm::Id> input;
  svtkm::cont::ArrayHandle<svtkm::Id> output;
  svtkm::cont::ArrayHandle<svtkm::Id> values;

  input.Allocate(ARRAY_SIZE);
  output.Allocate(ARRAY_SIZE);
  values.Allocate(ARRAY_SIZE);

  svtkm::cont::Algorithm::LowerBounds(input, values, output);
  svtkm::cont::Algorithm::LowerBounds(input, values, output, svtkm::Sum());
  svtkm::cont::Algorithm::LowerBounds(input, values);

  svtkm::cont::Algorithm::UpperBounds(input, values, output);
  svtkm::cont::Algorithm::UpperBounds(input, values, output, svtkm::Sum());
  svtkm::cont::Algorithm::UpperBounds(input, values);
}

void ReduceTest()
{

  svtkm::cont::ArrayHandle<svtkm::Id> input;
  svtkm::cont::ArrayHandle<svtkm::Id> keys;
  svtkm::cont::ArrayHandle<svtkm::Id> keysOut;
  svtkm::cont::ArrayHandle<svtkm::Id> valsOut;

  input.Allocate(ARRAY_SIZE);
  keys.Allocate(ARRAY_SIZE);
  keysOut.Allocate(ARRAY_SIZE);
  valsOut.Allocate(ARRAY_SIZE);

  svtkm::Id result;
  result = svtkm::cont::Algorithm::Reduce(input, svtkm::Id(0));
  result = svtkm::cont::Algorithm::Reduce(input, svtkm::Id(0), svtkm::Maximum());
  svtkm::cont::Algorithm::ReduceByKey(keys, input, keysOut, valsOut, svtkm::Maximum());
  (void)result;
}

void ScanTest()
{

  svtkm::cont::ArrayHandle<svtkm::Id> input;
  svtkm::cont::ArrayHandle<svtkm::Id> output;
  svtkm::cont::ArrayHandle<svtkm::Id> keys;

  input.Allocate(ARRAY_SIZE);
  output.Allocate(ARRAY_SIZE);
  keys.Allocate(ARRAY_SIZE);

  svtkm::Id out;
  out = svtkm::cont::Algorithm::ScanInclusive(input, output);
  out = svtkm::cont::Algorithm::ScanInclusive(input, output, svtkm::Maximum());
  out = svtkm::cont::Algorithm::StreamingScanExclusive(1, input, output);
  svtkm::cont::Algorithm::ScanInclusiveByKey(keys, input, output, svtkm::Maximum());
  svtkm::cont::Algorithm::ScanInclusiveByKey(keys, input, output);
  out = svtkm::cont::Algorithm::ScanExclusive(input, output, svtkm::Maximum(), svtkm::Id(0));
  svtkm::cont::Algorithm::ScanExclusiveByKey(keys, input, output, svtkm::Id(0), svtkm::Maximum());
  svtkm::cont::Algorithm::ScanExclusiveByKey(keys, input, output);
  svtkm::cont::Algorithm::ScanExtended(input, output);
  svtkm::cont::Algorithm::ScanExtended(input, output, svtkm::Maximum(), svtkm::Id(0));
  (void)out;
}

struct DummyFunctor : public svtkm::exec::FunctorBase
{
  template <typename IdType>
  SVTKM_EXEC void operator()(IdType) const
  {
  }
};

void ScheduleTest()
{
  svtkm::cont::Algorithm::Schedule(DummyFunctor(), svtkm::Id(1));
  svtkm::Id3 id3(1, 1, 1);
  svtkm::cont::Algorithm::Schedule(DummyFunctor(), id3);
}

struct CompFunctor
{
  template <typename T>
  SVTKM_EXEC_CONT bool operator()(const T& x, const T& y) const
  {
    return x < y;
  }
};

struct CompExecObject : svtkm::cont::ExecutionObjectBase
{
  template <typename Device>
  SVTKM_CONT CompFunctor PrepareForExecution(Device)
  {
    return CompFunctor();
  }
};

void SortTest()
{
  svtkm::cont::ArrayHandle<svtkm::Id> input;
  svtkm::cont::ArrayHandle<svtkm::Id> keys;

  input.Allocate(ARRAY_SIZE);
  keys.Allocate(ARRAY_SIZE);

  svtkm::cont::Algorithm::Sort(input);
  svtkm::cont::Algorithm::Sort(input, CompFunctor());
  svtkm::cont::Algorithm::Sort(input, CompExecObject());
  svtkm::cont::Algorithm::SortByKey(keys, input);
  svtkm::cont::Algorithm::SortByKey(keys, input, CompFunctor());
  svtkm::cont::Algorithm::SortByKey(keys, input, CompExecObject());
}

void SynchronizeTest()
{
  svtkm::cont::Algorithm::Synchronize();
}

void UniqueTest()
{
  svtkm::cont::ArrayHandle<svtkm::Id> input;

  input.Allocate(ARRAY_SIZE);

  svtkm::cont::Algorithm::Unique(input);
  svtkm::cont::Algorithm::Unique(input, CompFunctor());
  svtkm::cont::Algorithm::Unique(input, CompExecObject());
}

void TestAll()
{
  FillTest();
  CopyTest();
  BoundsTest();
  ReduceTest();
  ScanTest();
  ScheduleTest();
  SortTest();
  SynchronizeTest();
  UniqueTest();
}

} // anonymous namespace

int UnitTestAlgorithm(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestAll, argc, argv);
}
