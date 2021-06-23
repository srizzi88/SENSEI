//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtkm_testing_VecTraitsTest_h
#define svtkm_testing_VecTraitsTest_h

//GCC 4+ when running the test code have false positive warnings
//about uninitialized svtkm::VecC<> when filled by VecTraits<T>::CopyInto.
//The testing code already verifies that CopyInto works by verifying the
//results, so we are going to suppress `-Wmaybe-uninitialized` for this
//file
//This block has to go before we include any svtkm file that brings in
//<svtkm/Types.h> otherwise the warning suppression will not work
#include <svtkm/internal/Configure.h>
#if (defined(SVTKM_GCC) && __GNUC__ >= 4)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif // gcc  4+

#include <svtkm/VecTraits.h>

#include <svtkm/StaticAssert.h>
#include <svtkm/TypeTraits.h>

#include <svtkm/testing/Testing.h>

namespace svtkm
{
namespace testing
{

namespace detail
{

inline void CompareDimensionalityTags(svtkm::TypeTraitsScalarTag, svtkm::VecTraitsTagSingleComponent)
{
  // If we are here, everything is fine.
}
inline void CompareDimensionalityTags(svtkm::TypeTraitsVectorTag,
                                      svtkm::VecTraitsTagMultipleComponents)
{
  // If we are here, everything is fine.
}

template <svtkm::IdComponent NUM_COMPONENTS, typename T>
inline void CheckIsStatic(const T&, svtkm::VecTraitsTagSizeStatic)
{
  SVTKM_TEST_ASSERT(svtkm::VecTraits<T>::NUM_COMPONENTS == NUM_COMPONENTS,
                   "Traits returns unexpected number of components");
}

template <svtkm::IdComponent NUM_COMPONENTS, typename T>
inline void CheckIsStatic(const T&, svtkm::VecTraitsTagSizeVariable)
{
  // If we are here, everything is fine.
}

template <typename VecType>
struct VecIsWritable
{
  using type = std::true_type;
};

template <typename ComponentType>
struct VecIsWritable<svtkm::VecCConst<ComponentType>>
{
  using type = std::false_type;
};

// Part of TestVecTypeImpl that writes to the Vec type
template <svtkm::IdComponent NUM_COMPONENTS, typename T, typename VecCopyType>
static void TestVecTypeWritableImpl(const T& inVector,
                                    const VecCopyType& vectorCopy,
                                    T& outVector,
                                    std::true_type)
{
  using Traits = svtkm::VecTraits<T>;
  using ComponentType = typename Traits::ComponentType;

  {
    const ComponentType multiplier = 4;
    for (svtkm::IdComponent i = 0; i < NUM_COMPONENTS; i++)
    {
      Traits::SetComponent(
        outVector, i, ComponentType(multiplier * Traits::GetComponent(inVector, i)));
    }
    svtkm::Vec<ComponentType, NUM_COMPONENTS> resultCopy;
    Traits::CopyInto(outVector, resultCopy);
    SVTKM_TEST_ASSERT(test_equal(resultCopy, multiplier * vectorCopy),
                     "Got bad result for scalar multiple");
  }

  {
    const ComponentType multiplier = 7;
    for (svtkm::IdComponent i = 0; i < NUM_COMPONENTS; i++)
    {
      Traits::GetComponent(outVector, i) =
        ComponentType(multiplier * Traits::GetComponent(inVector, i));
    }
    svtkm::Vec<ComponentType, NUM_COMPONENTS> resultCopy;
    Traits::CopyInto(outVector, resultCopy);
    SVTKM_TEST_ASSERT(test_equal(resultCopy, multiplier * vectorCopy),
                     "Got bad result for scalar multiple");
  }
}

template <svtkm::IdComponent NUM_COMPONENTS, typename T, typename VecCopyType>
static void TestVecTypeWritableImpl(const T& svtkmNotUsed(inVector),
                                    const VecCopyType& svtkmNotUsed(vectorCopy),
                                    T& svtkmNotUsed(outVector),
                                    std::false_type)
{
  // Skip writable functionality.
}

/// Compares some manual arithmetic through type traits to arithmetic with
/// the Tuple class.
template <svtkm::IdComponent NUM_COMPONENTS, typename T>
static void TestVecTypeImpl(const typename std::remove_const<T>::type& inVector,
                            typename std::remove_const<T>::type& outVector)
{
  using Traits = svtkm::VecTraits<T>;
  using ComponentType = typename Traits::ComponentType;
  using NonConstT = typename std::remove_const<T>::type;

  CheckIsStatic<NUM_COMPONENTS>(inVector, typename Traits::IsSizeStatic());

  SVTKM_TEST_ASSERT(Traits::GetNumberOfComponents(inVector) == NUM_COMPONENTS,
                   "Traits returned wrong number of components.");

  svtkm::Vec<ComponentType, NUM_COMPONENTS> vectorCopy;
  Traits::CopyInto(inVector, vectorCopy);
  SVTKM_TEST_ASSERT(test_equal(vectorCopy, inVector), "CopyInto does not work.");

  {
    auto expected = svtkm::Dot(vectorCopy, vectorCopy);
    decltype(expected) result = 0;
    for (svtkm::IdComponent i = 0; i < NUM_COMPONENTS; i++)
    {
      ComponentType component = Traits::GetComponent(inVector, i);
      result = result + (component * component);
    }
    SVTKM_TEST_ASSERT(test_equal(result, expected), "Got bad result for dot product");
  }

  // This will fail to compile if the tags are wrong.
  detail::CompareDimensionalityTags(typename svtkm::TypeTraits<T>::DimensionalityTag(),
                                    typename svtkm::VecTraits<T>::HasMultipleComponents());

  TestVecTypeWritableImpl<NUM_COMPONENTS, NonConstT>(
    inVector, vectorCopy, outVector, typename VecIsWritable<NonConstT>::type());

  // Compiler checks for base component types
  using BaseComponentType = typename svtkm::VecTraits<T>::BaseComponentType;
  SVTKM_STATIC_ASSERT((std::is_same<typename svtkm::TypeTraits<BaseComponentType>::DimensionalityTag,
                                   svtkm::TypeTraitsScalarTag>::value));
  SVTKM_STATIC_ASSERT((std::is_same<typename svtkm::VecTraits<ComponentType>::BaseComponentType,
                                   BaseComponentType>::value));

  // Compiler checks for replacing component types
  using ReplaceWithVecComponent =
    typename svtkm::VecTraits<T>::template ReplaceComponentType<svtkm::Vec<char, 2>>;
  SVTKM_STATIC_ASSERT(
    (std::is_same<typename svtkm::TypeTraits<T>::DimensionalityTag,
                  svtkm::TypeTraitsVectorTag>::value &&
     std::is_same<typename svtkm::VecTraits<ReplaceWithVecComponent>::ComponentType,
                  svtkm::Vec<char, 2>>::value) ||
    (std::is_same<typename svtkm::TypeTraits<T>::DimensionalityTag,
                  svtkm::TypeTraitsScalarTag>::value &&
     std::is_same<typename svtkm::VecTraits<ReplaceWithVecComponent>::ComponentType, char>::value));
  SVTKM_STATIC_ASSERT(
    (std::is_same<typename svtkm::VecTraits<ReplaceWithVecComponent>::BaseComponentType,
                  char>::value));
  using ReplaceBaseComponent =
    typename svtkm::VecTraits<ReplaceWithVecComponent>::template ReplaceBaseComponentType<short>;
  SVTKM_STATIC_ASSERT(
    (std::is_same<typename svtkm::TypeTraits<T>::DimensionalityTag,
                  svtkm::TypeTraitsVectorTag>::value &&
     std::is_same<typename svtkm::VecTraits<ReplaceBaseComponent>::ComponentType,
                  svtkm::Vec<short, 2>>::value) ||
    (std::is_same<typename svtkm::TypeTraits<T>::DimensionalityTag,
                  svtkm::TypeTraitsScalarTag>::value &&
     std::is_same<typename svtkm::VecTraits<ReplaceBaseComponent>::ComponentType, short>::value));
  SVTKM_STATIC_ASSERT((
    std::is_same<typename svtkm::VecTraits<ReplaceBaseComponent>::BaseComponentType, short>::value));
}

inline void CheckVecComponentsTag(svtkm::VecTraitsTagMultipleComponents)
{
  // If we are running here, everything is fine.
}

} // namespace detail

/// Checks to make sure that the HasMultipleComponents tag is actually for
/// multiple components. Should only be called for vector classes that actually
/// have multiple components.
///
template <class T>
inline void TestVecComponentsTag()
{
  // This will fail to compile if the tag is wrong
  // (i.e. not svtkm::VecTraitsTagMultipleComponents)
  detail::CheckVecComponentsTag(typename svtkm::VecTraits<T>::HasMultipleComponents());
}

namespace detail
{

inline void CheckScalarComponentsTag(svtkm::VecTraitsTagSingleComponent)
{
  // If we are running here, everything is fine.
}

} // namespace detail

/// Compares some manual arithmetic through type traits to arithmetic with
/// the Tuple class.
template <svtkm::IdComponent NUM_COMPONENTS, typename T>
static void TestVecType(const T& inVector, T& outVector)
{
  detail::TestVecTypeImpl<NUM_COMPONENTS, T>(inVector, outVector);
  detail::TestVecTypeImpl<NUM_COMPONENTS, const T>(inVector, outVector);
}

/// Checks to make sure that the HasMultipleComponents tag is actually for a
/// single component. Should only be called for "vector" classes that actually
/// have only a single component (that is, are really scalars).
///
template <class T>
inline void TestScalarComponentsTag()
{
  // This will fail to compile if the tag is wrong
  // (i.e. not svtkm::VecTraitsTagSingleComponent)
  detail::CheckScalarComponentsTag(typename svtkm::VecTraits<T>::HasMultipleComponents());
}
}
} // namespace svtkm::testing

#if (defined(SVTKM_GCC) && __GNUC__ > 4 && __GNUC__ < 7)
#pragma GCC diagnostic pop
#endif // gcc  5 or 6

#endif //svtkm_testing_VecTraitsTest_h
