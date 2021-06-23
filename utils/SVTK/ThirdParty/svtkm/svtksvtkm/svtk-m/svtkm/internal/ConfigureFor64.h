//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
//This header can be used by external application that are consuming SVTKm
//to define if SVTKm should be set to use 64bit data types. If you need to
//customize more of the svtkm type system, or what Device Adapters
//need to be included look at svtkm/internal/Configure.h for all defines that
//you can over-ride.
#ifdef svtk_m_internal_Configure_h
#error Incorrect header order. Include this header before any other SVTKm headers.
#endif

#ifndef svtk_m_internal_Configure32_h
#define svtk_m_internal_Configure32_h

#define SVTKM_USE_DOUBLE_PRECISION
#define SVTKM_USE_64BIT_IDS

#include <svtkm/internal/Configure.h>

#endif
