//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_interop_internal_BufferTypePicker_h
#define svtk_m_interop_internal_BufferTypePicker_h

#include <svtkm/TypeTraits.h>
#include <svtkm/Types.h>
#include <svtkm/interop/internal/OpenGLHeaders.h>

namespace svtkm
{
namespace interop
{
namespace internal
{

namespace detail
{

template <typename NumericTag, typename DimensionalityTag>
static inline SVTKM_CONT GLenum BufferTypePickerImpl(NumericTag, DimensionalityTag)
{
  return GL_ARRAY_BUFFER;
}

SVTKM_CONT
static inline GLenum BufferTypePickerImpl(svtkm::TypeTraitsIntegerTag, svtkm::TypeTraitsScalarTag)
{
  return GL_ELEMENT_ARRAY_BUFFER;
}

} //namespace detail

static inline SVTKM_CONT GLenum BufferTypePicker(svtkm::Int32)
{
  return GL_ELEMENT_ARRAY_BUFFER;
}

static inline SVTKM_CONT GLenum BufferTypePicker(svtkm::UInt32)
{
  return GL_ELEMENT_ARRAY_BUFFER;
}

static inline SVTKM_CONT GLenum BufferTypePicker(svtkm::Int64)
{
  return GL_ELEMENT_ARRAY_BUFFER;
}

static inline SVTKM_CONT GLenum BufferTypePicker(svtkm::UInt64)
{
  return GL_ELEMENT_ARRAY_BUFFER;
}

/// helper function that guesses what OpenGL buffer type is the best default
/// given a primitive type. Currently GL_ELEMENT_ARRAY_BUFFER is used for
/// integer types, and GL_ARRAY_BUFFER is used for everything else
///
template <typename T>
static inline SVTKM_CONT GLenum BufferTypePicker(T)
{
  using Traits = svtkm::TypeTraits<T>;
  return detail::BufferTypePickerImpl(typename Traits::NumericTag(),
                                      typename Traits::DimensionalityTag());
}
}
}
} //namespace svtkm::interop::internal

#endif //svtk_m_interop_internal_BufferTypePicker_h
