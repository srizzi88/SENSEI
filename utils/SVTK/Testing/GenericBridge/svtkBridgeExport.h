/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgeExport.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBridgeExport
 * @brief   manage Windows system differences
 *
 * The svtkBridgeExport captures some system differences between Unix and
 * Windows operating systems.
 */

#ifndef svtkBridgeExport_h
#define svtkBridgeExport_h
#include "svtkSystemIncludes.h"
#include "svtkTestingGenericBridgeModule.h"

#if 1
#define SVTK_BRIDGE_EXPORT
#else

#if defined(_WIN32) && defined(SVTK_BUILD_SHARED_LIBS)

#if defined(svtkBridge_EXPORTS)
#define SVTK_BRIDGE_EXPORT __declspec(dllexport)
#else
#define SVTK_BRIDGE_EXPORT __declspec(dllimport)
#endif
#else
#define SVTK_BRIDGE_EXPORT
#endif

#endif //#if 1

#endif

// SVTK-HeaderTest-Exclude: svtkBridgeExport.h
