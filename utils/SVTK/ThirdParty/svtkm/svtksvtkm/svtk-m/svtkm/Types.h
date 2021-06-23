//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_Types_h
#define svtk_m_Types_h

#include <svtkm/internal/Configure.h>
#include <svtkm/internal/ExportMacros.h>

#include <svtkm/Assert.h>
#include <svtkm/StaticAssert.h>

#include <iostream>
#include <type_traits>

/*!
 * \namespace svtkm
 * \brief SVTK-m Toolkit.
 *
 * svtkm is the namespace for the SVTK-m Toolkit. It contains other sub namespaces,
 * as well as basic data types and functions callable from all components in SVTK-m
 * toolkit.
 *
 * \namespace svtkm::cont
 * \brief SVTK-m Control Environment.
 *
 * svtkm::cont defines the publicly accessible API for the SVTK-m Control
 * Environment. Users of the SVTK-m Toolkit can use this namespace to access the
 * Control Environment.
 *
 * \namespace svtkm::cont::arg
 * \brief Transportation controls for Control Environment Objects.
 *
 * svtkm::cont::arg includes the classes that allows the svtkm::worklet::Dispatchers
 * to request Control Environment Objects to be transferred to the Execution Environment.
 *
 * \namespace svtkm::cont::cuda
 * \brief CUDA implementation for Control Environment.
 *
 * svtkm::cont::cuda includes the code to implement the SVTK-m Control Environment
 * for the CUDA-based device adapter.
 *
 * \namespace svtkm::cont::openmp
 * \brief OPenMP implementation for Control Environment.
 *
 * svtkm::cont::openmp includes the code to implement the SVTK-m Control Environment
 * for the OpenMP-based device adapter.
 *
 * \namespace svtkm::cont::serial
 * \brief Serial implementation for Control Environment.
 *
 * svtkm::cont::serial includes the code to implement the SVTK-m Control Environment
 * for the serial device adapter.
 *
 * \namespace svtkm::cont::tbb
 * \brief TBB implementation for Control Environment.
 *
 * svtkm::cont::tbb includes the code to implement the SVTK-m Control Environment
 * for the TBB-based device adapter.
 *
 * \namespace svtkm::exec
 * \brief SVTK-m Execution Environment.
 *
 * svtkm::exec defines the publicly accessible API for the SVTK-m Execution
 * Environment. Worklets typically use classes/apis defined within this
 * namespace alone.
 *
 * \namespace svtkm::exec::cuda
 * \brief CUDA implementation for Execution Environment.
 *
 * svtkm::exec::cuda includes the code to implement the SVTK-m Execution Environment
 * for the CUDA-based device adapter.
 *
* \namespace svtkm::exec::openmp
 * \brief CUDA implementation for Execution Environment.
 *
 * svtkm::exec::openmp includes the code to implement the SVTK-m Execution Environment
 * for the OpenMP device adapter.
 *
 * \namespace svtkm::exec::serial
 * \brief CUDA implementation for Execution Environment.
 *
 * svtkm::exec::serial includes the code to implement the SVTK-m Execution Environment
 * for the serial device adapter.
 *
 * \namespace svtkm::exec::tbb
 * \brief TBB implementation for Execution Environment.
 *
 * svtkm::exec::tbb includes the code to implement the SVTK-m Execution Environment
 * for the TBB device adapter.
 *
 * \namespace svtkm::filter
 * \brief SVTK-m Filters
 *
 * svtkm::filter is the collection of predefined filters that take data as input
 * and write new data as output. Filters operate on svtkm::cont::DataSet objects,
 * svtkm::cont::Fields, and other runtime typeless objects.
 *
 * \namespace svtkm::internal
 * \brief SVTK-m Internal Environment
 *
 * svtkm::internal defines API which is internal and subject to frequent
 * change. This should not be used for projects using SVTK-m. Instead it servers
 * are a reference for the developers of SVTK-m.
 *
 * \namespace svtkm::interop
 * \brief SVTK-m OpenGL Interoperability
 *
 * svtkm::interop defines the publicly accessible API for interoperability between
 * svtkm and OpenGL.
 *
 * \namespace svtkm::io
 * \brief SVTK-m File input and output classes
 *
 * svtkm::io defines API for basic reading of SVTK files. Intended to be used for
 * examples and testing.
 *
 * \namespace svtkm::rendering
 * \brief SVTK-m Rendering
 *
 * svtkm::rendering defines API for
 *
 * \namespace svtkm::source
 * \brief SVTK-m Input source such as Wavelet
 *
 * svtkm::source is the collection of predefined sources that generate data.
 *
 * \namespace svtkm::testing
 * \brief Internal testing classes
 *
 * \namespace svtkm::worklet
 * \brief SVTK-m Worklets
 *
 * svtkm::worklet defines API for the low level worklets that operate on an element of data,
 * and the dispatcher that execute them in the execution environment.
 *
 * SVTK-m provides numerous worklet implementations. These worklet implementations for the most
 * part provide the underlying implementations of the algorithms in svtkm::filter.
 *
 */

namespace svtkm
{
//*****************************************************************************
// Typedefs for basic types.
//*****************************************************************************
using Float32 = float;
using Float64 = double;
using Int8 = signed char;
using UInt8 = unsigned char;
using Int16 = short;
using UInt16 = unsigned short;
using Int32 = int;
using UInt32 = unsigned int;

/// Represents a component ID (index of component in a vector). The number
/// of components, being a value fixed at compile time, is generally assumed
/// to be quite small. However, we are currently using a 32-bit width
/// integer because modern processors tend to access them more efficiently
/// than smaller widths.
using IdComponent = svtkm::Int32;

/// The default word size used for atomic bitwise operations. Universally
/// supported on all devices.
using WordTypeDefault = svtkm::UInt32;

//In this order so that we exactly match the logic that exists in SVTK
#if SVTKM_SIZE_LONG_LONG == 8
using Int64 = long long;
using UInt64 = unsigned long long;
#elif SVTKM_SIZE_LONG == 8
using Int64 = signed long;
using UInt64 = unsigned long;
#else
#error Could not find a 64-bit integer.
#endif

/// Represents an ID (index into arrays).
#ifdef SVTKM_USE_64BIT_IDS
using Id = svtkm::Int64;
#else
using Id = svtkm::Int32;
#endif

/// The floating point type to use when no other precision is specified.
#ifdef SVTKM_USE_DOUBLE_PRECISION
using FloatDefault = svtkm::Float64;
#else
using FloatDefault = svtkm::Float32;
#endif

namespace internal
{

//-----------------------------------------------------------------------------

/// Placeholder class for when a type is not applicable.
///
struct NullType
{
};

//-----------------------------------------------------------------------------
template <svtkm::IdComponent Size>
struct VecComponentWiseUnaryOperation
{
  template <typename T, typename UnaryOpType>
  inline SVTKM_EXEC_CONT T operator()(const T& v, const UnaryOpType& unaryOp) const
  {
    T result;
    for (svtkm::IdComponent i = 0; i < Size; ++i)
    {
      result[i] = unaryOp(v[i]);
    }
    return result;
  }
};

template <>
struct VecComponentWiseUnaryOperation<1>
{
  template <typename T, typename UnaryOpType>
  inline SVTKM_EXEC_CONT T operator()(const T& v, const UnaryOpType& unaryOp) const
  {
    return T(unaryOp(v[0]));
  }
};

template <>
struct VecComponentWiseUnaryOperation<2>
{
  template <typename T, typename UnaryOpType>
  inline SVTKM_EXEC_CONT T operator()(const T& v, const UnaryOpType& unaryOp) const
  {
    return T(unaryOp(v[0]), unaryOp(v[1]));
  }
};

template <>
struct VecComponentWiseUnaryOperation<3>
{
  template <typename T, typename UnaryOpType>
  inline SVTKM_EXEC_CONT T operator()(const T& v, const UnaryOpType& unaryOp) const
  {
    return T(unaryOp(v[0]), unaryOp(v[1]), unaryOp(v[2]));
  }
};

template <>
struct VecComponentWiseUnaryOperation<4>
{
  template <typename T, typename UnaryOpType>
  inline SVTKM_EXEC_CONT T operator()(const T& v, const UnaryOpType& unaryOp) const
  {
    return T(unaryOp(v[0]), unaryOp(v[1]), unaryOp(v[2]), unaryOp(v[3]));
  }
};

template <typename T, typename BinaryOpType, typename ReturnT = T>
struct BindLeftBinaryOp
{
  // Warning: a reference.
  const T& LeftValue;
  const BinaryOpType BinaryOp;
  SVTKM_EXEC_CONT
  BindLeftBinaryOp(const T& leftValue, BinaryOpType binaryOp = BinaryOpType())
    : LeftValue(leftValue)
    , BinaryOp(binaryOp)
  {
  }

