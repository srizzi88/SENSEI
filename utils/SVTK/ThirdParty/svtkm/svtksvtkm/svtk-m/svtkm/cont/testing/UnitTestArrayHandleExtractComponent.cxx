//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCompositeVector.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandleExtractComponent.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

template <typename ValueType>
struct ExtractComponentTests
{
  using InputArray = svtkm::cont::ArrayHandle<svtkm::Vec<ValueType, 4>>;
  using ExtractArray = svtkm::cont::ArrayHandleExtractComponent<InputArray>;
  using ReferenceComponentArray = svtkm::cont::ArrayHandleCounting<ValueType>;
  using ReferenceCompositeArray =
    typename svtkm::cont::ArrayHandleCompositeVector<ReferenceComponentArray,
                                                    ReferenceComponentArray,
                                                    ReferenceComponentArray,
                                                    ReferenceComponentArray>;

  using DeviceTag = svtkm::cont::DeviceAdapterTagSerial;
  using Algo = svtkm::cont::DeviceAdapterAlgorithm<DeviceTag>;

  // This is used to build a ArrayHandleExtractComponent's internal array.
  ReferenceCompositeArray RefComposite;

  void ConstructReferenceArray()
  {
    // Build the Ref array
    const svtkm::Id numValues = 32;
    ReferenceComponentArray c1 = svtkm::cont::make_ArrayHandleCounting<ValueType>(3, 2, numValues);
    ReferenceComponentArray c2 = svtkm::cont::make_ArrayHandleCounting<ValueType>(2, 3, numValues);
    ReferenceComponentArray c3 = svtkm::cont::make_ArrayHandleCounting<ValueType>(4, 4, numValues);
    ReferenceComponentArray c4 = svtkm::cont::make_ArrayHandleCounting<ValueType>(1, 3, numValues);

    this->RefComposite = svtkm::cont::make_ArrayHandleCompositeVector(c1, c2, c3, c4);
  }

  InputArray BuildInputArray() const
  {
    InputArray result;
    Algo::Copy(this->RefComposite, result);
    return result;
  }

  void SanityCheck(svtkm::IdComponent component) const
  {
    InputArray composite = this->BuildInputArray();
    ExtractArray extract(composite, component);

    SVTKM_TEST_ASSERT(composite.GetNumberOfValues() == extract.GetNumberOfValues(),
                     "Number of values in copied ExtractComponent array does not match input.");
  }

  void ReadTestComponentExtraction(svtkm::IdComponent component) const
  {
    // Test that the expected values are read from an ExtractComponent array.
    InputArray composite = this->BuildInputArray();
    ExtractArray extract(composite, component);

    // Test reading the data back in the control env:
    this->ValidateReadTestArray(extract, component);

    // Copy the extract array in the execution environment to test reading:
    svtkm::cont::ArrayHandle<ValueType> execCopy;
    Algo::Copy(extract, execCopy);
    this->ValidateReadTestArray(execCopy, component);
  }

  template <typename ArrayHandleType>
  void ValidateReadTestArray(ArrayHandleType testArray, svtkm::IdComponent component) const
  {
    using RefVectorType = typename ReferenceCompositeArray::ValueType;
    using Traits = svtkm::VecTraits<RefVectorType>;

    auto testPortal = testArray.GetPortalConstControl();
    auto refPortal = this->RefComposite.GetPortalConstControl();

    SVTKM_TEST_ASSERT(testPortal.GetNumberOfValues() == refPortal.GetNumberOfValues(),
                     "Number of values in read test output do not match input.");

    for (svtkm::Id i = 0; i < testPortal.GetNumberOfValues(); ++i)
    {
      SVTKM_TEST_ASSERT(
        test_equal(testPortal.Get(i), Traits::GetComponent(refPortal.Get(i), component), 0.),
        "Value mismatch in read test.");
    }
  }

