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

#ifndef svtk_m_filter_PolicyExtrude_h
#define svtk_m_filter_PolicyExtrude_h

#include <svtkm/cont/ArrayHandleExtrudeCoords.h>
#include <svtkm/cont/CellSetExtrude.h>

#include <svtkm/List.h>
#include <svtkm/filter/PolicyDefault.h>

namespace svtkm
{
namespace filter
{

struct SVTKM_ALWAYS_EXPORT PolicyExtrude : svtkm::filter::PolicyBase<PolicyExtrude>
{
public:
  using UnstructuredCellSetList = svtkm::List<svtkm::cont::CellSetExtrude>;
  using AllCellSetList = svtkm::List<svtkm::cont::CellSetExtrude>;
  //Todo: add in Cylinder storage tag when it is written
  using CoordinateStorageList =
    svtkm::List<svtkm::cont::StorageTagBasic, svtkm::cont::internal::StorageTagExtrude>;
};
}
}

#endif
