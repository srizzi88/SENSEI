//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/source/Source.h>

namespace svtkm
{
namespace source
{

//Fix the vtable for Source to be in the svtkm_source library
Source::Source() = default;
Source::~Source() = default;


} // namespace source
} // namespace svtkm
