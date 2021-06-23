/*=========================================================================

 Program:   Visualization Toolkit
 Module:    VTXHelper.txx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * VTXHelper.txx
 *
 *  Created on: May 3, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef SVTK_IO_ADIOS2_COMMON_VTXHelper_txx
#define SVTK_IO_ADIOS2_COMMON_VTXHelper_txx

#include "VTXHelper.h"

#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkIntArray.h"
#include "svtkLongArray.h"
#include "svtkLongLongArray.h"
#include "svtkUnsignedIntArray.h"
#include "svtkUnsignedLongArray.h"
#include "svtkUnsignedLongLongArray.h"

namespace vtx
{
namespace helper
{

// TODO: extend other types
template <>
svtkSmartPointer<svtkDataArray> NewDataArray<int>()
{
  return svtkSmartPointer<svtkIntArray>::New();
}

template <>
svtkSmartPointer<svtkDataArray> NewDataArray<unsigned int>()
{
  return svtkSmartPointer<svtkUnsignedIntArray>::New();
}

template <>
svtkSmartPointer<svtkDataArray> NewDataArray<long int>()
{
  return svtkSmartPointer<svtkLongArray>::New();
}

template <>
svtkSmartPointer<svtkDataArray> NewDataArray<unsigned long int>()
{
  return svtkSmartPointer<svtkUnsignedLongArray>::New();
}

template <>
svtkSmartPointer<svtkDataArray> NewDataArray<long long int>()
{
  return svtkSmartPointer<svtkLongLongArray>::New();
}

template <>
svtkSmartPointer<svtkDataArray> NewDataArray<unsigned long long int>()
{
  return svtkSmartPointer<svtkUnsignedLongLongArray>::New();
}

template <>
svtkSmartPointer<svtkDataArray> NewDataArray<float>()
{
  return svtkSmartPointer<svtkFloatArray>::New();
}

template <>
svtkSmartPointer<svtkDataArray> NewDataArray<double>()
{
  return svtkSmartPointer<svtkDoubleArray>::New();
}

} // end namespace helper
} // end namespace vtx

#endif /* SVTK_IO_ADIOS2_VTX_COMMON_VTXHelper_txx */
