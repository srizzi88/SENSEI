/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderingCoreEnums.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkRenderingCoreEnums_h
#define svtkRenderingCoreEnums_h

// Marker shapes for plotting
enum
{
  SVTK_MARKER_NONE = 0,
  SVTK_MARKER_CROSS,
  SVTK_MARKER_PLUS,
  SVTK_MARKER_SQUARE,
  SVTK_MARKER_CIRCLE,
  SVTK_MARKER_DIAMOND,

  SVTK_MARKER_UNKNOWN // Must be last.
};

#endif // svtkRenderingCoreEnums_h
// SVTK-HeaderTest-Exclude: svtkRenderingCoreEnums.h
