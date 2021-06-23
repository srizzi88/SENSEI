//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_testing_TestingVirtualObjectHandle_h
#define svtk_m_cont_testing_TestingVirtualObjectHandle_h

#include <svtkm/Types.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleTransform.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/cont/VirtualObjectHandle.h>
#include <svtkm/cont/testing/Testing.h>

#define ARRAY_LEN 8

namespace svtkm
{
namespace cont
{
namespace testing
{

namespace virtual_object_detail
{

class Transformer : public svtkm::VirtualObjectBase
{
public:
  SVTKM_EXEC
  virtual svtkm::FloatDefault Eval(svtkm::FloatDefault val) const = 0;
};

class Square : public Transformer
{
public:
  SVTKM_EXEC
  svtkm::FloatDefault Eval(svtkm::FloatDefault val) const override { return val * val; }
};

class Multiply : public Transformer
{
public:
  SVTKM_CONT
  void SetMultiplicand(svtkm::FloatDefault val)
  {
    this->Multiplicand = val;
    this->Modified();
  }

  SVTKM_CONT
  svtkm::FloatDefault GetMultiplicand() const { return this->Multiplicand; }

  SVTKM_EXEC
  svtkm::FloatDefault Eval(svtkm::FloatDefault val) const override
  {
    return val * this->Multiplicand;
  }

private:
  svtkm::FloatDefault Multiplicand = 0.0f;
};

class TransformerFunctor
{
public:
  TransformerFunctor() = default;
  explicit TransformerFunctor(const Transformer* impl)
    : Impl(impl)
  {
  }

  SVTKM_EXEC
  svtkm::FloatDefault operator()(svtkm::FloatDefault val) const { return this->Impl->Eval(val); }

private:
  const Transformer* Impl;
};

} // virtual_object_detail

template <typename DeviceAdapterList>
class TestingVirtualObjectHandle
{
private:
  using FloatArrayHandle = svtkm::cont::ArrayHandle<svtkm::FloatDefault>;
  using ArrayTransform =
    svtkm::cont::ArrayHandleTransform<FloatArrayHandle, virtual_object_detail::TransformerFunctor>;
  using TransformerHandle = svtkm::cont::VirtualObjectHandle<virtual_object_detail::Transformer>;

  class TestStage1
  {
  public:
    TestStage1(const FloatArrayHandle& input, TransformerHandle& handle)
      : Input(&input)
      , Handle(&handle)
    {
    }

    template <typename DeviceAdapter>
    void operator()(DeviceAdapter device) const
    {
      using Algorithm = svtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
      std::cout << "\tDeviceAdapter: " << svtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName()
                << std::endl;

      for (int n = 0; n < 2; ++n)
      {
        virtual_object_detail::TransformerFunctor tfnctr(this->Handle->PrepareForExecution(device));
        ArrayTransform transformed(*this->Input, tfnctr);

        FloatArrayHandle output;
        Algorithm::Copy(transformed, output);
        auto portal = output.GetPortalConstControl();
        for (svtkm::Id i = 0; i < ARRAY_LEN; ++i)
        {
          SVTKM_TEST_ASSERT(portal.Get(i) == FloatDefault(i * i), "\tIncorrect result");
        }
        std::cout << "\tSuccess." << std::endl;

        if (n == 0)
        {
          std::cout << "\tReleaseResources and test again..." << std::endl;
          this->Handle->ReleaseExecutionResources();
        }
      }
    }

  private:
    const FloatArrayHandle* Input;
    TransformerHandle* Handle;
  };

  class TestStage2
  {
  public:
    TestStage2(const FloatArrayHandle& input,
               virtual_object_detail::Multiply& mul,
               TransformerHandle& handle)
      : Input(&input)
      , Mul(&mul)
      , Handle(&handle)
    {
    }

    template <typename DeviceAdapter>
    void operator()(DeviceAdapter device) const
    {
      using Algorithm = svtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
      std::cout << "\tDeviceAdapter: " << svtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName()
                << std::endl;

      this->Mul->SetMultiplicand(2);
      for (int n = 0; n < 2; ++n)
      {
        virtual_object_detail::TransformerFunctor tfnctr(this->Handle->PrepareForExecution(device));
        ArrayTransform transformed(*this->Input, tfnctr);

        FloatArrayHandle output;
        Algorithm::Copy(transformed, output);
        auto portal = output.GetPortalConstControl();
        for (svtkm::Id i = 0; i < ARRAY_LEN; ++i)
        {
          SVTKM_TEST_ASSERT(portal.Get(i) == FloatDefault(i) * this->Mul->GetMultiplicand(),
                           "\tIncorrect result");
        }
        std::cout << "\tSuccess." << std::endl;

        if (n == 0)
        {
          std::cout << "\tUpdate and test again..." << std::endl;
          this->Mul->SetMultiplicand(3);
        }
      }
    }

  private:
    const FloatArrayHandle* Input;
    virtual_object_detail::Multiply* Mul;
    TransformerHandle* Handle;
  };

public:
  static void Run()
  {
    svtkm::cont::ArrayHandle<svtkm::FloatDefault> input;
    input.Allocate(ARRAY_LEN);
    auto portal = input.GetPortalControl();
    for (svtkm::Id i = 0; i < ARRAY_LEN; ++i)
    {
      portal.Set(i, svtkm::FloatDefault(i));
    }

    TransformerHandle handle;

    std::cout << "Testing with concrete type 1 (Square)..." << std::endl;
    virtual_object_detail::Square sqr;
    handle.Reset(&sqr, false, DeviceAdapterList());
    svtkm::ListForEach(TestStage1(input, handle), DeviceAdapterList());

    std::cout << "ReleaseResources..." << std::endl;
    handle.ReleaseResources();

    std::cout << "Testing with concrete type 2 (Multiply)..." << std::endl;
    virtual_object_detail::Multiply mul;
    handle.Reset(&mul, false, DeviceAdapterList());
    svtkm::ListForEach(TestStage2(input, mul, handle), DeviceAdapterList());
  }
};
}
}
} // svtkm::cont::testing

#endif
