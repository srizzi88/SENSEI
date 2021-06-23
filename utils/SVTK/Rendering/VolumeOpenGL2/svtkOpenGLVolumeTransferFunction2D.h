/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLVolumeTransferFunction2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkOpenGLTransferFunction2D_h
#define svtkOpenGLTransferFunction2D_h
#ifndef __SVTK_WRAP__

#include "svtkOpenGLVolumeLookupTable.h"

#include "svtkNew.h"

// Forward declarations
class svtkOpenGLRenderWindow;
class svtkImageResize;

/**
 * \brief 2D Transfer function container.
 *
 * Manages the texture fetched by the fragment shader when TransferFunction2D
 * mode is active. Update() assumes the svtkImageData instance used as source
 * is of type SVTK_FLOAT and has 4 components (svtkVolumeProperty ensures this
 * is the case when the function is set).
 *
 * \sa svtkVolumeProperty::SetTransferFunction2D
 */
class svtkOpenGLVolumeTransferFunction2D : public svtkOpenGLVolumeLookupTable
{
public:
  svtkTypeMacro(svtkOpenGLVolumeTransferFunction2D, svtkOpenGLVolumeLookupTable);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkOpenGLVolumeTransferFunction2D* New();

protected:
  svtkOpenGLVolumeTransferFunction2D();

  /**
   * Update the internal texture object using the 2D image data
   */
  void InternalUpdate(svtkObject* func, int blendMode, double sampleDistance, double unitDistance,
    int filterValue) override;

  /**
   * Override needs update to not test for scalar range changes since the range
   * is encoded in the svtkImageData
   */
  bool NeedsUpdate(
    svtkObject* func, double scalarRange[2], int blendMode, double sampleDistance) override;

  /**
   * Override allocate table to do nothing as no internal table management is
   * needed.
   */
  void AllocateTable() override;

  svtkNew<svtkImageResize> ResizeFilter;

private:
  svtkOpenGLVolumeTransferFunction2D(const svtkOpenGLVolumeTransferFunction2D&) = delete;
  svtkOpenGLVolumeTransferFunction2D& operator=(const svtkOpenGLVolumeTransferFunction2D&) = delete;
};

#endif // __SVTK_WRAP__
#endif // svtkOpenGLTransferFunction2D_h
// SVTK-HeaderTest-Exclude: svtkOpenGLVolumeTransferFunction2D.h