  template <typename RightT>
  SVTKM_EXEC_CONT ReturnT operator()(const RightT& rightValue) const
  {
    return static_cast<ReturnT>(this->BinaryOp(this->LeftValue, static_cast<T>(rightValue)));
  }

private:
  void operator=(const BindLeftBinaryOp<T, BinaryOpType, ReturnT>&) = delete;
};

template <typename T, typename BinaryOpType, typename ReturnT = T>
struct BindRightBinaryOp
{
  // Warning: a reference.
  const T& RightValue;
  const BinaryOpType BinaryOp;
  SVTKM_EXEC_CONT
  BindRightBinaryOp(const T& rightValue, BinaryOpType binaryOp = BinaryOpType())
    : RightValue(rightValue)
    , BinaryOp(binaryOp)
  {
  }

  template <typename LeftT>
  SVTKM_EXEC_CONT ReturnT operator()(const LeftT& leftValue) const
  {
    return static_cast<ReturnT>(this->BinaryOp(static_cast<T>(leftValue), this->RightValue));
  }

private:
  void operator=(const BindRightBinaryOp<T, BinaryOpType, ReturnT>&) = delete;
};

} // namespace internal

// Disable conversion warnings for Add, Subtract, Multiply, Divide on GCC only.
// GCC creates false positive warnings for signed/unsigned char* operations.
// This occurs because the values are implicitly casted up to int's for the
// operation, and than  casted back down to char's when return.
// This causes a false positive warning, even when the values is within
// the value types range
#if (defined(SVTKM_GCC) || defined(SVTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc || clang
struct Add
{
  template <typename T>
  inline SVTKM_EXEC_CONT T operator()(const T& a, const T& b) const
  {
    return T(a + b);
  }
};

struct Subtract
{
  template <typename T>
  inline SVTKM_EXEC_CONT T operator()(const T& a, const T& b) const
  {
    return T(a - b);
  }
};

struct Multiply
{
  template <typename T>
  inline SVTKM_EXEC_CONT T operator()(const T& a, const T& b) const
  {
    return T(a * b);
  }
};

struct Divide
{
  template <typename T>
  inline SVTKM_EXEC_CONT T operator()(const T& a, const T& b) const
  {
    return T(a / b);
  }
};

struct Negate
{
  template <typename T>
  inline SVTKM_EXEC_CONT T operator()(const T& x) const
  {
    return T(-x);
  }
};

#if (defined(SVTKM_GCC) || defined(SVTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang

//-----------------------------------------------------------------------------

// Pre declaration
template <typename T, svtkm::IdComponent Size>
class SVTKM_ALWAYS_EXPORT Vec;

template <typename T>
class SVTKM_ALWAYS_EXPORT VecC;

template <typename T>
class SVTKM_ALWAYS_EXPORT VecCConst;

namespace detail
{

/// Base implementation of all Vec and VecC classes.
///
// Disable conversion warnings for Add, Subtract, Multiply, Divide on GCC only.
// GCC creates false positive warnings for signed/unsigned char* operations.
// This occurs because the values are implicitly casted up to int's for the
// operation, and than  casted back down to char's when return.
// This causes a false positive warning, even when the values is within
// the value types range
//
// NVCC 7.5 and below does not recognize this pragma inside of class bodies,
// so put them before entering the class.
//
#if (defined(SVTKM_CUDA) && (__CUDACC_VER_MAJOR__ < 8))
#if (defined(SVTKM_GCC) || defined(SVTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif // gcc || clang
#endif // use cuda < 8
template <typename T, typename DerivedClass>
class SVTKM_ALWAYS_EXPORT VecBaseCommon
{
public:
  using ComponentType = T;

protected:
  VecBaseCommon() = default;

  SVTKM_EXEC_CONT
  const DerivedClass& Derived() const { return *static_cast<const DerivedClass*>(this); }

  SVTKM_EXEC_CONT
  DerivedClass& Derived() { return *static_cast<DerivedClass*>(this); }

private:
  // Only for internal use
  SVTKM_EXEC_CONT
  inline svtkm::IdComponent NumComponents() const { return this->Derived().GetNumberOfComponents(); }

  // Only for internal use
  SVTKM_EXEC_CONT
  inline const T& Component(svtkm::IdComponent index) const { return this->Derived()[index]; }

  // Only for internal use
  SVTKM_EXEC_CONT
  inline T& Component(svtkm::IdComponent index) { return this->Derived()[index]; }

public:
  template <svtkm::IdComponent OtherSize>
  SVTKM_EXEC_CONT void CopyInto(svtkm::Vec<ComponentType, OtherSize>& dest) const
  {
    for (svtkm::IdComponent index = 0; (index < this->NumComponents()) && (index < OtherSize);
         index++)
    {
      dest[index] = this->Component(index);
    }
  }

  template <typename OtherComponentType, typename OtherVecType>
  SVTKM_EXEC_CONT DerivedClass& operator=(
    const svtkm::detail::VecBaseCommon<OtherComponentType, OtherVecType>& src)
  {
    const OtherVecType& srcDerived = static_cast<const OtherVecType&>(src);
    SVTKM_ASSERT(this->NumComponents() == srcDerived.GetNumberOfComponents());
    for (svtkm::IdComponent i = 0; i < this->NumComponents(); ++i)
    {
      this->Component(i) = OtherComponentType(srcDerived[i]);
    }
    return this->Derived();
  }

  SVTKM_EXEC_CONT
  bool operator==(const DerivedClass& other) const
  {
    bool equal = true;
    for (svtkm::IdComponent i = 0; i < this->NumComponents() && equal; ++i)
    {
      equal = (this->Component(i) == other[i]);
    }
    return equal;
  }

  SVTKM_EXEC_CONT
  bool operator<(const DerivedClass& other) const
  {
    for (svtkm::IdComponent i = 0; i < this->NumComponents(); ++i)
    {
      // ignore equals as that represents check next value
      if (this->Component(i) < other[i])
      {
        return true;
      }
      else if (other[i] < this->Component(i))
      {
        return false;
      }
    } // if all same we are not less

    return false;
  }

  SVTKM_EXEC_CONT
  bool operator!=(const DerivedClass& other) const { return !(this->operator==(other)); }

#if (!(defined(SVTKM_CUDA) && (__CUDACC_VER_MAJOR__ < 8)))
#if (defined(SVTKM_GCC) || defined(SVTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif // gcc || clang
#endif // not using cuda < 8

  template <svtkm::IdComponent Size>
  inline SVTKM_EXEC_CONT svtkm::Vec<ComponentType, Size> operator+(
    const svtkm::Vec<ComponentType, Size>& other) const
  {
    SVTKM_ASSERT(Size == this->NumComponents());
    svtkm::Vec<ComponentType, Size> result;
    for (svtkm::IdComponent i = 0; i < Size; ++i)
    {
      result[i] = this->Component(i) + other[i];
    }
    return result;
  }

