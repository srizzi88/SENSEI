//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_PolicyDefault_h
#define svtk_m_filter_PolicyDefault_h

#include <svtkm/filter/PolicyBase.h>

namespace svtkm
{
namespace filter
{

struct PolicyDefault : svtkm::filter::PolicyBase<PolicyDefault>
{
  // Inherit defaults from PolicyBase
};
}
}

#endif //svtk_m_filter_PolicyDefault_h