  // Doubles the specified component (reading from RefVectorType).
  template <typename PortalType, typename RefPortalType>
  struct WriteTestFunctor : svtkm::exec::FunctorBase
  {
    using RefVectorType = typename RefPortalType::ValueType;
    using Traits = svtkm::VecTraits<RefVectorType>;

    PortalType Portal;
    RefPortalType RefPortal;
    svtkm::IdComponent Component;

    SVTKM_CONT
    WriteTestFunctor(const PortalType& portal,
                     const RefPortalType& ref,
                     svtkm::IdComponent component)
      : Portal(portal)
      , RefPortal(ref)
      , Component(component)
    {
    }

    SVTKM_EXEC_CONT
    void operator()(svtkm::Id index) const
    {
      this->Portal.Set(index,
                       Traits::GetComponent(this->RefPortal.Get(index), this->Component) * 2);
    }
  };

  void WriteTestComponentExtraction(svtkm::IdComponent component) const
  {
    // Control test:
    {
      InputArray composite = this->BuildInputArray();
      ExtractArray extract(composite, component);

      WriteTestFunctor<typename ExtractArray::PortalControl,
                       typename ReferenceCompositeArray::PortalConstControl>
        functor(extract.GetPortalControl(), this->RefComposite.GetPortalConstControl(), component);

      for (svtkm::Id i = 0; i < extract.GetNumberOfValues(); ++i)
      {
        functor(i);
      }

      this->ValidateWriteTestArray(composite, component);
    }

    // Exec test:
    {
      InputArray composite = this->BuildInputArray();
      ExtractArray extract(composite, component);

      using Portal = typename ExtractArray::template ExecutionTypes<DeviceTag>::Portal;
      using RefPortal =
        typename ReferenceCompositeArray::template ExecutionTypes<DeviceTag>::PortalConst;

      WriteTestFunctor<Portal, RefPortal> functor(extract.PrepareForInPlace(DeviceTag()),
                                                  this->RefComposite.PrepareForInput(DeviceTag()),
                                                  component);

      Algo::Schedule(functor, extract.GetNumberOfValues());
      this->ValidateWriteTestArray(composite, component);
    }
  }

  void ValidateWriteTestArray(InputArray testArray, svtkm::IdComponent component) const
  {
    using VectorType = typename ReferenceCompositeArray::ValueType;
    using Traits = svtkm::VecTraits<VectorType>;

    // Check that the indicated component is twice the reference value.
    auto refPortal = this->RefComposite.GetPortalConstControl();
    auto portal = testArray.GetPortalConstControl();

    SVTKM_TEST_ASSERT(portal.GetNumberOfValues() == refPortal.GetNumberOfValues(),
                     "Number of values in write test output do not match input.");

    for (svtkm::Id i = 0; i < portal.GetNumberOfValues(); ++i)
    {
      auto value = portal.Get(i);
      auto refValue = refPortal.Get(i);
      Traits::SetComponent(refValue, component, Traits::GetComponent(refValue, component) * 2);

      SVTKM_TEST_ASSERT(test_equal(refValue, value, 0.), "Value mismatch in write test.");
    }
  }

  void TestComponent(svtkm::IdComponent component) const
  {
    this->SanityCheck(component);
    this->ReadTestComponentExtraction(component);
    this->WriteTestComponentExtraction(component);
  }

  void operator()()
  {
    this->ConstructReferenceArray();

    this->TestComponent(0);
    this->TestComponent(1);
    this->TestComponent(2);
    this->TestComponent(3);
  }
};

struct ArgToTemplateType
{
  template <typename ValueType>
  void operator()(ValueType) const
  {
    ExtractComponentTests<ValueType>()();
  }
};

void TestArrayHandleExtractComponent()
{
  using TestTypes = svtkm::List<svtkm::Int32, svtkm::Int64, svtkm::Float32, svtkm::Float64>;
  svtkm::testing::Testing::TryTypes(ArgToTemplateType(), TestTypes());
}

} // end anon namespace

int UnitTestArrayHandleExtractComponent(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestArrayHandleExtractComponent, argc, argv);
}