  template <typename OtherClass>
  inline SVTKM_EXEC_CONT DerivedClass& operator+=(
    const VecBaseCommon<ComponentType, OtherClass>& other)
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    SVTKM_ASSERT(this->NumComponents() == other_derived.GetNumberOfComponents());
    for (svtkm::IdComponent i = 0; i < this->NumComponents(); ++i)
    {
      this->Component(i) += other_derived[i];
    }
    return this->Derived();
  }

  template <svtkm::IdComponent Size>
  inline SVTKM_EXEC_CONT svtkm::Vec<ComponentType, Size> operator-(
    const svtkm::Vec<ComponentType, Size>& other) const
  {
    SVTKM_ASSERT(Size == this->NumComponents());
    svtkm::Vec<ComponentType, Size> result;
    for (svtkm::IdComponent i = 0; i < Size; ++i)
    {
      result[i] = this->Component(i) - other[i];
    }
    return result;
  }

  template <typename OtherClass>
  inline SVTKM_EXEC_CONT DerivedClass& operator-=(
    const VecBaseCommon<ComponentType, OtherClass>& other)
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    SVTKM_ASSERT(this->NumComponents() == other_derived.GetNumberOfComponents());
    for (svtkm::IdComponent i = 0; i < this->NumComponents(); ++i)
    {
      this->Component(i) -= other_derived[i];
    }
    return this->Derived();
  }

  template <svtkm::IdComponent Size>
  inline SVTKM_EXEC_CONT svtkm::Vec<ComponentType, Size> operator*(
    const svtkm::Vec<ComponentType, Size>& other) const
  {
    svtkm::Vec<ComponentType, Size> result;
    for (svtkm::IdComponent i = 0; i < Size; ++i)
    {
      result[i] = this->Component(i) * other[i];
    }
    return result;
  }

  template <typename OtherClass>
  inline SVTKM_EXEC_CONT DerivedClass& operator*=(
    const VecBaseCommon<ComponentType, OtherClass>& other)
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    SVTKM_ASSERT(this->NumComponents() == other_derived.GetNumberOfComponents());
    for (svtkm::IdComponent i = 0; i < this->NumComponents(); ++i)
    {
      this->Component(i) *= other_derived[i];
    }
    return this->Derived();
  }

  template <svtkm::IdComponent Size>
  inline SVTKM_EXEC_CONT svtkm::Vec<ComponentType, Size> operator/(
    const svtkm::Vec<ComponentType, Size>& other) const
  {
    svtkm::Vec<ComponentType, Size> result;
    for (svtkm::IdComponent i = 0; i < Size; ++i)
    {
      result[i] = this->Component(i) / other[i];
    }
    return result;
  }

  template <typename OtherClass>
  SVTKM_EXEC_CONT DerivedClass& operator/=(const VecBaseCommon<ComponentType, OtherClass>& other)
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    SVTKM_ASSERT(this->NumComponents() == other_derived.GetNumberOfComponents());
    for (svtkm::IdComponent i = 0; i < this->NumComponents(); ++i)
    {
      this->Component(i) /= other_derived[i];
    }
    return this->Derived();
  }

#if (!(defined(SVTKM_CUDA) && (__CUDACC_VER_MAJOR__ < 8)))
#if (defined(SVTKM_GCC) || defined(SVTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang
#endif // not using cuda < 8

  SVTKM_EXEC_CONT
  ComponentType* GetPointer() { return &this->Component(0); }

  SVTKM_EXEC_CONT
  const ComponentType* GetPointer() const { return &this->Component(0); }
};


/// Base implementation of all Vec classes.
///
template <typename T, svtkm::IdComponent Size, typename DerivedClass>
class SVTKM_ALWAYS_EXPORT VecBase : public svtkm::detail::VecBaseCommon<T, DerivedClass>
{
public:
  using ComponentType = T;
  static constexpr svtkm::IdComponent NUM_COMPONENTS = Size;

  VecBase() = default;

  // The enable_if predicate will disable this constructor for Size=1 so that
  // the variadic constructor constexpr VecBase(T, Ts&&...) is called instead.
  template <svtkm::IdComponent Size2 = Size, typename std::enable_if<Size2 != 1, int>::type = 0>
  SVTKM_EXEC_CONT explicit VecBase(const ComponentType& value)
  {
    for (svtkm::IdComponent i = 0; i < Size; ++i)
    {
      this->Components[i] = value;
    }
  }

  template <typename... Ts>
  SVTKM_EXEC_CONT constexpr VecBase(ComponentType value0, Ts&&... values)
    : Components{ value0, values... }
  {
    SVTKM_STATIC_ASSERT(sizeof...(Ts) + 1 == Size);
  }

  SVTKM_EXEC_CONT
  VecBase(std::initializer_list<ComponentType> values)
  {
    ComponentType* dest = this->Components;
    auto src = values.begin();
    if (values.size() == 1)
    {
      for (svtkm::IdComponent i = 0; i < Size; ++i)
      {
        this->Components[i] = *src;
        ++dest;
      }
    }
    else
    {
      SVTKM_ASSERT((values.size() == NUM_COMPONENTS) &&
                  "Vec object initialized wrong number of components.");
      for (; src != values.end(); ++src)
      {
        *dest = *src;
        ++dest;
      }
    }
  }

#if (!(defined(SVTKM_CUDA) && (__CUDACC_VER_MAJOR__ < 8)))
#if (defined(SVTKM_GCC) || defined(SVTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif // gcc || clang
#endif //not using cuda < 8
#if defined(SVTKM_MSVC)
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

  template <typename OtherValueType, typename OtherDerivedType>
  SVTKM_EXEC_CONT explicit VecBase(const VecBase<OtherValueType, Size, OtherDerivedType>& src)
  {
    //DO NOT CHANGE THIS AND THE ABOVE PRAGMA'S UNLESS YOU FULLY UNDERSTAND THE
    //ISSUE https://gitlab.kitware.com/svtk/svtk-m/issues/221
    for (svtkm::IdComponent i = 0; i < Size; ++i)
    {
      this->Components[i] = src[i];
    }
  }

public:
  inline SVTKM_EXEC_CONT constexpr svtkm::IdComponent GetNumberOfComponents() const
  {
    return NUM_COMPONENTS;
  }

  inline SVTKM_EXEC_CONT constexpr const ComponentType& operator[](svtkm::IdComponent idx) const
  {
    return this->Components[idx];
  }

  inline SVTKM_EXEC_CONT ComponentType& operator[](svtkm::IdComponent idx)
  {
    SVTKM_ASSERT(idx >= 0);
    SVTKM_ASSERT(idx < NUM_COMPONENTS);
    return this->Components[idx];
  }


  template <typename OtherComponentType, typename OtherClass>
  inline SVTKM_EXEC_CONT DerivedClass
  operator+(const VecBaseCommon<OtherComponentType, OtherClass>& other) const
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    SVTKM_ASSERT(NUM_COMPONENTS == other_derived.GetNumberOfComponents());

