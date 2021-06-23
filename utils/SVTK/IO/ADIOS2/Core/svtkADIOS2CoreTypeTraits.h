/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkADIOS2CoreImageReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @brief  Type traits for adios2 types(Native types) to svtk types
 *
 */
#ifndef svtkADIOS2CoreTypeTraits_h
#define svtkADIOS2CoreTypeTraits_h

#include "svtkType.h"

#include "svtkIOADIOS2Module.h" // For export macro

template <typename T>
struct NativeToSVTKType
{
  static constexpr int SVTKType = 0;
};

template <>
struct NativeToSVTKType<char>
{
  static constexpr int SVTKType = SVTK_CHAR;
};

template <>
struct NativeToSVTKType<float>
{
  static constexpr int SVTKType = SVTK_FLOAT;
};

template <>
struct NativeToSVTKType<double>
{
  static constexpr int SVTKType = SVTK_DOUBLE;
};

template <>
struct NativeToSVTKType<int8_t>
{
  static constexpr int SVTKType = SVTK_TYPE_INT8;
};

template <>
struct NativeToSVTKType<uint8_t>
{
  static constexpr int SVTKType = SVTK_TYPE_UINT8;
};

template <>
struct NativeToSVTKType<int16_t>
{
  static constexpr int SVTKType = SVTK_TYPE_INT16;
};

template <>
struct NativeToSVTKType<uint16_t>
{
  static constexpr int SVTKType = SVTK_TYPE_UINT16;
};

template <>
struct NativeToSVTKType<int32_t>
{
  static constexpr int SVTKType = SVTK_TYPE_INT32;
};

template <>
struct NativeToSVTKType<uint32_t>
{
  static constexpr int SVTKType = SVTK_TYPE_UINT32;
};

template <>
struct NativeToSVTKType<int64_t>
{
  static constexpr int SVTKType = SVTK_TYPE_INT64;
};

template <>
struct NativeToSVTKType<uint64_t>
{
  static constexpr int SVTKType = SVTK_TYPE_UINT64;
};

#endif
// SVTK-HeaderTest-Exclude: svtkADIOS2CoreTypeTraits.h
