//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_internal_ScatterBase_h
#define svtk_m_worklet_internal_ScatterBase_h

#include <svtkm/internal/ExportMacros.h>
#include <svtkm/worklet/internal/DecayHelpers.h>

namespace svtkm
{
namespace worklet
{
namespace internal
{
/// Base class for all scatter classes.
///
/// This allows SVTK-m to determine when a parameter
/// is a scatter type instead of a worklet parameter.
///
struct SVTKM_ALWAYS_EXPORT ScatterBase
{
};

template <typename T>
using is_scatter = std::is_base_of<ScatterBase, remove_cvref<T>>;
}
}
}
#endif