    DerivedClass result;
    for (svtkm::IdComponent i = 0; i < NUM_COMPONENTS; ++i)
    {
      result[i] = this->Components[i] + static_cast<ComponentType>(other_derived[i]);
    }
    return result;
  }

  template <typename OtherComponentType, typename OtherClass>
  inline SVTKM_EXEC_CONT DerivedClass
  operator-(const VecBaseCommon<OtherComponentType, OtherClass>& other) const
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    SVTKM_ASSERT(NUM_COMPONENTS == other_derived.GetNumberOfComponents());

    DerivedClass result;
    for (svtkm::IdComponent i = 0; i < NUM_COMPONENTS; ++i)
    {
      result[i] = this->Components[i] - static_cast<ComponentType>(other_derived[i]);
    }
    return result;
  }

  template <typename OtherComponentType, typename OtherClass>
  inline SVTKM_EXEC_CONT DerivedClass
  operator*(const VecBaseCommon<OtherComponentType, OtherClass>& other) const
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    SVTKM_ASSERT(NUM_COMPONENTS == other_derived.GetNumberOfComponents());

    DerivedClass result;
    for (svtkm::IdComponent i = 0; i < NUM_COMPONENTS; ++i)
    {
      result[i] = this->Components[i] * static_cast<ComponentType>(other_derived[i]);
    }
    return result;
  }

  template <typename OtherComponentType, typename OtherClass>
  inline SVTKM_EXEC_CONT DerivedClass
  operator/(const VecBaseCommon<OtherComponentType, OtherClass>& other) const
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    SVTKM_ASSERT(NUM_COMPONENTS == other_derived.GetNumberOfComponents());

    DerivedClass result;
    for (svtkm::IdComponent i = 0; i < NUM_COMPONENTS; ++i)
    {
      result[i] = this->Components[i] / static_cast<ComponentType>(other_derived[i]);
    }
    return result;
  }

#if (!(defined(SVTKM_CUDA) && (__CUDACC_VER_MAJOR__ < 8)))
#if (defined(SVTKM_GCC) || defined(SVTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang
#endif // not using cuda < 8
#if defined(SVTKM_MSVC)
#pragma warning(pop)
#endif

protected:
  ComponentType Components[NUM_COMPONENTS];
};

#if (defined(SVTKM_CUDA) && (__CUDACC_VER_MAJOR__ < 8))
#if (defined(SVTKM_GCC) || defined(SVTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang
#endif // use cuda < 8

/// Base of all VecC and VecCConst classes.
///
template <typename T, typename DerivedClass>
class SVTKM_ALWAYS_EXPORT VecCBase : public svtkm::detail::VecBaseCommon<T, DerivedClass>
{
protected:
  SVTKM_EXEC_CONT
  VecCBase() {}
};

} // namespace detail

//-----------------------------------------------------------------------------

/// \brief A short fixed-length array.
///
/// The \c Vec templated class holds a short array of values of a size and
/// type specified by the template arguments.
///
/// The \c Vec class is most often used to represent vectors in the
/// mathematical sense as a quantity with a magnitude and direction. Vectors
/// are, of course, used extensively in computational geometry as well as
/// physical simulations. The \c Vec class can be (and is) repurposed for more
/// general usage of holding a fixed-length sequence of objects.
///
/// There is no real limit to the size of the sequence (other than the largest
/// number representable by svtkm::IdComponent), but the \c Vec class is really
/// designed for small sequences (seldom more than 10).
///
template <typename T, svtkm::IdComponent Size>
class SVTKM_ALWAYS_EXPORT Vec : public detail::VecBase<T, Size, Vec<T, Size>>
{
  using Superclass = detail::VecBase<T, Size, Vec<T, Size>>;

public:
#ifdef SVTKM_DOXYGEN_ONLY
  using ComponentType = T;
  static constexpr svtkm::IdComponent NUM_COMPONENTS = Size;
#endif

  using Superclass::Superclass;
  Vec() = default;
#if defined(_MSC_VER) && _MSC_VER < 1910
  template <typename... Ts>
  constexpr Vec(T value, Ts&&... values)
    : Superclass(value, std::forward<Ts>(values)...)
  {
  }
#endif

  inline SVTKM_EXEC_CONT void CopyInto(Vec<T, Size>& dest) const { dest = *this; }
};

//-----------------------------------------------------------------------------
// Specializations for common small tuples. We implement them a bit specially.

// A vector of size 0 cannot use VecBase because it will try to create a
// zero length array which troubles compilers. Vecs of size 0 are a bit
// pointless but might occur in some generic functions or classes.
template <typename T>
class SVTKM_ALWAYS_EXPORT Vec<T, 0>
{
public:
  using ComponentType = T;
  static constexpr svtkm::IdComponent NUM_COMPONENTS = 0;

  Vec() = default;
  SVTKM_EXEC_CONT explicit Vec(const ComponentType&) {}

  template <typename OtherType>
  SVTKM_EXEC_CONT Vec(const Vec<OtherType, NUM_COMPONENTS>&)
  {
  }

  SVTKM_EXEC_CONT
  Vec<ComponentType, NUM_COMPONENTS>& operator=(const Vec<ComponentType, NUM_COMPONENTS>&)
  {
    return *this;
  }

  inline SVTKM_EXEC_CONT constexpr svtkm::IdComponent GetNumberOfComponents() const
  {
    return NUM_COMPONENTS;
  }

  SVTKM_EXEC_CONT
  constexpr ComponentType operator[](svtkm::IdComponent svtkmNotUsed(idx)) const
  {
    return ComponentType();
  }

  SVTKM_EXEC_CONT
  bool operator==(const Vec<T, NUM_COMPONENTS>& svtkmNotUsed(other)) const { return true; }
  SVTKM_EXEC_CONT
  bool operator!=(const Vec<T, NUM_COMPONENTS>& svtkmNotUsed(other)) const { return false; }
};

// Vectors of size 1 should implicitly convert between the scalar and the
// vector. Otherwise, it should behave the same.
template <typename T>
class SVTKM_ALWAYS_EXPORT Vec<T, 1> : public detail::VecBase<T, 1, Vec<T, 1>>
{
  using Superclass = detail::VecBase<T, 1, Vec<T, 1>>;

public:
  Vec() = default;
  SVTKM_EXEC_CONT constexpr Vec(const T& value)
    : Superclass(value)
  {
  }

  template <typename OtherType>
  SVTKM_EXEC_CONT Vec(const Vec<OtherType, 1>& src)
    : Superclass(src)
  {
  }
};

//-----------------------------------------------------------------------------
// Specializations for common tuple sizes (with special names).

template <typename T>
class SVTKM_ALWAYS_EXPORT Vec<T, 2> : public detail::VecBase<T, 2, Vec<T, 2>>
{
  using Superclass = detail::VecBase<T, 2, Vec<T, 2>>;

public:
  Vec() = default;
  SVTKM_EXEC_CONT Vec(const T& value)
    : Superclass(value)
  {
  }

  template <typename OtherType>
  SVTKM_EXEC_CONT Vec(const Vec<OtherType, 2>& src)
    : Superclass(src)
  {
  }

  SVTKM_EXEC_CONT
  constexpr Vec(const T& x, const T& y)
    : Superclass(x, y)
  {
  }
};

/// \brief Id2 corresponds to a 2-dimensional index.
///
using Id2 = svtkm::Vec<svtkm::Id, 2>;

/// \brief IdComponent2 corresponds to an index to a local (small) 2-d array or equivalent.
///
using IdComponent2 = svtkm::Vec<svtkm::IdComponent, 2>;

/// \brief Vec2f corresponds to a 2-dimensional vector of floating point values.
///
/// Each floating point value is of the default precision (i.e. svtkm::FloatDefault). It is
/// typedef for svtkm::Vec<svtkm::FloatDefault, 2>.
///
using Vec2f = svtkm::Vec<svtkm::FloatDefault, 2>;

/// \brief Vec2f_32 corresponds to a 2-dimensional vector of 32-bit floating point values.
///
/// It is typedef for svtkm::Vec<svtkm::Float32, 2>.
///
using Vec2f_32 = svtkm::Vec<svtkm::Float32, 2>;

/// \brief Vec2f_64 corresponds to a 2-dimensional vector of 64-bit floating point values.
///
/// It is typedef for svtkm::Vec<svtkm::Float64, 2>.
///
using Vec2f_64 = svtkm::Vec<svtkm::Float64, 2>;

/// \brief Vec2i corresponds to a 2-dimensional vector of integer values.
///
/// Each integer value is of the default precision (i.e. svtkm::Id).
///
using Vec2i = svtkm::Vec<svtkm::Id, 2>;

