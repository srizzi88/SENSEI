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

#ifndef svtkmFilterPolicy_h
#define svtkmFilterPolicy_h
#ifndef __SVTK_WRAP__
#ifndef SVTK_WRAPPING_CXX

#include "svtkmConfig.h" //required for general svtkm setup

#include <svtkm/List.h>
#include <svtkm/cont/ArrayHandleCast.h>
#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/CellSetPermutation.h>
#include <svtkm/cont/CellSetSingleType.h>
#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/filter/PolicyDefault.h>

namespace tosvtkm
{

//------------------------------------------------------------------------------
// All scalar types in svtkType.h
using SVTKScalarTypes = svtkm::List< //
  char,                            //
  signed char,                     //
  unsigned char,                   //
  short,                           //
  unsigned short,                  //
  int,                             //
  unsigned int,                    //
  long,                            //
  unsigned long,                   //
  long long,                       //
  unsigned long long,              //
  float,                           //
  double                           //
  >;

using SpecialGradientOutTypes =
  svtkm::List<svtkm::Vec<svtkm::Vec<svtkm::Float32, 3>, 3>, svtkm::Vec<svtkm::Vec<svtkm::Float64, 3>, 3> >;

using FieldTypeInSVTK = svtkm::ListAppend<svtkm::TypeListVecCommon, SVTKScalarTypes>;

using FieldTypeOutSVTK =
  svtkm::ListAppend<svtkm::TypeListVecCommon, SpecialGradientOutTypes, SVTKScalarTypes>;

//------------------------------------------------------------------------------
using CellListStructuredInSVTK =
  svtkm::List<svtkm::cont::CellSetStructured<3>, svtkm::cont::CellSetStructured<2> >;
using CellListStructuredOutSVTK =
  svtkm::List<svtkm::cont::CellSetPermutation<svtkm::cont::CellSetStructured<3> >,
    svtkm::cont::CellSetPermutation<svtkm::cont::CellSetStructured<2> > >;

// svtkCellArray may use either 32 or 64 bit arrays to hold connectivity/offset
// data, so we may be using ArrayHandleCast to convert to svtkm::Ids.
#ifdef SVTKM_USE_64BIT_IDS
using Int32AOSHandle = svtkm::cont::ArrayHandle<svtkTypeInt32>;
using Int32AsIdAOSHandle = svtkm::cont::ArrayHandleCast<svtkm::Id, Int32AOSHandle>;
using Int32AsIdAOSStorage = typename Int32AsIdAOSHandle::StorageTag;

using CellSetExplicit32Bit = svtkm::cont::CellSetExplicit<svtkm::cont::StorageTagBasic,
  Int32AsIdAOSStorage, Int32AsIdAOSStorage>;
using CellSetExplicit64Bit = svtkm::cont::CellSetExplicit<svtkm::cont::StorageTagBasic,
  svtkm::cont::StorageTagBasic, svtkm::cont::StorageTagBasic>;
using CellSetSingleType32Bit = svtkm::cont::CellSetSingleType<Int32AsIdAOSStorage>;
using CellSetSingleType64Bit = svtkm::cont::CellSetSingleType<svtkm::cont::StorageTagBasic>;
#else  // SVTKM_USE_64BIT_IDS
using Int64AOSHandle = svtkm::cont::ArrayHandle<svtkTypeInt64, svtkm::cont::StorageTagBasic>;
using Int64AsIdAOSHandle = svtkm::cont::ArrayHandleCast<svtkm::Id, Int64AOSHandle>;
using Int64AsIdAOSStorage = typename Int64AsIdAOSHandle::StorageTag;

using CellSetExplicit32Bit = svtkm::cont::CellSetExplicit<svtkm::cont::StorageTagBasic,
  svtkm::cont::StorageTagBasic, svtkm::cont::StorageTagBasic>;
using CellSetExplicit64Bit = svtkm::cont::CellSetExplicit<svtkm::cont::StorageTagBasic,
  Int64AsIdAOSStorage, Int64AsIdAOSStorage>;
using CellSetSingleType32Bit = svtkm::cont::CellSetSingleType<svtkm::cont::StorageTagBasic>;
using CellSetSingleType64Bit = svtkm::cont::CellSetSingleType<Int64AsIdAOSStorage>;
#endif // SVTKM_USE_64BIT_IDS

//------------------------------------------------------------------------------
using CellListUnstructuredInSVTK = svtkm::List< //
  CellSetExplicit32Bit,                       //
  CellSetExplicit64Bit,                       //
  CellSetSingleType32Bit,                     //
  CellSetSingleType64Bit                      //
  >;

using CellListUnstructuredOutSVTK = svtkm::List<                     //
  svtkm::cont::CellSetExplicit<>,                                   //
  svtkm::cont::CellSetSingleType<>,                                 //
  CellSetExplicit32Bit,                                            //
  CellSetExplicit64Bit,                                            //
  CellSetSingleType32Bit,                                          //
  CellSetSingleType64Bit,                                          //
  svtkm::cont::CellSetPermutation<CellSetExplicit32Bit>,            //
  svtkm::cont::CellSetPermutation<CellSetExplicit64Bit>,            //
  svtkm::cont::CellSetPermutation<CellSetSingleType32Bit>,          //
  svtkm::cont::CellSetPermutation<CellSetSingleType64Bit>,          //
  svtkm::cont::CellSetPermutation<svtkm::cont::CellSetExplicit<> >,  //
  svtkm::cont::CellSetPermutation<svtkm::cont::CellSetSingleType<> > //
  >;

//------------------------------------------------------------------------------
using CellListAllInSVTK = svtkm::ListAppend<CellListStructuredInSVTK, CellListUnstructuredInSVTK>;
using CellListAllOutSVTK = svtkm::ListAppend<CellListStructuredOutSVTK, CellListUnstructuredOutSVTK>;

} // end namespace tosvtkm

//------------------------------------------------------------------------------
class svtkmInputFilterPolicy : public svtkm::filter::PolicyBase<svtkmInputFilterPolicy>
{
public:
  using FieldTypeList = tosvtkm::FieldTypeInSVTK;

  using StructuredCellSetList = tosvtkm::CellListStructuredInSVTK;
  using UnstructuredCellSetList = tosvtkm::CellListUnstructuredInSVTK;
  using AllCellSetList = tosvtkm::CellListAllInSVTK;
};

//------------------------------------------------------------------------------
class svtkmOutputFilterPolicy : public svtkm::filter::PolicyBase<svtkmOutputFilterPolicy>
{
public:
  using FieldTypeList = tosvtkm::FieldTypeOutSVTK;

  using StructuredCellSetList = tosvtkm::CellListStructuredOutSVTK;
  using UnstructuredCellSetList = tosvtkm::CellListUnstructuredOutSVTK;
  using AllCellSetList = tosvtkm::CellListAllOutSVTK;
};

#endif
#endif
#endif
// SVTK-HeaderTest-Exclude: svtkmFilterPolicy.h
