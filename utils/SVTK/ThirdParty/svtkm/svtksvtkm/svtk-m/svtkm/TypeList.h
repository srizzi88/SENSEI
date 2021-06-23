//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_TypeList_h
#define svtk_m_TypeList_h

#ifndef SVTKM_DEFAULT_TYPE_LIST
#define SVTKM_DEFAULT_TYPE_LIST ::svtkm::TypeListCommon
#endif

#include <svtkm/List.h>
#include <svtkm/Types.h>

namespace svtkm
{

/// A list containing the type svtkm::Id.
///
using TypeListId = svtkm::List<svtkm::Id>;

/// A list containing the type svtkm::Id2.
///
using TypeListId2 = svtkm::List<svtkm::Id2>;

/// A list containing the type svtkm::Id3.
///
using TypeListId3 = svtkm::List<svtkm::Id3>;

/// A list containing the type svtkm::Id4.
///
using TypeListId4 = svtkm::List<svtkm::Id4>;

/// A list containing the type svtkm::IdComponent
///
using TypeListIdComponent = svtkm::List<svtkm::IdComponent>;

/// A list containing types used to index arrays. Contains svtkm::Id, svtkm::Id2,
/// and svtkm::Id3.
///
using TypeListIndex = svtkm::List<svtkm::Id, svtkm::Id2, svtkm::Id3>;

/// A list containing types used for scalar fields. Specifically, contains
/// floating point numbers of different widths (i.e. svtkm::Float32 and
/// svtkm::Float64).
using TypeListFieldScalar = svtkm::List<svtkm::Float32, svtkm::Float64>;

/// A list containing types for values for fields with two dimensional
/// vectors.
///
using TypeListFieldVec2 = svtkm::List<svtkm::Vec2f_32, svtkm::Vec2f_64>;

/// A list containing types for values for fields with three dimensional
/// vectors.
///
using TypeListFieldVec3 = svtkm::List<svtkm::Vec3f_32, svtkm::Vec3f_64>;

/// A list containing types for values for fields with four dimensional
/// vectors.
///
using TypeListFieldVec4 = svtkm::List<svtkm::Vec4f_32, svtkm::Vec4f_64>;

/// A list containing common types for floating-point vectors. Specifically contains
/// floating point vectors of size 2, 3, and 4 with floating point components.
/// Scalars are not included.
///
using TypeListFloatVec = svtkm::List<svtkm::Vec2f_32,
                                    svtkm::Vec2f_64,
                                    svtkm::Vec3f_32,
                                    svtkm::Vec3f_64,
                                    svtkm::Vec4f_32,
                                    svtkm::Vec4f_64>;

/// A list containing common types for values in fields. Specifically contains
/// floating point scalars and vectors of size 2, 3, and 4 with floating point
/// components.
///
using TypeListField = svtkm::List<svtkm::Float32,
                                 svtkm::Float64,
                                 svtkm::Vec2f_32,
                                 svtkm::Vec2f_64,
                                 svtkm::Vec3f_32,
                                 svtkm::Vec3f_64,
                                 svtkm::Vec4f_32,
                                 svtkm::Vec4f_64>;

/// A list of all scalars defined in svtkm/Types.h. A scalar is a type that
/// holds a single number.
///
using TypeListScalarAll = svtkm::List<svtkm::Int8,
                                     svtkm::UInt8,
                                     svtkm::Int16,
                                     svtkm::UInt16,
                                     svtkm::Int32,
                                     svtkm::UInt32,
                                     svtkm::Int64,
                                     svtkm::UInt64,
                                     svtkm::Float32,
                                     svtkm::Float64>;

/// A list of the most commonly use Vec classes. Specifically, these are
/// vectors of size 2, 3, or 4 containing either unsigned bytes, signed
/// integers of 32 or 64 bits, or floating point values of 32 or 64 bits.
///
using TypeListVecCommon = svtkm::List<svtkm::Vec2ui_8,
                                     svtkm::Vec2i_32,
                                     svtkm::Vec2i_64,
                                     svtkm::Vec2f_32,
                                     svtkm::Vec2f_64,
                                     svtkm::Vec3ui_8,
                                     svtkm::Vec3i_32,
                                     svtkm::Vec3i_64,
                                     svtkm::Vec3f_32,
                                     svtkm::Vec3f_64,
                                     svtkm::Vec4ui_8,
                                     svtkm::Vec4i_32,
                                     svtkm::Vec4i_64,
                                     svtkm::Vec4f_32,
                                     svtkm::Vec4f_64>;

namespace internal
{

/// A list of uncommon Vec classes with length up to 4. This is not much
/// use in general, but is used when joined with \c TypeListVecCommon
/// to get a list of all vectors up to size 4.
///
using TypeListVecUncommon = svtkm::List<svtkm::Vec2i_8,
                                       svtkm::Vec2i_16,
                                       svtkm::Vec2ui_16,
                                       svtkm::Vec2ui_32,
                                       svtkm::Vec2ui_64,
                                       svtkm::Vec3i_8,
                                       svtkm::Vec3i_16,
                                       svtkm::Vec3ui_16,
                                       svtkm::Vec3ui_32,
                                       svtkm::Vec3ui_64,
                                       svtkm::Vec4i_8,
                                       svtkm::Vec4i_16,
                                       svtkm::Vec4ui_16,
                                       svtkm::Vec4ui_32,
                                       svtkm::Vec4ui_64>;

} // namespace internal

/// A list of all vector classes with standard types as components and
/// lengths between 2 and 4.
///
using TypeListVecAll =
  svtkm::ListAppend<svtkm::TypeListVecCommon, svtkm::internal::TypeListVecUncommon>;

/// A list of all basic types listed in svtkm/Types.h. Does not include all
/// possible SVTK-m types like arbitrarily typed and sized Vecs (only up to
/// length 4) or math types like matrices.
///
using TypeListAll = svtkm::ListAppend<svtkm::TypeListScalarAll, svtkm::TypeListVecAll>;

/// A list of the most commonly used types across multiple domains. Includes
/// integers, floating points, and 3 dimensional vectors of floating points.
///
using TypeListCommon = svtkm::List<svtkm::UInt8,
                                  svtkm::Int32,
                                  svtkm::Int64,
                                  svtkm::Float32,
                                  svtkm::Float64,
                                  svtkm::Vec3f_32,
                                  svtkm::Vec3f_64>;

} // namespace svtkm

#endif //svtk_m_TypeList_h