/// \brief Vec2i_8 corresponds to a 2-dimensional vector of 8-bit integer values.
///
/// It is typedef for svtkm::Vec<svtkm::Int32, 2>.
///
using Vec2i_8 = svtkm::Vec<svtkm::Int8, 2>;

/// \brief Vec2i_16 corresponds to a 2-dimensional vector of 16-bit integer values.
///
/// It is typedef for svtkm::Vec<svtkm::Int32, 2>.
///
using Vec2i_16 = svtkm::Vec<svtkm::Int16, 2>;

/// \brief Vec2i_32 corresponds to a 2-dimensional vector of 32-bit integer values.
///
/// It is typedef for svtkm::Vec<svtkm::Int32, 2>.
///
using Vec2i_32 = svtkm::Vec<svtkm::Int32, 2>;

/// \brief Vec2i_64 corresponds to a 2-dimensional vector of 64-bit integer values.
///
/// It is typedef for svtkm::Vec<svtkm::Int64, 2>.
///
using Vec2i_64 = svtkm::Vec<svtkm::Int64, 2>;

/// \brief Vec2ui corresponds to a 2-dimensional vector of unsigned integer values.
///
/// Each integer value is of the default precision (following svtkm::Id).
///
#ifdef SVTKM_USE_64BIT_IDS
using Vec2ui = svtkm::Vec<svtkm::UInt64, 2>;
#else
using Vec2ui = svtkm::Vec<svtkm::UInt32, 2>;
#endif

/// \brief Vec2ui_8 corresponds to a 2-dimensional vector of 8-bit unsigned integer values.
///
/// It is typedef for svtkm::Vec<svtkm::UInt32, 2>.
///
using Vec2ui_8 = svtkm::Vec<svtkm::UInt8, 2>;

/// \brief Vec2ui_16 corresponds to a 2-dimensional vector of 16-bit unsigned integer values.
///
/// It is typedef for svtkm::Vec<svtkm::UInt32, 2>.
///
using Vec2ui_16 = svtkm::Vec<svtkm::UInt16, 2>;

/// \brief Vec2ui_32 corresponds to a 2-dimensional vector of 32-bit unsigned integer values.
///
/// It is typedef for svtkm::Vec<svtkm::UInt32, 2>.
///
using Vec2ui_32 = svtkm::Vec<svtkm::UInt32, 2>;

/// \brief Vec2ui_64 corresponds to a 2-dimensional vector of 64-bit unsigned integer values.
///
/// It is typedef for svtkm::Vec<svtkm::UInt64, 2>.
///
using Vec2ui_64 = svtkm::Vec<svtkm::UInt64, 2>;

template <typename T>
class SVTKM_ALWAYS_EXPORT Vec<T, 3> : public detail::VecBase<T, 3, Vec<T, 3>>
{
  using Superclass = detail::VecBase<T, 3, Vec<T, 3>>;

public:
  Vec() = default;
  SVTKM_EXEC_CONT Vec(const T& value)
    : Superclass(value)
  {
  }

  template <typename OtherType>
  SVTKM_EXEC_CONT Vec(const Vec<OtherType, 3>& src)
    : Superclass(src)
  {
  }

  SVTKM_EXEC_CONT
  constexpr Vec(const T& x, const T& y, const T& z)
    : Superclass(x, y, z)
  {
  }
};

/// \brief Id3 corresponds to a 3-dimensional index for 3d arrays.
///
/// Note that the precision of each index may be less than svtkm::Id.
///
using Id3 = svtkm::Vec<svtkm::Id, 3>;

/// \brief IdComponent2 corresponds to an index to a local (small) 3-d array or equivalent.
///
using IdComponent3 = svtkm::Vec<svtkm::IdComponent, 3>;

/// \brief Vec3f corresponds to a 3-dimensional vector of floating point values.
///
/// Each floating point value is of the default precision (i.e. svtkm::FloatDefault). It is
/// typedef for svtkm::Vec<svtkm::FloatDefault, 3>.
///
using Vec3f = svtkm::Vec<svtkm::FloatDefault, 3>;

/// \brief Vec3f_32 corresponds to a 3-dimensional vector of 32-bit floating point values.
///
/// It is typedef for svtkm::Vec<svtkm::Float32, 3>.
///
using Vec3f_32 = svtkm::Vec<svtkm::Float32, 3>;

/// \brief Vec3f_64 corresponds to a 3-dimensional vector of 64-bit floating point values.
///
/// It is typedef for svtkm::Vec<svtkm::Float64, 3>.
///
using Vec3f_64 = svtkm::Vec<svtkm::Float64, 3>;

/// \brief Vec3i corresponds to a 3-dimensional vector of integer values.
///
/// Each integer value is of the default precision (i.e. svtkm::Id).
///
using Vec3i = svtkm::Vec<svtkm::Id, 3>;

/// \brief Vec3i_8 corresponds to a 3-dimensional vector of 8-bit integer values.
///
/// It is typedef for svtkm::Vec<svtkm::Int32, 3>.
///
using Vec3i_8 = svtkm::Vec<svtkm::Int8, 3>;

/// \brief Vec3i_16 corresponds to a 3-dimensional vector of 16-bit integer values.
///
/// It is typedef for svtkm::Vec<svtkm::Int32, 3>.
///
using Vec3i_16 = svtkm::Vec<svtkm::Int16, 3>;

/// \brief Vec3i_32 corresponds to a 3-dimensional vector of 32-bit integer values.
///
/// It is typedef for svtkm::Vec<svtkm::Int32, 3>.
///
using Vec3i_32 = svtkm::Vec<svtkm::Int32, 3>;

/// \brief Vec3i_64 corresponds to a 3-dimensional vector of 64-bit integer values.
///
/// It is typedef for svtkm::Vec<svtkm::Int64, 3>.
///
using Vec3i_64 = svtkm::Vec<svtkm::Int64, 3>;

/// \brief Vec3ui corresponds to a 3-dimensional vector of unsigned integer values.
///
/// Each integer value is of the default precision (following svtkm::Id).
///
#ifdef SVTKM_USE_64BIT_IDS
using Vec3ui = svtkm::Vec<svtkm::UInt64, 3>;
#else
using Vec3ui = svtkm::Vec<svtkm::UInt32, 3>;
#endif

/// \brief Vec3ui_8 corresponds to a 3-dimensional vector of 8-bit unsigned integer values.
///
/// It is typedef for svtkm::Vec<svtkm::UInt32, 3>.
///
using Vec3ui_8 = svtkm::Vec<svtkm::UInt8, 3>;

/// \brief Vec3ui_16 corresponds to a 3-dimensional vector of 16-bit unsigned integer values.
///
/// It is typedef for svtkm::Vec<svtkm::UInt32, 3>.
///
using Vec3ui_16 = svtkm::Vec<svtkm::UInt16, 3>;

/// \brief Vec3ui_32 corresponds to a 3-dimensional vector of 32-bit unsigned integer values.
///
/// It is typedef for svtkm::Vec<svtkm::UInt32, 3>.
///
using Vec3ui_32 = svtkm::Vec<svtkm::UInt32, 3>;

/// \brief Vec3ui_64 corresponds to a 3-dimensional vector of 64-bit unsigned integer values.
///
/// It is typedef for svtkm::Vec<svtkm::UInt64, 3>.
///
using Vec3ui_64 = svtkm::Vec<svtkm::UInt64, 3>;

template <typename T>
class SVTKM_ALWAYS_EXPORT Vec<T, 4> : public detail::VecBase<T, 4, Vec<T, 4>>
{
  using Superclass = detail::VecBase<T, 4, Vec<T, 4>>;

public:
  Vec() = default;
  SVTKM_EXEC_CONT Vec(const T& value)
    : Superclass(value)
  {
  }

