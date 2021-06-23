/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLVolumeGradientOpacityTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkOpenGLVolumeGradientOpacityTable_h
#define svtkOpenGLVolumeGradientOpacityTable_h
#ifndef __SVTK_WRAP__

#include "svtkOpenGLVolumeLookupTable.h"

// Forward declarations
class svtkOpenGLRenderWindow;

//----------------------------------------------------------------------------
class svtkOpenGLVolumeGradientOpacityTable : public svtkOpenGLVolumeLookupTable
{
public:
  svtkTypeMacro(svtkOpenGLVolumeGradientOpacityTable, svtkOpenGLVolumeLookupTable);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkOpenGLVolumeGradientOpacityTable* New();

protected:
  svtkOpenGLVolumeGradientOpacityTable() = default;

  /**
   * Update the internal texture object using the gradient opacity transfer
   * function
   */
  void InternalUpdate(svtkObject* func, int blendMode, double sampleDistance, double unitDistance,
    int filterValue) override;

private:
  svtkOpenGLVolumeGradientOpacityTable(const svtkOpenGLVolumeGradientOpacityTable&) = delete;
  svtkOpenGLVolumeGradientOpacityTable& operator=(
    const svtkOpenGLVolumeGradientOpacityTable&) = delete;
};

#endif // __SVTK_WRAP__
#endif // svtkOpenGLVolumeGradientOpacityTable_h
// SVTK-HeaderTest-Exclude: svtkOpenGLVolumeGradientOpacityTable.h
