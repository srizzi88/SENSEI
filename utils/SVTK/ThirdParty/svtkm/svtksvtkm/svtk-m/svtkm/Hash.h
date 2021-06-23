//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_Hash_h
#define svtk_m_Hash_h

#include <svtkm/TypeTraits.h>
#include <svtkm/Types.h>
#include <svtkm/VecTraits.h>

namespace svtkm
{

using HashType = svtkm::UInt32;

namespace detail
{

static constexpr svtkm::HashType FNV1A_OFFSET = 2166136261;
static constexpr svtkm::HashType FNV1A_PRIME = 16777619;

/// \brief Performs an FNV-1a hash on 32-bit integers returning a 32-bit hash
///
template <typename InVecType>
SVTKM_EXEC_CONT inline svtkm::HashType HashFNV1a32(const InVecType& inVec)
{
  using Traits = svtkm::VecTraits<InVecType>;
  const svtkm::IdComponent numComponents = Traits::GetNumberOfComponents(inVec);

  svtkm::HashType hash = FNV1A_OFFSET;
  for (svtkm::IdComponent index = 0; index < numComponents; index++)
  {
    svtkm::HashType dataBits = static_cast<svtkm::HashType>(Traits::GetComponent(inVec, index));
    hash = (hash * FNV1A_PRIME) ^ dataBits;
  }

  return hash;
}

/// \brief Performs an FNV-1a hash on 64-bit integers returning a 32-bit hash
///
template <typename InVecType>
SVTKM_EXEC_CONT inline svtkm::HashType HashFNV1a64(const InVecType& inVec)
{
  using Traits = svtkm::VecTraits<InVecType>;
  const svtkm::IdComponent numComponents = Traits::GetNumberOfComponents(inVec);

  svtkm::HashType hash = FNV1A_OFFSET;
  for (svtkm::IdComponent index = 0; index < numComponents; index++)
  {
    svtkm::UInt64 allDataBits = static_cast<svtkm::UInt64>(Traits::GetComponent(inVec, index));
    svtkm::HashType upperDataBits =
      static_cast<svtkm::HashType>((allDataBits & 0xFFFFFFFF00000000L) >> 32);
    hash = (hash * FNV1A_PRIME) ^ upperDataBits;
    svtkm::HashType lowerDataBits = static_cast<svtkm::HashType>(allDataBits & 0x00000000FFFFFFFFL);
    hash = (hash * FNV1A_PRIME) ^ lowerDataBits;
  }

  return hash;
}

// If you get a compile error saying that there is no implementation of the class HashChooser,
// then you have tried to make a hash from an invalid type (like a float).
template <typename NumericTag, std::size_t DataSize>
struct HashChooser;

template <>
struct HashChooser<svtkm::TypeTraitsIntegerTag, 4>
{
  template <typename InVecType>
  SVTKM_EXEC_CONT static svtkm::HashType Hash(const InVecType& inVec)
  {
    return svtkm::detail::HashFNV1a32(inVec);
  }
};

template <>
struct HashChooser<svtkm::TypeTraitsIntegerTag, 8>
{
  template <typename InVecType>
  SVTKM_EXEC_CONT static svtkm::HashType Hash(const InVecType& inVec)
  {
    return svtkm::detail::HashFNV1a64(inVec);
  }
};

} // namespace detail

/// \brief Returns a 32-bit hash on a group of integer-type values.
///
/// The input to the hash is expected to be a svtkm::Vec or a Vec-like object. The values can be
/// either 32-bit integers or 64-bit integers (signed or unsigned). Regardless, the resulting hash
/// is an unsigned 32-bit integer.
///
/// The hash is designed to minimize the probability of collisions, but collisions are always
/// possible. Thus, when using these hashes there should be a contingency for dealing with
/// collisions.
///
template <typename InVecType>
SVTKM_EXEC_CONT inline svtkm::HashType Hash(const InVecType& inVec)
{
  using VecTraits = svtkm::VecTraits<InVecType>;
  using ComponentType = typename VecTraits::ComponentType;
  using ComponentTraits = svtkm::TypeTraits<ComponentType>;
  using Chooser = detail::HashChooser<typename ComponentTraits::NumericTag, sizeof(ComponentType)>;
  return Chooser::Hash(inVec);
}

} // namespace svtkm

#endif //svtk_m_Hash_h
