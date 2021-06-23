/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLVolumeRGBTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkOpenGLVolumeRGBTable_h
#define svtkOpenGLVolumeRGBTable_h
#ifndef __SVTK_WRAP__

#include "svtkOpenGLVolumeLookupTable.h"

// Forward declarations
class svtkOpenGLRenderWindow;

//----------------------------------------------------------------------------
class svtkOpenGLVolumeRGBTable : public svtkOpenGLVolumeLookupTable
{
public:
  svtkTypeMacro(svtkOpenGLVolumeRGBTable, svtkOpenGLVolumeLookupTable);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkOpenGLVolumeRGBTable* New();

protected:
  svtkOpenGLVolumeRGBTable();

  /**
   * Update the internal texture object using the color transfer function
   */
  void InternalUpdate(svtkObject* func, int blendMode, double sampleDistance, double unitDistance,
    int filterValue) override;

private:
  svtkOpenGLVolumeRGBTable(const svtkOpenGLVolumeRGBTable&) = delete;
  svtkOpenGLVolumeRGBTable& operator=(const svtkOpenGLVolumeRGBTable&) = delete;
};

#endif // __SVTK_WRAP__
#endif // svtkOpenGLVolumeRGBTable_h
// SVTK-HeaderTest-Exclude: svtkOpenGLVolumeRGBTable.h