  template <typename OtherType>
  SVTKM_EXEC_CONT Vec(const Vec<OtherType, 4>& src)
    : Superclass(src)
  {
  }

  SVTKM_EXEC_CONT
  constexpr Vec(const T& x, const T& y, const T& z, const T& w)
    : Superclass(x, y, z, w)
  {
  }
};

/// \brief Id4 corresponds to a 4-dimensional index.
///
using Id4 = svtkm::Vec<svtkm::Id, 4>;

/// \brief IdComponent4 corresponds to an index to a local (small) 4-d array or equivalent.
///
using IdComponent4 = svtkm::Vec<svtkm::IdComponent, 4>;

/// \brief Vec4f corresponds to a 4-dimensional vector of floating point values.
///
/// Each floating point value is of the default precision (i.e. svtkm::FloatDefault). It is
/// typedef for svtkm::Vec<svtkm::FloatDefault, 4>.
///
using Vec4f = svtkm::Vec<svtkm::FloatDefault, 4>;

/// \brief Vec4f_32 corresponds to a 4-dimensional vector of 32-bit floating point values.
///
/// It is typedef for svtkm::Vec<svtkm::Float32, 4>.
///
using Vec4f_32 = svtkm::Vec<svtkm::Float32, 4>;

/// \brief Vec4f_64 corresponds to a 4-dimensional vector of 64-bit floating point values.
///
/// It is typedef for svtkm::Vec<svtkm::Float64, 4>.
///
using Vec4f_64 = svtkm::Vec<svtkm::Float64, 4>;

/// \brief Vec4i corresponds to a 4-dimensional vector of integer values.
///
/// Each integer value is of the default precision (i.e. svtkm::Id).
///
using Vec4i = svtkm::Vec<svtkm::Id, 4>;

/// \brief Vec4i_8 corresponds to a 4-dimensional vector of 8-bit integer values.
///
/// It is typedef for svtkm::Vec<svtkm::Int32, 4>.
///
using Vec4i_8 = svtkm::Vec<svtkm::Int8, 4>;

/// \brief Vec4i_16 corresponds to a 4-dimensional vector of 16-bit integer values.
///
/// It is typedef for svtkm::Vec<svtkm::Int32, 4>.
///
using Vec4i_16 = svtkm::Vec<svtkm::Int16, 4>;

/// \brief Vec4i_32 corresponds to a 4-dimensional vector of 32-bit integer values.
///
/// It is typedef for svtkm::Vec<svtkm::Int32, 4>.
///
using Vec4i_32 = svtkm::Vec<svtkm::Int32, 4>;

/// \brief Vec4i_64 corresponds to a 4-dimensional vector of 64-bit integer values.
///
/// It is typedef for svtkm::Vec<svtkm::Int64, 4>.
///
using Vec4i_64 = svtkm::Vec<svtkm::Int64, 4>;

/// \brief Vec4ui corresponds to a 4-dimensional vector of unsigned integer values.
///
/// Each integer value is of the default precision (following svtkm::Id).
///
#ifdef SVTKM_USE_64BIT_IDS
using Vec4ui = svtkm::Vec<svtkm::UInt64, 4>;
#else
using Vec4ui = svtkm::Vec<svtkm::UInt32, 4>;
#endif

/// \brief Vec4ui_8 corresponds to a 4-dimensional vector of 8-bit unsigned integer values.
///
/// It is typedef for svtkm::Vec<svtkm::UInt32, 4>.
///
using Vec4ui_8 = svtkm::Vec<svtkm::UInt8, 4>;

/// \brief Vec4ui_16 corresponds to a 4-dimensional vector of 16-bit unsigned integer values.
///
/// It is typedef for svtkm::Vec<svtkm::UInt32, 4>.
///
using Vec4ui_16 = svtkm::Vec<svtkm::UInt16, 4>;

/// \brief Vec4ui_32 corresponds to a 4-dimensional vector of 32-bit unsigned integer values.
///
/// It is typedef for svtkm::Vec<svtkm::UInt32, 4>.
///
using Vec4ui_32 = svtkm::Vec<svtkm::UInt32, 4>;

/// \brief Vec4ui_64 corresponds to a 4-dimensional vector of 64-bit unsigned integer values.
///
/// It is typedef for svtkm::Vec<svtkm::UInt64, 4>.
///
using Vec4ui_64 = svtkm::Vec<svtkm::UInt64, 4>;

/// Initializes and returns a Vec containing all the arguments. The arguments should all be the
/// same type or compile issues will occur.
///
template <typename T, typename... Ts>
SVTKM_EXEC_CONT constexpr svtkm::Vec<T, svtkm::IdComponent(sizeof...(Ts) + 1)> make_Vec(T value0,
                                                                                     Ts&&... args)
{
  return svtkm::Vec<T, svtkm::IdComponent(sizeof...(Ts) + 1)>(value0, T(args)...);
}

/// \brief A Vec-like representation for short arrays.
///
/// The \c VecC class takes a short array of values and provides an interface
/// that mimics \c Vec. This provides a mechanism to treat C arrays like a \c
/// Vec. It is useful in situations where you want to use a \c Vec but the data
/// must come from elsewhere or in certain situations where the size cannot be
/// determined at compile time. In particular, \c Vec objects of different
/// sizes can potentially all be converted to a \c VecC of the same type.
///
/// Note that \c VecC holds a reference to an outside array given to it. If
/// that array gets destroyed (for example because the source goes out of
/// scope), the behavior becomes undefined.
///
/// You cannot use \c VecC with a const type in its template argument. For
/// example, you cannot declare <tt>VecC<const svtkm::Id></tt>. If you want a
/// non-mutable \c VecC, the \c VecCConst class (e.g.
/// <tt>VecCConst<svtkm::Id></tt>).
///
template <typename T>
class SVTKM_ALWAYS_EXPORT VecC : public detail::VecCBase<T, VecC<T>>
{
  using Superclass = detail::VecCBase<T, VecC<T>>;

  SVTKM_STATIC_ASSERT_MSG(std::is_const<T>::value == false,
                         "You cannot use VecC with a const type as its template argument. "
                         "Use either const VecC or VecCConst.");

public:
#ifdef SVTKM_DOXYGEN_ONLY
  using ComponentType = T;
#endif

  SVTKM_EXEC_CONT
  VecC()
    : Components(nullptr)
    , NumberOfComponents(0)
  {
  }

  SVTKM_EXEC_CONT
  VecC(T* array, svtkm::IdComponent size)
    : Components(array)
    , NumberOfComponents(size)
  {
  }

  template <svtkm::IdComponent Size>
  SVTKM_EXEC_CONT VecC(svtkm::Vec<T, Size>& src)
    : Components(src.GetPointer())
    , NumberOfComponents(Size)
  {
  }

  SVTKM_EXEC_CONT
  explicit VecC(T& src)
    : Components(&src)
    , NumberOfComponents(1)
  {
  }

  SVTKM_EXEC_CONT
  VecC(const VecC<T>& src)
    : Components(src.Components)
    , NumberOfComponents(src.NumberOfComponents)
  {
  }

  inline SVTKM_EXEC_CONT const T& operator[](svtkm::IdComponent index) const
  {
    SVTKM_ASSERT(index >= 0);
    SVTKM_ASSERT(index < this->NumberOfComponents);
    return this->Components[index];
  }

  inline SVTKM_EXEC_CONT T& operator[](svtkm::IdComponent index)
  {
    SVTKM_ASSERT(index >= 0);
    SVTKM_ASSERT(index < this->NumberOfComponents);
    return this->Components[index];
  }

  inline SVTKM_EXEC_CONT svtkm::IdComponent GetNumberOfComponents() const
  {
    return this->NumberOfComponents;
  }

