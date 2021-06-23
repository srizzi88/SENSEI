/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLVolumeOpacityTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkOpenGLVolumeOpacityTable_h
#define svtkOpenGLVolumeOpacityTable_h
#ifndef __SVTK_WRAP__

#include "svtkOpenGLVolumeLookupTable.h"

#include "svtkVolumeMapper.h"

// Forward declarations
class svtkOpenGLRenderWindow;

//----------------------------------------------------------------------------
class svtkOpenGLVolumeOpacityTable : public svtkOpenGLVolumeLookupTable
{
public:
  svtkTypeMacro(svtkOpenGLVolumeOpacityTable, svtkOpenGLVolumeLookupTable);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkOpenGLVolumeOpacityTable* New();

protected:
  svtkOpenGLVolumeOpacityTable() = default;

  /**
   * Update the internal texture object using the opacity transfer function
   */
  void InternalUpdate(svtkObject* func, int blendMode, double sampleDistance, double unitDistance,
    int filterValue) override;

  /**
   * Test whether the internal function needs to be updated.
   */
  bool NeedsUpdate(
    svtkObject* func, double scalarRange[2], int blendMode, double sampleDistance) override;

  int LastBlendMode = svtkVolumeMapper::MAXIMUM_INTENSITY_BLEND;
  double LastSampleDistance = 1.0;

private:
  svtkOpenGLVolumeOpacityTable(const svtkOpenGLVolumeOpacityTable&) = delete;
  svtkOpenGLVolumeOpacityTable& operator=(const svtkOpenGLVolumeOpacityTable&) = delete;
};

#endif // __SVTK_WRAP__
#endif // svtkOpenGLVolumeOpacityTable_h
// SVTK-HeaderTest-Exclude: svtkOpenGLVolumeOpacityTable.h
