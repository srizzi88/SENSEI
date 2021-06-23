/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLVolumeMaskTransferFunction2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkOpenGLVolumeMaskTransferFunction2D_h
#define svtkOpenGLVolumeMaskTransferFunction2D_h
#ifndef __SVTK_WRAP__

#include "svtkOpenGLVolumeLookupTable.h"

#include "svtkNew.h"

// Forward declarations
class svtkOpenGLRenderWindow;

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
class svtkOpenGLVolumeMaskTransferFunction2D : public svtkOpenGLVolumeLookupTable
{
public:
  svtkTypeMacro(svtkOpenGLVolumeMaskTransferFunction2D, svtkOpenGLVolumeLookupTable);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkOpenGLVolumeMaskTransferFunction2D* New();

protected:
  svtkOpenGLVolumeMaskTransferFunction2D();

  /**
   * Update the internal texture object using the 2D image data
   */
  void InternalUpdate(svtkObject* func, int blendMode, double sampleDistance, double unitDistance,
    int filterValue) override;

  /**
   * Compute the ideal texture size based on the number of labels and transfer
   * functions in the label map.
   */
  void ComputeIdealTextureSize(
    svtkObject* func, int& width, int& height, svtkOpenGLRenderWindow* renWin) override;

private:
  svtkOpenGLVolumeMaskTransferFunction2D(const svtkOpenGLVolumeLookupTable&) = delete;
  svtkOpenGLVolumeMaskTransferFunction2D& operator=(
    const svtkOpenGLVolumeMaskTransferFunction2D&) = delete;
};

#endif // __SVTK_WRAP__
#endif // svtkOpenGLVolumeMaskTransferFunction2D_h
// SVTK-HeaderTest-Exclude: svtkOpenGLVolumeMaskTransferFunction2D.h
