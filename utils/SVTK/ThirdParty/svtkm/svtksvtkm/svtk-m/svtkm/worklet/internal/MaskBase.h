//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_internal_MaskBase_h
#define svtk_m_worklet_internal_MaskBase_h

#include <svtkm/internal/ExportMacros.h>
#include <svtkm/worklet/internal/DecayHelpers.h>

namespace svtkm
{
namespace worklet
{
namespace internal
{

/// Base class for all mask classes.
///
/// This allows SVTK-m to determine when a parameter
/// is a mask type instead of a worklet parameter.
///
struct SVTKM_ALWAYS_EXPORT MaskBase
{
};

template <typename T>
using is_mask = std::is_base_of<MaskBase, remove_cvref<T>>;
}
}
}
#endif
