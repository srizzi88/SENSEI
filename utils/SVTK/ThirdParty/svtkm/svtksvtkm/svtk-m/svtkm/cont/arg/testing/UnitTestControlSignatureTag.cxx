//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

void TestControlSignatures()
{
  SVTKM_IS_CONTROL_SIGNATURE_TAG(svtkm::worklet::WorkletMapField::FieldIn);

  SVTKM_TEST_ASSERT(svtkm::cont::arg::internal::ControlSignatureTagCheck<
                     svtkm::worklet::WorkletMapField::FieldIn>::Valid,
                   "Bad check for FieldIn");

  SVTKM_TEST_ASSERT(svtkm::cont::arg::internal::ControlSignatureTagCheck<
                     svtkm::worklet::WorkletMapField::FieldOut>::Valid,
                   "Bad check for FieldOut");

  SVTKM_TEST_ASSERT(
    !svtkm::cont::arg::internal::ControlSignatureTagCheck<svtkm::exec::arg::WorkIndex>::Valid,
    "Bad check for WorkIndex");

  SVTKM_TEST_ASSERT(!svtkm::cont::arg::internal::ControlSignatureTagCheck<svtkm::Id>::Valid,
                   "Bad check for svtkm::Id");
}

} // anonymous namespace

int UnitTestControlSignatureTag(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestControlSignatures, argc, argv);
}
