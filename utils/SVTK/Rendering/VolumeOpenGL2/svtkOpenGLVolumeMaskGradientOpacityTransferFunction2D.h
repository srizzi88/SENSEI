/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D_h
#define svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D_h
#ifndef __SVTK_WRAP__

#include "svtkOpenGLVolumeLookupTable.h"

#include "svtkNew.h"

// Forward declarations
class svtkOpenGLRenderWindow;

/**
 * \brief 2D Transfer function container for label map mask gradient opacity.
 *
 * Manages the texture fetched by the fragment shader when TransferFunction2D
 * mode is active. Update() assumes the svtkImageData instance used as source
 * is of type SVTK_FLOAT and has 1 component (svtkVolumeProperty ensures this
 * is the case when the function is set).
 *
 * \sa svtkVolumeProperty::SetLabelGradientOpacity
 */
class svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D : public svtkOpenGLVolumeLookupTable
{
public:
  svtkTypeMacro(svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D, svtkOpenGLVolumeLookupTable);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D* New();

protected:
  svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D();

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
  svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D(const svtkOpenGLVolumeLookupTable&) = delete;
  svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D& operator=(
    const svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D&) = delete;
};

#endif // __SVTK_WRAP__
#endif // svtkOpenGLVolumeMaskTransferFunction2D_h
// SVTK-HeaderTest-Exclude: svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D.h