  SVTKM_EXEC_CONT
  VecC<T>& operator=(const VecC<T>& src)
  {
    SVTKM_ASSERT(this->NumberOfComponents == src.GetNumberOfComponents());
    for (svtkm::IdComponent index = 0; index < this->NumberOfComponents; index++)
    {
      (*this)[index] = src[index];
    }

    return *this;
  }

private:
  T* const Components;
  svtkm::IdComponent NumberOfComponents;
};

/// \brief A const version of VecC
///
/// \c VecCConst is a non-mutable form of \c VecC. It can be used in place of
/// \c VecC when a constant array is available.
///
/// A \c VecC can be automatically converted to a \c VecCConst, but not vice
/// versa, so function arguments should use \c VecCConst when the data do not
/// need to be changed.
///
template <typename T>
class SVTKM_ALWAYS_EXPORT VecCConst : public detail::VecCBase<T, VecCConst<T>>
{
  using Superclass = detail::VecCBase<T, VecCConst<T>>;

  SVTKM_STATIC_ASSERT_MSG(std::is_const<T>::value == false,
                         "You cannot use VecCConst with a const type as its template argument. "
                         "Remove the const from the type.");

public:
#ifdef SVTKM_DOXYGEN_ONLY
  using ComponentType = T;
#endif

  SVTKM_EXEC_CONT
  VecCConst()
    : Components(nullptr)
    , NumberOfComponents(0)
  {
  }

  SVTKM_EXEC_CONT
  VecCConst(const T* array, svtkm::IdComponent size)
    : Components(array)
    , NumberOfComponents(size)
  {
  }

  template <svtkm::IdComponent Size>
  SVTKM_EXEC_CONT VecCConst(const svtkm::Vec<T, Size>& src)
    : Components(src.GetPointer())
    , NumberOfComponents(Size)
  {
  }

  SVTKM_EXEC_CONT
  explicit VecCConst(const T& src)
    : Components(&src)
    , NumberOfComponents(1)
  {
  }

  SVTKM_EXEC_CONT
  VecCConst(const VecCConst<T>& src)
    : Components(src.Components)
    , NumberOfComponents(src.NumberOfComponents)
  {
  }

  SVTKM_EXEC_CONT
  VecCConst(const VecC<T>& src)
    : Components(src.Components)
    , NumberOfComponents(src.NumberOfComponents)
  {
  }

  inline SVTKM_EXEC_CONT const T& operator[](svtkm::IdComponent index) const
  {
    SVTKM_ASSERT(index >= 0);
    SVTKM_ASSERT(index < this->NumberOfComponents);
    return this->Components[index];
  }

  inline SVTKM_EXEC_CONT svtkm::IdComponent GetNumberOfComponents() const
  {
    return this->NumberOfComponents;
  }

private:
  const T* const Components;
  svtkm::IdComponent NumberOfComponents;

  // You are not allowed to assign to a VecCConst, so these operators are not
  // implemented and are disallowed.
  void operator=(const VecCConst<T>&) = delete;
  void operator+=(const VecCConst<T>&) = delete;
  void operator-=(const VecCConst<T>&) = delete;
  void operator*=(const VecCConst<T>&) = delete;
  void operator/=(const VecCConst<T>&) = delete;
};

/// Creates a \c VecC from an input array.
///
template <typename T>
static inline SVTKM_EXEC_CONT svtkm::VecC<T> make_VecC(T* array, svtkm::IdComponent size)
{
  return svtkm::VecC<T>(array, size);
}

/// Creates a \c VecCConst from a constant input array.
///
template <typename T>
static inline SVTKM_EXEC_CONT svtkm::VecCConst<T> make_VecC(const T* array, svtkm::IdComponent size)
{
  return svtkm::VecCConst<T>(array, size);
}

namespace detail
{
template <typename T>
struct DotType
{
  //results when < 32bit can be float if somehow we are using float16/float8, otherwise is
  // int32 or uint32 depending on if it signed or not.
  using float_type = svtkm::Float32;
  using integer_type =
    typename std::conditional<std::is_signed<T>::value, svtkm::Int32, svtkm::UInt32>::type;
  using promote_type =
    typename std::conditional<std::is_integral<T>::value, integer_type, float_type>::type;
  using type =
    typename std::conditional<(sizeof(T) < sizeof(svtkm::Float32)), promote_type, T>::type;
};

template <typename T>
static inline SVTKM_EXEC_CONT typename DotType<typename T::ComponentType>::type vec_dot(const T& a,
                                                                                       const T& b)
{
  using U = typename DotType<typename T::ComponentType>::type;
  U result = a[0] * b[0];
  for (svtkm::IdComponent i = 1; i < a.GetNumberOfComponents(); ++i)
  {
    result = result + a[i] * b[i];
  }
  return result;
}
template <typename T, svtkm::IdComponent Size>
static inline SVTKM_EXEC_CONT typename DotType<T>::type vec_dot(const svtkm::Vec<T, Size>& a,
                                                               const svtkm::Vec<T, Size>& b)
{
  using U = typename DotType<T>::type;
  U result = a[0] * b[0];
  for (svtkm::IdComponent i = 1; i < Size; ++i)
  {
    result = result + a[i] * b[i];
  }
  return result;
}
}

template <typename T>
static inline SVTKM_EXEC_CONT auto Dot(const T& a, const T& b) -> decltype(detail::vec_dot(a, b))
{
  return detail::vec_dot(a, b);
}
template <typename T>
static inline SVTKM_EXEC_CONT typename detail::DotType<T>::type Dot(const svtkm::Vec<T, 2>& a,
                                                                   const svtkm::Vec<T, 2>& b)
{
  return (a[0] * b[0]) + (a[1] * b[1]);
}
template <typename T>
static inline SVTKM_EXEC_CONT typename detail::DotType<T>::type Dot(const svtkm::Vec<T, 3>& a,
                                                                   const svtkm::Vec<T, 3>& b)
{
  return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
}
template <typename T>
static inline SVTKM_EXEC_CONT typename detail::DotType<T>::type Dot(const svtkm::Vec<T, 4>& a,
                                                                   const svtkm::Vec<T, 4>& b)
{
  return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]) + (a[3] * b[3]);
}
// Integer types of a width less than an integer get implicitly casted to
// an integer when doing a multiplication.
#define SVTK_M_SCALAR_DOT(stype)                                                                    \
  static inline SVTKM_EXEC_CONT detail::DotType<stype>::type dot(stype a, stype b)                  \
  {                                                                                                \
    return a * b;                                                                                  \
  } /* LEGACY */                                                                                   \
  static inline SVTKM_EXEC_CONT detail::DotType<stype>::type Dot(stype a, stype b) { return a * b; }
SVTK_M_SCALAR_DOT(svtkm::Int8)
SVTK_M_SCALAR_DOT(svtkm::UInt8)
SVTK_M_SCALAR_DOT(svtkm::Int16)
SVTK_M_SCALAR_DOT(svtkm::UInt16)
SVTK_M_SCALAR_DOT(svtkm::Int32)
SVTK_M_SCALAR_DOT(svtkm::UInt32)
SVTK_M_SCALAR_DOT(svtkm::Int64)
SVTK_M_SCALAR_DOT(svtkm::UInt64)
SVTK_M_SCALAR_DOT(svtkm::Float32)
SVTK_M_SCALAR_DOT(svtkm::Float64)

// v============ LEGACY =============v
template <typename T>
static inline SVTKM_EXEC_CONT auto dot(const T& a, const T& b) -> decltype(detail::vec_dot(a, b))
{
  return svtkm::Dot(a, b);
}
template <typename T>
static inline SVTKM_EXEC_CONT typename detail::DotType<T>::type dot(const svtkm::Vec<T, 2>& a,
                                                                   const svtkm::Vec<T, 2>& b)
{
  return svtkm::Dot(a, b);
}
template <typename T>
static inline SVTKM_EXEC_CONT typename detail::DotType<T>::type dot(const svtkm::Vec<T, 3>& a,
                                                                   const svtkm::Vec<T, 3>& b)
{
  return svtkm::Dot(a, b);
}
template <typename T>
static inline SVTKM_EXEC_CONT typename detail::DotType<T>::type dot(const svtkm::Vec<T, 4>& a,
                                                                   const svtkm::Vec<T, 4>& b)
{
  return svtkm::Dot(a, b);
}
// ^============ LEGACY =============^

