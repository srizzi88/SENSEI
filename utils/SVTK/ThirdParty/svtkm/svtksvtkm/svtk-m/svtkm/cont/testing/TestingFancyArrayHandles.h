//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_testing_TestingFancyArrayHandles_h
#define svtk_m_cont_testing_TestingFancyArrayHandles_h

#include <svtkm/VecTraits.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCast.h>
#include <svtkm/cont/ArrayHandleCompositeVector.h>
#include <svtkm/cont/ArrayHandleConcatenate.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandleDiscard.h>
#include <svtkm/cont/ArrayHandleGroupVec.h>
#include <svtkm/cont/ArrayHandleGroupVecVariable.h>
#include <svtkm/cont/ArrayHandleImplicit.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/ArrayHandleMultiplexer.h>
#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/ArrayHandleSOA.h>
#include <svtkm/cont/ArrayHandleTransform.h>
#include <svtkm/cont/ArrayHandleView.h>
#include <svtkm/cont/ArrayHandleZip.h>
#include <svtkm/cont/VirtualObjectHandle.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/cont/testing/Testing.h>

#include <vector>

namespace fancy_array_detail
{

template <typename ValueType>
struct IndexSquared
{
  SVTKM_EXEC_CONT
  ValueType operator()(svtkm::Id index) const
  {
    using ComponentType = typename svtkm::VecTraits<ValueType>::ComponentType;
    return ValueType(static_cast<ComponentType>(index * index));
  }
};

template <typename ValueType>
struct ValueSquared
{
  template <typename U>
  SVTKM_EXEC_CONT ValueType operator()(U u) const
  {
    return svtkm::Dot(u, u);
  }
};

struct ValueScale
{
  ValueScale()
    : Factor(1.0)
  {
  }

  ValueScale(svtkm::Float64 factor)
    : Factor(factor)
  {
  }

  template <typename ValueType>
  SVTKM_EXEC_CONT ValueType operator()(const ValueType& v) const
  {
    using Traits = svtkm::VecTraits<ValueType>;
    using TTraits = svtkm::TypeTraits<ValueType>;
    using ComponentType = typename Traits::ComponentType;

    ValueType result = TTraits::ZeroInitialization();
    for (svtkm::IdComponent i = 0; i < Traits::GetNumberOfComponents(v); ++i)
    {
      svtkm::Float64 vi = static_cast<svtkm::Float64>(Traits::GetComponent(v, i));
      svtkm::Float64 ri = vi * this->Factor;
      Traits::SetComponent(result, i, static_cast<ComponentType>(ri));
    }
    return result;
  }

private:
  svtkm::Float64 Factor;
};

struct InverseValueScale
{
  InverseValueScale()
    : InverseFactor(1.0)
  {
  }

  InverseValueScale(svtkm::Float64 factor)
    : InverseFactor(1.0 / factor)
  {
  }

  template <typename ValueType>
  SVTKM_EXEC_CONT ValueType operator()(const ValueType& v) const
  {
    using Traits = svtkm::VecTraits<ValueType>;
    using TTraits = svtkm::TypeTraits<ValueType>;
    using ComponentType = typename Traits::ComponentType;

    ValueType result = TTraits::ZeroInitialization();
    for (svtkm::IdComponent i = 0; i < Traits::GetNumberOfComponents(v); ++i)
    {
      svtkm::Float64 vi = static_cast<svtkm::Float64>(Traits::GetComponent(v, i));
      svtkm::Float64 ri = vi * this->InverseFactor;
      Traits::SetComponent(result, i, static_cast<ComponentType>(ri));
    }
    return result;
  }

private:
  svtkm::Float64 InverseFactor;
};

template <typename ValueType>
struct VirtualTransformFunctorBase : public svtkm::VirtualObjectBase
{
  VirtualTransformFunctorBase() = default;

  SVTKM_EXEC_CONT
  virtual ValueType operator()(const ValueType& v) const = 0;
};

template <typename ValueType, typename FunctorType>
struct VirtualTransformFunctor : VirtualTransformFunctorBase<ValueType>
{
  FunctorType Functor;

  SVTKM_CONT
  VirtualTransformFunctor(const FunctorType& functor)
    : Functor(functor)
  {
  }

  SVTKM_EXEC_CONT
  ValueType operator()(const ValueType& v) const override { return this->Functor(v); }
};

template <typename ValueType>
struct TransformExecObject : public svtkm::cont::ExecutionAndControlObjectBase
{
  svtkm::cont::VirtualObjectHandle<VirtualTransformFunctorBase<ValueType>> VirtualFunctor;

  SVTKM_CONT TransformExecObject() = default;

  template <typename FunctorType>
  SVTKM_CONT TransformExecObject(const FunctorType& functor)
  {
    // Need to make sure the serial device is supported, since that is what is used on the
    // control side. Therefore we reset to all supported devices.
    svtkm::cont::ScopedRuntimeDeviceTracker scopedTracker(
      svtkm::cont::DeviceAdapterTagSerial{}, svtkm::cont::RuntimeDeviceTrackerMode::Enable);
    this->VirtualFunctor.Reset(new VirtualTransformFunctor<ValueType, FunctorType>(functor));
  }

  struct FunctorWrapper
  {
    const VirtualTransformFunctorBase<ValueType>* FunctorPointer;

    FunctorWrapper() = default;

    SVTKM_CONT
    FunctorWrapper(const VirtualTransformFunctorBase<ValueType>* functorPointer)
      : FunctorPointer(functorPointer)
    {
    }

    template <typename InValueType>
    SVTKM_EXEC ValueType operator()(const InValueType& value) const
    {
      return (*this->FunctorPointer)(value);
    }
  };

  template <typename DeviceAdapterTag>
  SVTKM_CONT FunctorWrapper PrepareForExecution(DeviceAdapterTag device) const
  {
    return FunctorWrapper(this->VirtualFunctor.PrepareForExecution(device));
  }

  SVTKM_CONT FunctorWrapper PrepareForControl() const
  {
    return FunctorWrapper(this->VirtualFunctor.Get());
  }
};
}

namespace svtkm
{
namespace cont
{
namespace testing
{

/// This class has a single static member, Run, that tests that all Fancy Array
/// Handles work with the given DeviceAdapter
///
template <class DeviceAdapterTag>
struct TestingFancyArrayHandles
{

private:
  static const int ARRAY_SIZE = 10;

public:
  struct PassThrough : public svtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn, FieldOut);
    using ExecutionSignature = _2(_1);

    template <class ValueType>
    SVTKM_EXEC ValueType operator()(const ValueType& inValue) const
    {
      return inValue;
    }
  };

  struct InplaceFunctorPair : public svtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldInOut);
    using ExecutionSignature = void(_1);

    template <typename T>
    SVTKM_EXEC void operator()(svtkm::Pair<T, T>& value) const
    {
      value.second = value.first;
    }
  };

