//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/filter/Entropy.h>

#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/source/Tangle.h>


namespace
{
void TestEntropy()
{
  ///// make a data set /////
  svtkm::Id3 dims(32, 32, 32);
  svtkm::source::Tangle tangle(dims);
  svtkm::cont::DataSet dataSet = tangle.Execute();

  svtkm::filter::Entropy entropyFilter;

  ///// calculate entropy of "nodevar" field of the data set /////
  entropyFilter.SetNumberOfBins(50); //set number of bins
  entropyFilter.SetActiveField("nodevar");
  svtkm::cont::DataSet resultEntropy = entropyFilter.Execute(dataSet);

  ///// get entropy from resultEntropy /////
  svtkm::cont::ArrayHandle<svtkm::Float64> entropy;
  resultEntropy.GetField("entropy").GetData().CopyTo(entropy);
  svtkm::cont::ArrayHandle<svtkm::Float64>::PortalConstControl portal =
    entropy.GetPortalConstControl();
  svtkm::Float64 entropyFromFilter = portal.Get(0);

  /////// check if calculating entopry is close enough to ground truth value /////
  // At least in one case, we are seeing a result which is off by more than
  // 0.001 due to floating point precision issues.
  // We are seeing this in the Reduce algorithm of the OpenMP backend, due to
  // operator+ being non-commutative for floating point numbers.
  // Therefore, this variance while high, is still allowed.
  // Instead of increasing the error threshold, we will just check against the
  // two known values.
  SVTKM_TEST_ASSERT(fabs(entropyFromFilter - 4.59093) < 0.001 ||
                     fabs(entropyFromFilter - 4.59798) < 0.001,
                   "Entropy calculation is incorrect");
} // TestFieldEntropy
}

int UnitTestEntropyFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestEntropy, argc, argv);
}
