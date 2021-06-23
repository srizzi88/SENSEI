//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#ifndef svtkmlib_PortalTraits_h
#define svtkmlib_PortalTraits_h

#include "svtkmConfig.h" //required for general svtkm setup

#include <svtkm/Types.h>
#include <svtkm/internal/Assume.h>

#include <type_traits>

namespace tosvtkm
{

struct svtkPortalOfVecOfVecValues;
struct svtkPortalOfVecOfValues;
struct svtkPortalOfScalarValues;

template <typename T>
struct svtkPortalTraits
{
  using TagType = svtkPortalOfScalarValues;
  using ComponentType = typename std::remove_const<T>::type;
  using Type = ComponentType;
  static constexpr svtkm::IdComponent NUM_COMPONENTS = 1;

  static inline void SetComponent(Type& t, svtkm::IdComponent, const ComponentType& v) { t = v; }

  static inline ComponentType GetComponent(const Type& t, svtkm::IdComponent) { return t; }
};

template <typename T, int N>
struct svtkPortalTraits<svtkm::Vec<T, N> >
{
  using TagType = svtkPortalOfVecOfValues;
  using ComponentType = typename std::remove_const<T>::type;
  using Type = svtkm::Vec<T, N>;
  static constexpr svtkm::IdComponent NUM_COMPONENTS = N;

  static inline void SetComponent(Type& t, svtkm::IdComponent i, const ComponentType& v)
  {
    SVTKM_ASSUME((i >= 0 && i < N));
    t[i] = v;
  }

  static inline ComponentType GetComponent(const Type& t, svtkm::IdComponent i)
  {
    SVTKM_ASSUME((i >= 0 && i < N));
    return t[i];
  }
};

template <typename T, int N>
struct svtkPortalTraits<const svtkm::Vec<T, N> >
{
  using TagType = svtkPortalOfVecOfValues;
  using ComponentType = typename std::remove_const<T>::type;
  using Type = svtkm::Vec<T, N>;
  static constexpr svtkm::IdComponent NUM_COMPONENTS = N;

  static inline void SetComponent(Type& t, svtkm::IdComponent i, const ComponentType& v)
  {
    SVTKM_ASSUME((i >= 0 && i < N));
    t[i] = v;
  }

  static inline ComponentType GetComponent(const Type& t, svtkm::IdComponent i)
  {
    SVTKM_ASSUME((i >= 0 && i < N));
    return t[i];
  }
};

template <typename T, int N, int M>
struct svtkPortalTraits<svtkm::Vec<svtkm::Vec<T, N>, M> >
{
  using TagType = svtkPortalOfVecOfVecValues;
  using ComponentType = typename std::remove_const<T>::type;
  using Type = svtkm::Vec<svtkm::Vec<T, N>, M>;
  static constexpr svtkm::IdComponent NUM_COMPONENTS = N * M;

  static constexpr svtkm::IdComponent NUM_COMPONENTS_OUTER = M;
  static constexpr svtkm::IdComponent NUM_COMPONENTS_INNER = N;

  static inline void SetComponent(Type& t, svtkm::IdComponent i, const ComponentType& v)
  {
    SVTKM_ASSUME((i >= 0 && i < NUM_COMPONENTS));
    // We need to convert i back to a 2d index
    const svtkm::IdComponent j = i % N;
    t[i / N][j] = v;
  }

  static inline ComponentType GetComponent(const Type& t, svtkm::IdComponent i)
  {
    SVTKM_ASSUME((i >= 0 && i < NUM_COMPONENTS));
    // We need to convert i back to a 2d index
    const svtkm::IdComponent j = i % N;
    return t[i / N][j];
  }
};

template <typename T, int N, int M>
struct svtkPortalTraits<const svtkm::Vec<svtkm::Vec<T, N>, M> >
{
  using TagType = svtkPortalOfVecOfVecValues;
  using ComponentType = typename std::remove_const<T>::type;
  using Type = svtkm::Vec<svtkm::Vec<T, N>, M>;
  static constexpr svtkm::IdComponent NUM_COMPONENTS = N * M;

  static constexpr svtkm::IdComponent NUM_COMPONENTS_OUTER = M;
  static constexpr svtkm::IdComponent NUM_COMPONENTS_INNER = N;

  static inline void SetComponent(Type& t, svtkm::IdComponent i, const ComponentType& v)
  {
    SVTKM_ASSUME((i >= 0 && i < NUM_COMPONENTS));
    // We need to convert i back to a 2d index
    const svtkm::IdComponent j = i % N;
    t[i / N][j] = v;
  }

  static inline ComponentType GetComponent(const Type& t, svtkm::IdComponent i)
  {
    SVTKM_ASSUME((i >= 0 && i < NUM_COMPONENTS));
    // We need to convert i back to a 2d index
    const svtkm::IdComponent j = i % N;
    return t[i / N][j];
  }
};

} // namespace svtkmlib

#endif // svtkmlib_PortalsTraits_h