#ifndef SVTKM_CUDA
private:
#endif

  struct TestArrayPortalSOA
  {
    template <typename ComponentType>
    SVTKM_CONT void operator()(ComponentType) const
    {
      constexpr svtkm::IdComponent NUM_COMPONENTS = 4;
      using ValueType = svtkm::Vec<ComponentType, NUM_COMPONENTS>;
      using ComponentArrayType = svtkm::cont::ArrayHandle<ComponentType>;
      using SOAPortalType =
        svtkm::internal::ArrayPortalSOA<ValueType, typename ComponentArrayType::PortalControl>;

      std::cout << "Test SOA portal reflects data in component portals." << std::endl;
      SOAPortalType soaPortalIn(ARRAY_SIZE);

      std::array<svtkm::cont::ArrayHandle<ComponentType>, NUM_COMPONENTS> implArrays;
      for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
      {
        svtkm::cont::ArrayHandle<ComponentType> array;
        array.Allocate(ARRAY_SIZE);
        auto portal = array.GetPortalControl();
        for (svtkm::IdComponent valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
        {
          portal.Set(valueIndex, TestValue(valueIndex, ValueType{})[componentIndex]);
        }

        soaPortalIn.SetPortal(componentIndex, portal);

        implArrays[static_cast<std::size_t>(componentIndex)] = array;
      }

      SVTKM_TEST_ASSERT(soaPortalIn.GetNumberOfValues() == ARRAY_SIZE);
      CheckPortal(soaPortalIn);

      std::cout << "Test data set in SOA portal gets set in component portals." << std::endl;
      SOAPortalType soaPortalOut(ARRAY_SIZE);
      for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
      {
        svtkm::cont::ArrayHandle<ComponentType> array;
        array.Allocate(ARRAY_SIZE);
        auto portal = array.GetPortalControl();
        soaPortalOut.SetPortal(componentIndex, portal);

        implArrays[static_cast<std::size_t>(componentIndex)] = array;
      }

      SetPortal(soaPortalOut);

      for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
      {
        auto portal = implArrays[static_cast<size_t>(componentIndex)].GetPortalConstControl();
        for (svtkm::Id valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
        {
          ComponentType x = TestValue(valueIndex, ValueType{})[componentIndex];
          SVTKM_TEST_ASSERT(test_equal(x, portal.Get(valueIndex)));
        }
      }
    }
  };

  struct TestSOAAsInput
  {
    template <typename ValueType>
    SVTKM_CONT void operator()(const ValueType svtkmNotUsed(v)) const
    {
      using VTraits = svtkm::VecTraits<ValueType>;
      using ComponentType = typename VTraits::ComponentType;
      constexpr svtkm::IdComponent NUM_COMPONENTS = VTraits::NUM_COMPONENTS;

      {
        svtkm::cont::ArrayHandleSOA<ValueType> soaArray;
        for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
             ++componentIndex)
        {
          svtkm::cont::ArrayHandle<ComponentType> componentArray;
          componentArray.Allocate(ARRAY_SIZE);
          auto componentPortal = componentArray.GetPortalControl();
          for (svtkm::Id valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
          {
            componentPortal.Set(
              valueIndex,
              VTraits::GetComponent(TestValue(valueIndex, ValueType{}), componentIndex));
          }
          soaArray.SetArray(componentIndex, componentArray);
        }

        SVTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
        SVTKM_TEST_ASSERT(soaArray.GetPortalConstControl().GetNumberOfValues() == ARRAY_SIZE);
        CheckPortal(soaArray.GetPortalConstControl());

        svtkm::cont::ArrayHandle<ValueType> basicArray;
        svtkm::cont::ArrayCopy(soaArray, basicArray);
        SVTKM_TEST_ASSERT(basicArray.GetNumberOfValues() == ARRAY_SIZE);
        CheckPortal(basicArray.GetPortalConstControl());
      }

      {
        // Check constructors
        using Vec3 = svtkm::Vec<ComponentType, 3>;
        std::vector<ComponentType> vector0;
        std::vector<ComponentType> vector1;
        std::vector<ComponentType> vector2;
        for (svtkm::Id valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
        {
          Vec3 value = TestValue(valueIndex, Vec3{});
          vector0.push_back(value[0]);
          vector1.push_back(value[1]);
          vector2.push_back(value[2]);
        }

        {
          svtkm::cont::ArrayHandleSOA<Vec3> soaArray = { vector0, vector1, vector2 };
          SVTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
          CheckPortal(soaArray.GetPortalConstControl());
        }

        {
          svtkm::cont::ArrayHandleSOA<Vec3> soaArray =
            svtkm::cont::make_ArrayHandleSOA<Vec3>({ vector0, vector1, vector2 });
          SVTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
          CheckPortal(soaArray.GetPortalConstControl());
        }

        {
          svtkm::cont::ArrayHandleSOA<Vec3> soaArray =
            svtkm::cont::make_ArrayHandleSOA(vector0, vector1, vector2);
          SVTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
          CheckPortal(soaArray.GetPortalConstControl());
        }

        {
          svtkm::cont::ArrayHandleSOA<Vec3> soaArray = svtkm::cont::make_ArrayHandleSOA<Vec3>(
            { &vector0.front(), &vector1.front(), &vector2.front() }, ARRAY_SIZE);
          SVTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
          CheckPortal(soaArray.GetPortalConstControl());
        }

        {
          svtkm::cont::ArrayHandleSOA<Vec3> soaArray = svtkm::cont::make_ArrayHandleSOA(
            ARRAY_SIZE, &vector0.front(), &vector1.front(), &vector2.front());
          SVTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
          CheckPortal(soaArray.GetPortalConstControl());
        }
      }
    }
  };

  struct TestSOAAsOutput
  {
    template <typename ValueType>
    SVTKM_CONT void operator()(const ValueType svtkmNotUsed(v)) const
    {
      using VTraits = svtkm::VecTraits<ValueType>;
      using ComponentType = typename VTraits::ComponentType;
      constexpr svtkm::IdComponent NUM_COMPONENTS = VTraits::NUM_COMPONENTS;

      svtkm::cont::ArrayHandle<ValueType> basicArray;
      basicArray.Allocate(ARRAY_SIZE);
      SetPortal(basicArray.GetPortalControl());

      svtkm::cont::ArrayHandleSOA<ValueType> soaArray;
      svtkm::cont::ArrayCopy(basicArray, soaArray);

      SVTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
      for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
      {
        svtkm::cont::ArrayHandle<ComponentType> componentArray = soaArray.GetArray(componentIndex);
        auto componentPortal = componentArray.GetPortalConstControl();
        for (svtkm::Id valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
        {
          ComponentType expected =
            VTraits::GetComponent(TestValue(valueIndex, ValueType{}), componentIndex);
          ComponentType got = componentPortal.Get(valueIndex);
          SVTKM_TEST_ASSERT(test_equal(expected, got));
        }
      }
    }
  };

  struct TestCompositeAsInput
  {
    template <typename ValueType>
    SVTKM_CONT void operator()(const ValueType svtkmNotUsed(v)) const
    {
      const ValueType value = TestValue(13, ValueType());
      std::vector<ValueType> compositeData(ARRAY_SIZE, value);
      svtkm::cont::ArrayHandle<ValueType> compositeInput =
        svtkm::cont::make_ArrayHandle(compositeData);

      auto composite =
        svtkm::cont::make_ArrayHandleCompositeVector(compositeInput, compositeInput, compositeInput);

      svtkm::cont::printSummary_ArrayHandle(composite, std::cout);
      std::cout << std::endl;

      svtkm::cont::ArrayHandle<svtkm::Vec<ValueType, 3>> result;

      svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(composite, result);

      //verify that the control portal works
      for (svtkm::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        const svtkm::Vec<ValueType, 3> result_v = result.GetPortalConstControl().Get(i);
        SVTKM_TEST_ASSERT(test_equal(result_v, svtkm::Vec<ValueType, 3>(value)),
                         "CompositeVector Handle Failed");

        const svtkm::Vec<ValueType, 3> result_c = composite.GetPortalConstControl().Get(i);
        SVTKM_TEST_ASSERT(test_equal(result_c, svtkm::Vec<ValueType, 3>(value)),
                         "CompositeVector Handle Failed");
      }
    }
  };

  struct TestConstantAsInput
  {
    template <typename ValueType>
    SVTKM_CONT void operator()(const ValueType svtkmNotUsed(v)) const
    {
      const ValueType value = TestValue(43, ValueType());

      svtkm::cont::ArrayHandleConstant<ValueType> constant =
        svtkm::cont::make_ArrayHandleConstant(value, ARRAY_SIZE);
      svtkm::cont::ArrayHandle<ValueType> result;

      svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(constant, result);

      svtkm::cont::printSummary_ArrayHandle(constant, std::cout);
      std::cout << std::endl;

      //verify that the control portal works
      for (svtkm::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        const ValueType result_v = result.GetPortalConstControl().Get(i);
        const ValueType control_value = constant.GetPortalConstControl().Get(i);
        SVTKM_TEST_ASSERT(test_equal(result_v, value), "Counting Handle Failed");
        SVTKM_TEST_ASSERT(test_equal(result_v, control_value), "Counting Handle Control Failed");
      }
    }
  };

  struct TestCountingAsInput
  {
    template <typename ValueType>
    SVTKM_CONT void operator()(const ValueType svtkmNotUsed(v)) const
    {
      using ComponentType = typename svtkm::VecTraits<ValueType>::ComponentType;

      const svtkm::Id length = ARRAY_SIZE;

      //need to initialize the start value or else vectors will have
      //random values to start
      ComponentType component_value(0);
      const ValueType start = ValueType(component_value);

      svtkm::cont::ArrayHandleCounting<ValueType> counting =
        svtkm::cont::make_ArrayHandleCounting(start, ValueType(1), length);
      svtkm::cont::ArrayHandle<ValueType> result;

      svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(counting, result);

      svtkm::cont::printSummary_ArrayHandle(counting, std::cout);
      std::cout << std::endl;

      //verify that the control portal works
      for (svtkm::Id i = 0; i < length; ++i)
      {
        const ValueType result_v = result.GetPortalConstControl().Get(i);
        const ValueType correct_value = ValueType(component_value);
        const ValueType control_value = counting.GetPortalConstControl().Get(i);
        SVTKM_TEST_ASSERT(test_equal(result_v, correct_value), "Counting Handle Failed");
        SVTKM_TEST_ASSERT(test_equal(result_v, control_value), "Counting Handle Control Failed");
        component_value = ComponentType(component_value + ComponentType(1));
      }
    }
  };

  struct TestImplicitAsInput
  {
    template <typename ValueType>
    SVTKM_CONT void operator()(const ValueType svtkmNotUsed(v)) const
    {
      const svtkm::Id length = ARRAY_SIZE;
      using FunctorType = ::fancy_array_detail::IndexSquared<ValueType>;
      FunctorType functor;

      svtkm::cont::ArrayHandleImplicit<FunctorType> implicit =
        svtkm::cont::make_ArrayHandleImplicit(functor, length);

      svtkm::cont::printSummary_ArrayHandle(implicit, std::cout);
      std::cout << std::endl;

      svtkm::cont::ArrayHandle<ValueType> result;

      svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(implicit, result);

      //verify that the control portal works
      for (svtkm::Id i = 0; i < length; ++i)
      {
        const ValueType result_v = result.GetPortalConstControl().Get(i);
        const ValueType correct_value = functor(i);
        const ValueType control_value = implicit.GetPortalConstControl().Get(i);
        SVTKM_TEST_ASSERT(test_equal(result_v, correct_value), "Implicit Handle Failed");
        SVTKM_TEST_ASSERT(test_equal(result_v, control_value), "Implicit Handle Failed");
      }
    }
  };

  struct TestConcatenateAsInput
  {
    template <typename ValueType>
    SVTKM_CONT void operator()(const ValueType svtkmNotUsed(v)) const
    {
      const svtkm::Id length = ARRAY_SIZE;

      using FunctorType = ::fancy_array_detail::IndexSquared<ValueType>;
      using ComponentType = typename svtkm::VecTraits<ValueType>::ComponentType;

      using ValueHandleType = svtkm::cont::ArrayHandleImplicit<FunctorType>;
      using BasicArrayType = svtkm::cont::ArrayHandle<ValueType>;
      using ConcatenateType = svtkm::cont::ArrayHandleConcatenate<ValueHandleType, BasicArrayType>;

      FunctorType functor;
      for (svtkm::Id start_pos = 0; start_pos < length; start_pos += length / 4)
      {
        svtkm::Id implicitLen = length - start_pos;
        svtkm::Id basicLen = start_pos;

        // make an implicit array
        ValueHandleType implicit = svtkm::cont::make_ArrayHandleImplicit(functor, implicitLen);
        // make a basic array
        std::vector<ValueType> basicVec;
        for (svtkm::Id i = 0; i < basicLen; i++)
        {
          basicVec.push_back(ValueType(static_cast<ComponentType>(i)));
          basicVec.push_back(ValueType(ComponentType(i)));
        }
        BasicArrayType basic = svtkm::cont::make_ArrayHandle(basicVec);

        // concatenate two arrays together
        ConcatenateType concatenate = svtkm::cont::make_ArrayHandleConcatenate(implicit, basic);
        svtkm::cont::printSummary_ArrayHandle(concatenate, std::cout);
        std::cout << std::endl;

        svtkm::cont::ArrayHandle<ValueType> result;

        svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
        dispatcher.Invoke(concatenate, result);

        //verify that the control portal works
        for (svtkm::Id i = 0; i < length; ++i)
        {
          const ValueType result_v = result.GetPortalConstControl().Get(i);
          ValueType correct_value;
          if (i < implicitLen)
            correct_value = implicit.GetPortalConstControl().Get(i);
          else
            correct_value = basic.GetPortalConstControl().Get(i - implicitLen);
          const ValueType control_value = concatenate.GetPortalConstControl().Get(i);
          SVTKM_TEST_ASSERT(test_equal(result_v, correct_value),
                           "ArrayHandleConcatenate as Input Failed");
          SVTKM_TEST_ASSERT(test_equal(result_v, control_value),
                           "ArrayHandleConcatenate as Input Failed");
        }
      }
    }
  };

  struct TestPermutationAsInput
  {
    template <typename ValueType>
    SVTKM_CONT void operator()(const ValueType svtkmNotUsed(v)) const
    {
      const svtkm::Id length = ARRAY_SIZE;

      using FunctorType = ::fancy_array_detail::IndexSquared<ValueType>;

      using KeyHandleType = svtkm::cont::ArrayHandleCounting<svtkm::Id>;
      using ValueHandleType = svtkm::cont::ArrayHandleImplicit<FunctorType>;
      using PermutationHandleType =
        svtkm::cont::ArrayHandlePermutation<KeyHandleType, ValueHandleType>;

      FunctorType functor;
      for (svtkm::Id start_pos = 0; start_pos < length; start_pos += length / 4)
      {
        const svtkm::Id counting_length = length - start_pos;

        KeyHandleType counting =
          svtkm::cont::make_ArrayHandleCounting<svtkm::Id>(start_pos, 1, counting_length);

        ValueHandleType implicit = svtkm::cont::make_ArrayHandleImplicit(functor, length);

        PermutationHandleType permutation =
          svtkm::cont::make_ArrayHandlePermutation(counting, implicit);

        svtkm::cont::printSummary_ArrayHandle(permutation, std::cout);
        std::cout << std::endl;

        svtkm::cont::ArrayHandle<ValueType> result;

        svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
        dispatcher.Invoke(permutation, result);

        //verify that the control portal works
        for (svtkm::Id i = 0; i < counting_length; ++i)
        {
          const svtkm::Id value_index = i;
          const svtkm::Id key_index = start_pos + i;

          const ValueType result_v = result.GetPortalConstControl().Get(value_index);
          const ValueType correct_value = implicit.GetPortalConstControl().Get(key_index);
          const ValueType control_value = permutation.GetPortalConstControl().Get(value_index);
          SVTKM_TEST_ASSERT(test_equal(result_v, correct_value), "Implicit Handle Failed");
          SVTKM_TEST_ASSERT(test_equal(result_v, control_value), "Implicit Handle Failed");
        }
      }
    }
  };

  struct TestViewAsInput
  {
    template <typename ValueType>
    SVTKM_CONT void operator()(const ValueType svtkmNotUsed(v)) const
    {
      const svtkm::Id length = ARRAY_SIZE;

      using FunctorType = ::fancy_array_detail::IndexSquared<ValueType>;

      using ValueHandleType = svtkm::cont::ArrayHandleImplicit<FunctorType>;
      using ViewHandleType = svtkm::cont::ArrayHandleView<ValueHandleType>;

      FunctorType functor;
      for (svtkm::Id start_pos = 0; start_pos < length; start_pos += length / 4)
      {
        const svtkm::Id counting_length = length - start_pos;

        ValueHandleType implicit = svtkm::cont::make_ArrayHandleImplicit(functor, length);

        ViewHandleType view =
          svtkm::cont::make_ArrayHandleView(implicit, start_pos, counting_length);

        svtkm::cont::printSummary_ArrayHandle(view, std::cout);
        std::cout << std::endl;

        svtkm::cont::ArrayHandle<ValueType> result;

        svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
        dispatcher.Invoke(view, result);

        //verify that the control portal works
        for (svtkm::Id i = 0; i < counting_length; ++i)
        {
          const svtkm::Id value_index = i;
          const svtkm::Id key_index = start_pos + i;

          const ValueType result_v = result.GetPortalConstControl().Get(value_index);
          const ValueType correct_value = implicit.GetPortalConstControl().Get(key_index);
          const ValueType control_value = view.GetPortalConstControl().Get(value_index);
          SVTKM_TEST_ASSERT(test_equal(result_v, correct_value), "Implicit Handle Failed");
          SVTKM_TEST_ASSERT(test_equal(result_v, control_value), "Implicit Handle Failed");
        }
      }
    }
  };

  struct TestTransformAsInput
  {
    template <typename ValueType>
    SVTKM_CONT void operator()(const ValueType svtkmNotUsed(v)) const
    {
      using FunctorType = fancy_array_detail::ValueScale;

      const svtkm::Id length = ARRAY_SIZE;
      FunctorType functor(2.0);

      svtkm::cont::ArrayHandle<ValueType> input;
      svtkm::cont::ArrayHandleTransform<svtkm::cont::ArrayHandle<ValueType>, FunctorType>
        transformed = svtkm::cont::make_ArrayHandleTransform(input, functor);

      input.Allocate(length);
      SetPortal(input.GetPortalControl());

      svtkm::cont::printSummary_ArrayHandle(transformed, std::cout);
      std::cout << std::endl;

      svtkm::cont::ArrayHandle<ValueType> result;

      svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(transformed, result);

      //verify that the control portal works
      for (svtkm::Id i = 0; i < length; ++i)
      {
        const ValueType result_v = result.GetPortalConstControl().Get(i);
        const ValueType correct_value = functor(TestValue(i, ValueType()));
        const ValueType control_value = transformed.GetPortalConstControl().Get(i);
        SVTKM_TEST_ASSERT(test_equal(result_v, correct_value), "Transform Handle Failed");
        SVTKM_TEST_ASSERT(test_equal(result_v, control_value), "Transform Handle Control Failed");
      }
    }
  };

  struct TestTransformVirtualAsInput
  {
    template <typename ValueType>
    SVTKM_CONT void operator()(const ValueType svtkmNotUsed(v)) const
    {
      using FunctorType = fancy_array_detail::ValueScale;
      using VirtualFunctorType = fancy_array_detail::TransformExecObject<ValueType>;

      const svtkm::Id length = ARRAY_SIZE;
      FunctorType functor(2.0);
      VirtualFunctorType virtualFunctor(functor);

      svtkm::cont::ArrayHandle<ValueType> input;
      auto transformed = svtkm::cont::make_ArrayHandleTransform(input, virtualFunctor);

      input.Allocate(length);
      SetPortal(input.GetPortalControl());

      svtkm::cont::printSummary_ArrayHandle(transformed, std::cout);
      std::cout << std::endl;

      svtkm::cont::ArrayHandle<ValueType> result;

      svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(transformed, result);

      //verify that the control portal works
      for (svtkm::Id i = 0; i < length; ++i)
      {
        const ValueType result_v = result.GetPortalConstControl().Get(i);
        const ValueType correct_value = functor(TestValue(i, ValueType()));
        const ValueType control_value = transformed.GetPortalConstControl().Get(i);
        SVTKM_TEST_ASSERT(test_equal(result_v, correct_value), "Transform Handle Failed");
        SVTKM_TEST_ASSERT(test_equal(result_v, control_value), "Transform Handle Control Failed");
      }
    }
  };

  struct TestCountingTransformAsInput
  {
    template <typename ValueType>
    SVTKM_CONT void operator()(const ValueType svtkmNotUsed(v)) const
    {
      using ComponentType = typename svtkm::VecTraits<ValueType>::ComponentType;
      using OutputValueType = ComponentType;
      using FunctorType = fancy_array_detail::ValueSquared<OutputValueType>;

      svtkm::Id length = ARRAY_SIZE;
      FunctorType functor;

      //need to initialize the start value or else vectors will have
      //random values to start
      ComponentType component_value(0);
      const ValueType start = ValueType(component_value);

      svtkm::cont::ArrayHandleCounting<ValueType> counting(start, ValueType(1), length);

      svtkm::cont::ArrayHandleTransform<svtkm::cont::ArrayHandleCounting<ValueType>, FunctorType>
        countingTransformed = svtkm::cont::make_ArrayHandleTransform(counting, functor);

      svtkm::cont::printSummary_ArrayHandle(countingTransformed, std::cout);
      std::cout << std::endl;

      svtkm::cont::ArrayHandle<OutputValueType> result;

      svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(countingTransformed, result);

      //verify that the control portal works
      for (svtkm::Id i = 0; i < length; ++i)
      {
        const OutputValueType result_v = result.GetPortalConstControl().Get(i);
        const OutputValueType correct_value = functor(ValueType(component_value));
        const OutputValueType control_value = countingTransformed.GetPortalConstControl().Get(i);
        SVTKM_TEST_ASSERT(test_equal(result_v, correct_value), "Transform Counting Handle Failed");
        SVTKM_TEST_ASSERT(test_equal(result_v, control_value),
                         "Transform Counting Handle Control Failed");
        component_value = ComponentType(component_value + ComponentType(1));
      }
    }
  };

  struct TestCastAsInput
  {
    template <typename CastToType>
    SVTKM_CONT void operator()(CastToType svtkmNotUsed(type)) const
    {
      using InputArrayType = svtkm::cont::ArrayHandleIndex;

      InputArrayType input(ARRAY_SIZE);
      svtkm::cont::ArrayHandleCast<CastToType, InputArrayType> castArray =
        svtkm::cont::make_ArrayHandleCast(input, CastToType());
      svtkm::cont::ArrayHandle<CastToType> result;

      svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(castArray, result);

      svtkm::cont::printSummary_ArrayHandle(castArray, std::cout);
      std::cout << std::endl;

      // verify results
      svtkm::Id length = ARRAY_SIZE;
      for (svtkm::Id i = 0; i < length; ++i)
      {
        SVTKM_TEST_ASSERT(result.GetPortalConstControl().Get(i) ==
                           static_cast<CastToType>(input.GetPortalConstControl().Get(i)),
                         "Casting ArrayHandle Failed");
      }
    }
  };

  struct TestCastAsOutput
  {
    template <typename CastFromType>
    SVTKM_CONT void operator()(CastFromType svtkmNotUsed(type)) const
    {
      using InputArrayType = svtkm::cont::ArrayHandleIndex;
      using ResultArrayType = svtkm::cont::ArrayHandle<CastFromType>;

      InputArrayType input(ARRAY_SIZE);

      ResultArrayType result;
      svtkm::cont::ArrayHandleCast<svtkm::Id, ResultArrayType> castArray =
        svtkm::cont::make_ArrayHandleCast<CastFromType>(result);

      svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(input, castArray);

      svtkm::cont::printSummary_ArrayHandle(castArray, std::cout);
      std::cout << std::endl;

      // verify results
      svtkm::Id length = ARRAY_SIZE;
      for (svtkm::Id i = 0; i < length; ++i)
      {
        SVTKM_TEST_ASSERT(input.GetPortalConstControl().Get(i) ==
                           static_cast<svtkm::Id>(result.GetPortalConstControl().Get(i)),
                         "Casting ArrayHandle Failed");
      }
    }
  };

  struct TestMultiplexerAsInput
  {
    svtkm::cont::Invoker Invoke;

    template <typename T>
    SVTKM_CONT void operator()(T svtkmNotUsed(type)) const
    {
      using InputArrayType = svtkm::cont::ArrayHandleCounting<T>;

      InputArrayType input(T(1), T(2), ARRAY_SIZE);
      svtkm::cont::ArrayHandleMultiplexer<
        svtkm::cont::ArrayHandle<T>,
        InputArrayType,
        svtkm::cont::ArrayHandleCast<T, svtkm::cont::ArrayHandleIndex>>
        multiplexArray(input);
      svtkm::cont::ArrayHandle<T> result;

      this->Invoke(PassThrough{}, multiplexArray, result);

      svtkm::cont::printSummary_ArrayHandle(multiplexArray, std::cout);
      std::cout << std::endl;

      // verify results
      SVTKM_TEST_ASSERT(
        test_equal_portals(result.GetPortalConstControl(), input.GetPortalConstControl()),
        "CastingArrayHandle failed");
    }
  };

  struct TestMultiplexerAsOutput
  {
    svtkm::cont::Invoker Invoke;

    template <typename CastFromType>
    SVTKM_CONT void operator()(CastFromType svtkmNotUsed(type)) const
    {
      using InputArrayType = svtkm::cont::ArrayHandleIndex;
      using ResultArrayType = svtkm::cont::ArrayHandle<CastFromType>;

      InputArrayType input(ARRAY_SIZE);

      ResultArrayType result;
      svtkm::cont::ArrayHandleMultiplexer<svtkm::cont::ArrayHandle<svtkm::Id>,
                                         svtkm::cont::ArrayHandleCast<svtkm::Id, ResultArrayType>>
        multiplexerArray = svtkm::cont::make_ArrayHandleCast<svtkm::Id>(result);

      this->Invoke(PassThrough{}, input, multiplexerArray);

      svtkm::cont::printSummary_ArrayHandle(multiplexerArray, std::cout);
      std::cout << std::endl;

      // verify results
      SVTKM_TEST_ASSERT(
        test_equal_portals(input.GetPortalConstControl(), result.GetPortalConstControl()),
        "Multiplexing ArrayHandle failed");
    }
  };

  template <svtkm::IdComponent NUM_COMPONENTS>
  struct TestGroupVecAsInput
  {
    template <typename ComponentType>
    SVTKM_CONT void operator()(ComponentType) const
    {
      using ValueType = svtkm::Vec<ComponentType, NUM_COMPONENTS>;

      ComponentType testValues[ARRAY_SIZE * NUM_COMPONENTS];

      for (svtkm::Id index = 0; index < ARRAY_SIZE * NUM_COMPONENTS; ++index)
      {
        testValues[index] = TestValue(index, ComponentType());
      }
      svtkm::cont::ArrayHandle<ComponentType> baseArray =
        svtkm::cont::make_ArrayHandle(testValues, ARRAY_SIZE * NUM_COMPONENTS);

      svtkm::cont::ArrayHandleGroupVec<svtkm::cont::ArrayHandle<ComponentType>, NUM_COMPONENTS>
        groupArray(baseArray);
      SVTKM_TEST_ASSERT(groupArray.GetNumberOfValues() == ARRAY_SIZE,
                       "Group array reporting wrong array size.");

      svtkm::cont::printSummary_ArrayHandle(groupArray, std::cout);
      std::cout << std::endl;

      svtkm::cont::ArrayHandle<ValueType> resultArray;

      svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(groupArray, resultArray);

      SVTKM_TEST_ASSERT(resultArray.GetNumberOfValues() == ARRAY_SIZE, "Got bad result array size.");

      //verify that the control portal works
      svtkm::Id totalIndex = 0;
      for (svtkm::Id index = 0; index < ARRAY_SIZE; ++index)
      {
        const ValueType result = resultArray.GetPortalConstControl().Get(index);
        for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
             componentIndex++)
        {
          const ComponentType expectedValue = TestValue(totalIndex, ComponentType());
          SVTKM_TEST_ASSERT(test_equal(result[componentIndex], expectedValue),
                           "Result array got wrong value.");
          totalIndex++;
        }
      }
    }
  };

  template <svtkm::IdComponent NUM_COMPONENTS>
  struct TestGroupVecAsOutput
  {
    template <typename ComponentType>
    SVTKM_CONT void operator()(ComponentType) const
    {
      using ValueType = svtkm::Vec<ComponentType, NUM_COMPONENTS>;

      svtkm::cont::ArrayHandle<ValueType> baseArray;
      baseArray.Allocate(ARRAY_SIZE);
      SetPortal(baseArray.GetPortalControl());

      svtkm::cont::ArrayHandle<ComponentType> resultArray;

      svtkm::cont::ArrayHandleGroupVec<svtkm::cont::ArrayHandle<ComponentType>, NUM_COMPONENTS>
        groupArray(resultArray);

      svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(baseArray, groupArray);

      svtkm::cont::printSummary_ArrayHandle(groupArray, std::cout);
      std::cout << std::endl;
      svtkm::cont::printSummary_ArrayHandle(resultArray, std::cout);
      std::cout << std::endl;

      SVTKM_TEST_ASSERT(groupArray.GetNumberOfValues() == ARRAY_SIZE,
                       "Group array reporting wrong array size.");

      SVTKM_TEST_ASSERT(resultArray.GetNumberOfValues() == ARRAY_SIZE * NUM_COMPONENTS,
                       "Got bad result array size.");

      //verify that the control portal works
      svtkm::Id totalIndex = 0;
      for (svtkm::Id index = 0; index < ARRAY_SIZE; ++index)
      {
        const ValueType expectedValue = TestValue(index, ValueType());
        for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
             componentIndex++)
        {
          const ComponentType result = resultArray.GetPortalConstControl().Get(totalIndex);
          SVTKM_TEST_ASSERT(test_equal(result, expectedValue[componentIndex]),
                           "Result array got wrong value.");
          totalIndex++;
        }
      }
    }
  };

  // GroupVecVariable is a bit strange because it supports values of different
  // lengths, so a simple pass through worklet will not work. Use custom
  // worklets.
  struct GroupVariableInputWorklet : public svtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn, FieldOut);
    using ExecutionSignature = void(_1, WorkIndex, _2);

    template <typename InputType>
    SVTKM_EXEC void operator()(const InputType& input, svtkm::Id workIndex, svtkm::Id& dummyOut) const
    {
      using ComponentType = typename InputType::ComponentType;
      svtkm::IdComponent expectedSize = static_cast<svtkm::IdComponent>(workIndex + 1);
      if (expectedSize != input.GetNumberOfComponents())
      {
        this->RaiseError("Got unexpected number of components.");
      }

      svtkm::Id valueIndex = workIndex * (workIndex + 1) / 2;
      dummyOut = valueIndex;
      for (svtkm::IdComponent componentIndex = 0; componentIndex < expectedSize; componentIndex++)
      {
        ComponentType expectedValue = TestValue(valueIndex, ComponentType());
        if (svtkm::Abs(expectedValue - input[componentIndex]) > 0.000001)
        {
          this->RaiseError("Got bad value in GroupVariableInputWorklet.");
        }
        valueIndex++;
      }
    }
  };

  struct TestGroupVecVariableAsInput
  {
    template <typename ComponentType>
    SVTKM_CONT void operator()(ComponentType) const
    {
      svtkm::Id sourceArraySize;

      svtkm::cont::ArrayHandleCounting<svtkm::IdComponent> numComponentsArray(1, 1, ARRAY_SIZE);
      svtkm::cont::ArrayHandle<svtkm::Id> offsetsArray =
        svtkm::cont::ConvertNumComponentsToOffsets(numComponentsArray, sourceArraySize);

      svtkm::cont::ArrayHandle<ComponentType> sourceArray;
      sourceArray.Allocate(sourceArraySize);
      SetPortal(sourceArray.GetPortalControl());

      svtkm::cont::printSummary_ArrayHandle(
        svtkm::cont::make_ArrayHandleGroupVecVariable(sourceArray, offsetsArray), std::cout);
      std::cout << std::endl;

      svtkm::cont::ArrayHandle<svtkm::Id> dummyArray;

      svtkm::worklet::DispatcherMapField<GroupVariableInputWorklet> dispatcher;
      dispatcher.Invoke(svtkm::cont::make_ArrayHandleGroupVecVariable(sourceArray, offsetsArray),
                        dummyArray);

      dummyArray.GetPortalConstControl();
    }
  };

  // GroupVecVariable is a bit strange because it supports values of different
  // lengths, so a simple pass through worklet will not work. Use custom
  // worklets.
  struct GroupVariableOutputWorklet : public svtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn, FieldOut);
    using ExecutionSignature = void(_2, WorkIndex);

    template <typename OutputType>
    SVTKM_EXEC void operator()(OutputType& output, svtkm::Id workIndex) const
    {
      using ComponentType = typename OutputType::ComponentType;
      svtkm::IdComponent expectedSize = static_cast<svtkm::IdComponent>(workIndex + 1);
      if (expectedSize != output.GetNumberOfComponents())
      {
        this->RaiseError("Got unexpected number of components.");
      }

      svtkm::Id valueIndex = workIndex * (workIndex + 1) / 2;
      for (svtkm::IdComponent componentIndex = 0; componentIndex < expectedSize; componentIndex++)
      {
        output[componentIndex] = TestValue(valueIndex, ComponentType());
        valueIndex++;
      }
    }
  };

  struct TestGroupVecVariableAsOutput
  {
    template <typename ComponentType>
    SVTKM_CONT void operator()(ComponentType) const
    {
      svtkm::Id sourceArraySize;

      svtkm::cont::ArrayHandleCounting<svtkm::IdComponent> numComponentsArray(1, 1, ARRAY_SIZE);
      svtkm::cont::ArrayHandle<svtkm::Id> offsetsArray = svtkm::cont::ConvertNumComponentsToOffsets(
        numComponentsArray, sourceArraySize, DeviceAdapterTag());

      svtkm::cont::ArrayHandle<ComponentType> sourceArray;
      sourceArray.Allocate(sourceArraySize);

      svtkm::worklet::DispatcherMapField<GroupVariableOutputWorklet> dispatcher;
      dispatcher.Invoke(svtkm::cont::ArrayHandleIndex(ARRAY_SIZE),
                        svtkm::cont::make_ArrayHandleGroupVecVariable(sourceArray, offsetsArray));

      svtkm::cont::printSummary_ArrayHandle(
        svtkm::cont::make_ArrayHandleGroupVecVariable(sourceArray, offsetsArray), std::cout);
      std::cout << std::endl;
      svtkm::cont::printSummary_ArrayHandle(sourceArray, std::cout);
      std::cout << std::endl;

      CheckPortal(sourceArray.GetPortalConstControl());
    }
  };

  struct TestZipAsInput
  {
    template <typename KeyType, typename ValueType>
    SVTKM_CONT void operator()(svtkm::Pair<KeyType, ValueType> svtkmNotUsed(pair)) const
    {
      using PairType = svtkm::Pair<KeyType, ValueType>;
      using KeyComponentType = typename svtkm::VecTraits<KeyType>::ComponentType;
      using ValueComponentType = typename svtkm::VecTraits<ValueType>::ComponentType;

      KeyType testKeys[ARRAY_SIZE];
      ValueType testValues[ARRAY_SIZE];

      for (svtkm::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        testKeys[i] = KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i));
        testValues[i] = ValueType(static_cast<ValueComponentType>(i));
      }
      svtkm::cont::ArrayHandle<KeyType> keys = svtkm::cont::make_ArrayHandle(testKeys, ARRAY_SIZE);
      svtkm::cont::ArrayHandle<ValueType> values =
        svtkm::cont::make_ArrayHandle(testValues, ARRAY_SIZE);

      svtkm::cont::ArrayHandleZip<svtkm::cont::ArrayHandle<KeyType>,
                                 svtkm::cont::ArrayHandle<ValueType>>
        zip = svtkm::cont::make_ArrayHandleZip(keys, values);

      svtkm::cont::printSummary_ArrayHandle(zip, std::cout);
      std::cout << std::endl;

      svtkm::cont::ArrayHandle<PairType> result;

      svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(zip, result);

      //verify that the control portal works
      for (int i = 0; i < ARRAY_SIZE; ++i)
      {
        const PairType result_v = result.GetPortalConstControl().Get(i);
        const PairType correct_value(KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i)),
                                     ValueType(static_cast<ValueComponentType>(i)));
        SVTKM_TEST_ASSERT(test_equal(result_v, correct_value), "ArrayHandleZip Failed as input");
      }
    }
  };

  struct TestDiscardAsOutput
  {
    template <typename ValueType>
    SVTKM_CONT void operator()(const ValueType svtkmNotUsed(v)) const
    {
      using DiscardHandleType = svtkm::cont::ArrayHandleDiscard<ValueType>;
      using ComponentType = typename svtkm::VecTraits<ValueType>::ComponentType;

      using Portal = typename svtkm::cont::ArrayHandle<ValueType>::PortalControl;

      const svtkm::Id length = ARRAY_SIZE;

      svtkm::cont::ArrayHandle<ValueType> input;
      input.Allocate(length);
      Portal inputPortal = input.GetPortalControl();
      for (svtkm::Id i = 0; i < length; ++i)
      {
        inputPortal.Set(i, ValueType(ComponentType(i)));
      }

      DiscardHandleType discard;
      discard.Allocate(length);

      svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(input, discard);

      // No output to verify since none is stored in memory. Just checking that
      // this compiles/runs without errors.
    }
  };

  struct TestPermutationAsOutput
  {
    template <typename ValueType>
    SVTKM_CONT void operator()(const ValueType svtkmNotUsed(v)) const
    {
      const svtkm::Id length = ARRAY_SIZE;

      using KeyHandleType = svtkm::cont::ArrayHandleCounting<svtkm::Id>;
      using ValueHandleType = svtkm::cont::ArrayHandle<ValueType>;
      using PermutationHandleType =
        svtkm::cont::ArrayHandlePermutation<KeyHandleType, ValueHandleType>;

      using ComponentType = typename svtkm::VecTraits<ValueType>::ComponentType;
      svtkm::cont::ArrayHandle<ValueType> input;
      using Portal = typename svtkm::cont::ArrayHandle<ValueType>::PortalControl;
      input.Allocate(length);
      Portal inputPortal = input.GetPortalControl();
      for (svtkm::Id i = 0; i < length; ++i)
      {
        inputPortal.Set(i, ValueType(ComponentType(i)));
      }

      ValueHandleType values;
      values.Allocate(length * 2);

      KeyHandleType counting = svtkm::cont::make_ArrayHandleCounting<svtkm::Id>(length, 1, length);

      PermutationHandleType permutation = svtkm::cont::make_ArrayHandlePermutation(counting, values);
      svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(input, permutation);

      svtkm::cont::printSummary_ArrayHandle(permutation, std::cout);
      std::cout << std::endl;

      //verify that the control portal works
      for (svtkm::Id i = 0; i < length; ++i)
      {
        const ValueType result_v = permutation.GetPortalConstControl().Get(i);
        const ValueType correct_value = ValueType(ComponentType(i));
        SVTKM_TEST_ASSERT(test_equal(result_v, correct_value),
                         "Permutation Handle Failed As Output");
      }
    }
  };

  struct TestViewAsOutput
  {
    template <typename ValueType>
    SVTKM_CONT void operator()(const ValueType svtkmNotUsed(v)) const
    {
      const svtkm::Id length = ARRAY_SIZE;

      using ValueHandleType = svtkm::cont::ArrayHandle<ValueType>;
      using ViewHandleType = svtkm::cont::ArrayHandleView<ValueHandleType>;

      using ComponentType = typename svtkm::VecTraits<ValueType>::ComponentType;
      svtkm::cont::ArrayHandle<ValueType> input;
      using Portal = typename svtkm::cont::ArrayHandle<ValueType>::PortalControl;
      input.Allocate(length);
      Portal inputPortal = input.GetPortalControl();
      for (svtkm::Id i = 0; i < length; ++i)
      {
        inputPortal.Set(i, ValueType(ComponentType(i)));
      }

      ValueHandleType values;
      values.Allocate(length * 2);

      ViewHandleType view = svtkm::cont::make_ArrayHandleView(values, length, length);
      svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(input, view);

      svtkm::cont::printSummary_ArrayHandle(view, std::cout);
      std::cout << std::endl;

      //verify that the control portal works
      for (svtkm::Id i = 0; i < length; ++i)
      {
        const ValueType result_v = view.GetPortalConstControl().Get(i);
        const ValueType correct_value = ValueType(ComponentType(i));
        SVTKM_TEST_ASSERT(test_equal(result_v, correct_value),
                         "Permutation Handle Failed As Output");
      }
    }
  };

  struct TestTransformAsOutput
  {
    template <typename ValueType>
    SVTKM_CONT void operator()(const ValueType svtkmNotUsed(v)) const
    {
      using FunctorType = fancy_array_detail::ValueScale;
      using InverseFunctorType = fancy_array_detail::InverseValueScale;

      const svtkm::Id length = ARRAY_SIZE;
      FunctorType functor(2.0);
      InverseFunctorType inverseFunctor(2.0);

      svtkm::cont::ArrayHandle<ValueType> input;
      input.Allocate(length);
      SetPortal(input.GetPortalControl());

      svtkm::cont::ArrayHandle<ValueType> output;
      auto transformed = svtkm::cont::make_ArrayHandleTransform(output, functor, inverseFunctor);

      svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(input, transformed);

      svtkm::cont::printSummary_ArrayHandle(transformed, std::cout);
      std::cout << std::endl;

      //verify that the control portal works
      for (svtkm::Id i = 0; i < length; ++i)
      {
        const ValueType result_v = output.GetPortalConstControl().Get(i);
        const ValueType correct_value = inverseFunctor(TestValue(i, ValueType()));
        const ValueType control_value = transformed.GetPortalConstControl().Get(i);
        SVTKM_TEST_ASSERT(test_equal(result_v, correct_value), "Transform Handle Failed");
        SVTKM_TEST_ASSERT(test_equal(functor(result_v), control_value),
                         "Transform Handle Control Failed");
      }
    }
  };

  struct TestTransformVirtualAsOutput
  {
    template <typename ValueType>
    SVTKM_CONT void operator()(const ValueType svtkmNotUsed(v)) const
    {
      using FunctorType = fancy_array_detail::ValueScale;
      using InverseFunctorType = fancy_array_detail::InverseValueScale;

      using VirtualFunctorType = fancy_array_detail::TransformExecObject<ValueType>;

      const svtkm::Id length = ARRAY_SIZE;
      FunctorType functor(2.0);
      InverseFunctorType inverseFunctor(2.0);

      VirtualFunctorType virtualFunctor(functor);
      VirtualFunctorType virtualInverseFunctor(inverseFunctor);

      svtkm::cont::ArrayHandle<ValueType> input;
      input.Allocate(length);
      SetPortal(input.GetPortalControl());

      svtkm::cont::ArrayHandle<ValueType> output;
      auto transformed =
        svtkm::cont::make_ArrayHandleTransform(output, virtualFunctor, virtualInverseFunctor);

      svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(input, transformed);

      svtkm::cont::printSummary_ArrayHandle(transformed, std::cout);
      std::cout << std::endl;

      //verify that the control portal works
      for (svtkm::Id i = 0; i < length; ++i)
      {
        const ValueType result_v = output.GetPortalConstControl().Get(i);
        const ValueType correct_value = inverseFunctor(TestValue(i, ValueType()));
        const ValueType control_value = transformed.GetPortalConstControl().Get(i);
        SVTKM_TEST_ASSERT(test_equal(result_v, correct_value), "Transform Handle Failed");
        SVTKM_TEST_ASSERT(test_equal(functor(result_v), control_value),
                         "Transform Handle Control Failed");
      }
    }
  };

  struct TestZipAsOutput
  {
    template <typename KeyType, typename ValueType>
    SVTKM_CONT void operator()(svtkm::Pair<KeyType, ValueType> svtkmNotUsed(pair)) const
    {
      using PairType = svtkm::Pair<KeyType, ValueType>;
      using KeyComponentType = typename svtkm::VecTraits<KeyType>::ComponentType;
      using ValueComponentType = typename svtkm::VecTraits<ValueType>::ComponentType;

      PairType testKeysAndValues[ARRAY_SIZE];
      for (svtkm::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        testKeysAndValues[i] = PairType(KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i)),
                                        ValueType(static_cast<ValueComponentType>(i)));
      }
      svtkm::cont::ArrayHandle<PairType> input =
        svtkm::cont::make_ArrayHandle(testKeysAndValues, ARRAY_SIZE);

      svtkm::cont::ArrayHandle<KeyType> result_keys;
      svtkm::cont::ArrayHandle<ValueType> result_values;
      svtkm::cont::ArrayHandleZip<svtkm::cont::ArrayHandle<KeyType>,
                                 svtkm::cont::ArrayHandle<ValueType>>
        result_zip = svtkm::cont::make_ArrayHandleZip(result_keys, result_values);

      svtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(input, result_zip);

      svtkm::cont::printSummary_ArrayHandle(result_zip, std::cout);
      std::cout << std::endl;

      //now the two arrays we have zipped should have data inside them
      for (int i = 0; i < ARRAY_SIZE; ++i)
      {
        const KeyType result_key = result_keys.GetPortalConstControl().Get(i);
        const ValueType result_value = result_values.GetPortalConstControl().Get(i);

        SVTKM_TEST_ASSERT(
          test_equal(result_key, KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i))),
          "ArrayHandleZip Failed as input for key");
        SVTKM_TEST_ASSERT(test_equal(result_value, ValueType(static_cast<ValueComponentType>(i))),
                         "ArrayHandleZip Failed as input for value");
      }
    }
  };

  struct TestZipAsInPlace
  {
    template <typename ValueType>
    SVTKM_CONT void operator()(ValueType) const
    {
      svtkm::cont::ArrayHandle<ValueType> inputValues;
      inputValues.Allocate(ARRAY_SIZE);
      SetPortal(inputValues.GetPortalControl());

      svtkm::cont::ArrayHandle<ValueType> outputValues;
      outputValues.Allocate(ARRAY_SIZE);

      svtkm::worklet::DispatcherMapField<InplaceFunctorPair> dispatcher;
      dispatcher.Invoke(svtkm::cont::make_ArrayHandleZip(inputValues, outputValues));

      svtkm::cont::printSummary_ArrayHandle(outputValues, std::cout);
      std::cout << std::endl;

      CheckPortal(outputValues.GetPortalConstControl());
    }
  };

  using ScalarTypesToTest = svtkm::List<svtkm::UInt8, svtkm::FloatDefault>;

  using ZipTypesToTest = svtkm::List<svtkm::Pair<svtkm::UInt8, svtkm::Id>,
                                    svtkm::Pair<svtkm::Float64, svtkm::Vec4ui_8>,
                                    svtkm::Pair<svtkm::Vec3f_32, svtkm::Vec4i_8>>;

  using HandleTypesToTest =
    svtkm::List<svtkm::Id, svtkm::Vec2i_32, svtkm::FloatDefault, svtkm::Vec3f_64>;

  using CastTypesToTest = svtkm::List<svtkm::Int32, svtkm::UInt32>;

  struct TestAll
  {
    SVTKM_CONT void operator()() const
    {
      std::cout << "Doing FancyArrayHandle tests" << std::endl;

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayPortalSOA" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestArrayPortalSOA(), ScalarTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleSOA as Input" << std::endl;
      svtkm::testing::Testing::TryTypes(TestingFancyArrayHandles<DeviceAdapterTag>::TestSOAAsInput(),
                                       HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleSOA as Output" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestSOAAsOutput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleCompositeVector as Input" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestCompositeAsInput(), ScalarTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleConstant as Input" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestConstantAsInput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleCounting as Input" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestCountingAsInput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleImplicit as Input" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestImplicitAsInput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandlePermutation as Input" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestPermutationAsInput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleView as Input" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestViewAsInput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleTransform as Input" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestTransformAsInput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleTransform with virtual as Input" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestTransformVirtualAsInput(),
        HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleTransform with Counting as Input" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestCountingTransformAsInput(),
        HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleCast as Input" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestCastAsInput(), CastTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleCast as Output" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestCastAsOutput(), CastTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleMultiplexer as Input" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestMultiplexerAsInput(), CastTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleMultiplexer as Output" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestMultiplexerAsOutput(), CastTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleGroupVec<3> as Input" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestGroupVecAsInput<3>(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleGroupVec<4> as Input" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestGroupVecAsInput<4>(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleGroupVec<2> as Output" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestGroupVecAsOutput<2>(), ScalarTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleGroupVec<3> as Output" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestGroupVecAsOutput<3>(), ScalarTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleGroupVecVariable as Input" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestGroupVecVariableAsInput(),
        ScalarTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleGroupVecVariable as Output" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestGroupVecVariableAsOutput(),
        ScalarTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleZip as Input" << std::endl;
      svtkm::testing::Testing::TryTypes(TestingFancyArrayHandles<DeviceAdapterTag>::TestZipAsInput(),
                                       ZipTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandlePermutation as Output" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestPermutationAsOutput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleView as Output" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestViewAsOutput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleTransform as Output" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestTransformAsOutput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleTransform with virtual as Output" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestTransformVirtualAsOutput(),
        HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleDiscard as Output" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestDiscardAsOutput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleZip as Output" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestZipAsOutput(), ZipTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleZip as In Place" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestZipAsInPlace(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleConcatenate as Input" << std::endl;
      svtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestConcatenateAsInput(), HandleTypesToTest());
    }
  };

public:
  /// Run a suite of tests to check to see if a DeviceAdapter properly supports
  /// all the fancy array handles that svtkm supports. Returns an
  /// error code that can be returned from the main function of a test.
  ///
  static SVTKM_CONT int Run(int argc, char* argv[])
  {
    svtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapterTag());
    return svtkm::cont::testing::Testing::Run(TestAll(), argc, argv);
  }
};
}
}
} // namespace svtkm::cont::testing

#endif //svtk_m_cont_testing_TestingFancyArrayHandles_h