template <typename T, svtkm::IdComponent Size>
inline SVTKM_EXEC_CONT T ReduceSum(const svtkm::Vec<T, Size>& a)
{
  T result = a[0];
  for (svtkm::IdComponent i = 1; i < Size; ++i)
  {
    result += a[i];
  }
  return result;
}

template <typename T>
inline SVTKM_EXEC_CONT T ReduceSum(const svtkm::Vec<T, 2>& a)
{
  return a[0] + a[1];
}

template <typename T>
inline SVTKM_EXEC_CONT T ReduceSum(const svtkm::Vec<T, 3>& a)
{
  return a[0] + a[1] + a[2];
}

template <typename T>
inline SVTKM_EXEC_CONT T ReduceSum(const svtkm::Vec<T, 4>& a)
{
  return a[0] + a[1] + a[2] + a[3];
}

template <typename T, svtkm::IdComponent Size>
inline SVTKM_EXEC_CONT T ReduceProduct(const svtkm::Vec<T, Size>& a)
{
  T result = a[0];
  for (svtkm::IdComponent i = 1; i < Size; ++i)
  {
    result *= a[i];
  }
  return result;
}

template <typename T>
inline SVTKM_EXEC_CONT T ReduceProduct(const svtkm::Vec<T, 2>& a)
{
  return a[0] * a[1];
}

template <typename T>
inline SVTKM_EXEC_CONT T ReduceProduct(const svtkm::Vec<T, 3>& a)
{
  return a[0] * a[1] * a[2];
}

template <typename T>
inline SVTKM_EXEC_CONT T ReduceProduct(const svtkm::Vec<T, 4>& a)
{
  return a[0] * a[1] * a[2] * a[3];
}

// A pre-declaration of svtkm::Pair so that classes templated on them can refer
// to it. The actual implementation is in svtkm/Pair.h.
template <typename U, typename V>
struct Pair;

template <typename T, svtkm::IdComponent Size>
inline SVTKM_EXEC_CONT svtkm::Vec<T, Size> operator*(T scalar, const svtkm::Vec<T, Size>& vec)
{
  return svtkm::internal::VecComponentWiseUnaryOperation<Size>()(
    vec, svtkm::internal::BindLeftBinaryOp<T, svtkm::Multiply>(scalar));
}

template <typename T, svtkm::IdComponent Size>
inline SVTKM_EXEC_CONT svtkm::Vec<T, Size> operator*(const svtkm::Vec<T, Size>& vec, T scalar)
{
  return svtkm::internal::VecComponentWiseUnaryOperation<Size>()(
    vec, svtkm::internal::BindRightBinaryOp<T, svtkm::Multiply>(scalar));
}

template <typename T, svtkm::IdComponent Size>
inline SVTKM_EXEC_CONT svtkm::Vec<T, Size> operator*(svtkm::Float64 scalar,
                                                   const svtkm::Vec<T, Size>& vec)
{
  return svtkm::Vec<T, Size>(svtkm::internal::VecComponentWiseUnaryOperation<Size>()(
    vec, svtkm::internal::BindLeftBinaryOp<svtkm::Float64, svtkm::Multiply, T>(scalar)));
}

template <typename T, svtkm::IdComponent Size>
inline SVTKM_EXEC_CONT svtkm::Vec<T, Size> operator*(const svtkm::Vec<T, Size>& vec,
                                                   svtkm::Float64 scalar)
{
  return svtkm::Vec<T, Size>(svtkm::internal::VecComponentWiseUnaryOperation<Size>()(
    vec, svtkm::internal::BindRightBinaryOp<svtkm::Float64, svtkm::Multiply, T>(scalar)));
}

template <svtkm::IdComponent Size>
inline SVTKM_EXEC_CONT svtkm::Vec<svtkm::Float64, Size> operator*(
  svtkm::Float64 scalar,
  const svtkm::Vec<svtkm::Float64, Size>& vec)
{
  return svtkm::internal::VecComponentWiseUnaryOperation<Size>()(
    vec, svtkm::internal::BindLeftBinaryOp<svtkm::Float64, svtkm::Multiply>(scalar));
}

template <svtkm::IdComponent Size>
inline SVTKM_EXEC_CONT svtkm::Vec<svtkm::Float64, Size> operator*(
  const svtkm::Vec<svtkm::Float64, Size>& vec,
  svtkm::Float64 scalar)
{
  return svtkm::internal::VecComponentWiseUnaryOperation<Size>()(
    vec, svtkm::internal::BindRightBinaryOp<svtkm::Float64, svtkm::Multiply>(scalar));
}

template <typename T, svtkm::IdComponent Size>
inline SVTKM_EXEC_CONT svtkm::Vec<T, Size> operator/(const svtkm::Vec<T, Size>& vec, T scalar)
{
  return svtkm::internal::VecComponentWiseUnaryOperation<Size>()(
    vec, svtkm::internal::BindRightBinaryOp<T, svtkm::Divide>(scalar));
}

template <typename T, svtkm::IdComponent Size>
inline SVTKM_EXEC_CONT svtkm::Vec<T, Size> operator/(const svtkm::Vec<T, Size>& vec,
                                                   svtkm::Float64 scalar)
{
  return svtkm::Vec<T, Size>(svtkm::internal::VecComponentWiseUnaryOperation<Size>()(
    vec, svtkm::internal::BindRightBinaryOp<svtkm::Float64, svtkm::Divide, T>(scalar)));
}

template <svtkm::IdComponent Size>
inline SVTKM_EXEC_CONT svtkm::Vec<svtkm::Float64, Size> operator/(
  const svtkm::Vec<svtkm::Float64, Size>& vec,
  svtkm::Float64 scalar)
{
  return svtkm::internal::VecComponentWiseUnaryOperation<Size>()(
    vec, svtkm::internal::BindRightBinaryOp<svtkm::Float64, svtkm::Divide>(scalar));
}

// clang-format off
// The enable_if for this operator is effectively disabling the negate
// operator for Vec of unsigned integers. Another approach would be
// to use enable_if<!is_unsigned>. That would be more inclusive but would
// also allow other types like Vec<Vec<unsigned> >. If necessary, we could
// change this implementation to be more inclusive.
template <typename T, svtkm::IdComponent Size>
inline SVTKM_EXEC_CONT
typename std::enable_if<(std::is_floating_point<T>::value || std::is_signed<T>::value),
                        svtkm::Vec<T, Size>>::type
operator-(const svtkm::Vec<T, Size>& x)
{
  return svtkm::internal::VecComponentWiseUnaryOperation<Size>()(x, svtkm::Negate());
}
// clang-format on

/// Helper function for printing out vectors during testing.
///
template <typename T, svtkm::IdComponent Size>
inline SVTKM_CONT std::ostream& operator<<(std::ostream& stream, const svtkm::Vec<T, Size>& vec)
{
  stream << "[";
  for (svtkm::IdComponent component = 0; component < Size - 1; component++)
  {
    stream << vec[component] << ",";
  }
  return stream << vec[Size - 1] << "]";
}

/// Helper function for printing out pairs during testing.
///
template <typename T, typename U>
inline SVTKM_EXEC_CONT std::ostream& operator<<(std::ostream& stream, const svtkm::Pair<T, U>& vec)
{
  return stream << "[" << vec.first << "," << vec.second << "]";
}


} // End of namespace svtkm
// Declared inside of svtkm namespace so that the operator work with ADL lookup
#endif //svtk_m_Types_h
