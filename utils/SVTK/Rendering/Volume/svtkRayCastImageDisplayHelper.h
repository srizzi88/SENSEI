/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRayCastImageDisplayHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkRayCastImageDisplayHelper
 * @brief   helper class that draws the image to the screen
 *
 * This is a helper class for drawing images created from ray casting on the screen.
 * This is the abstract device-independent superclass.
 *
 * @sa
 * svtkUnstructuredGridVolumeRayCastMapper
 * svtkOpenGLRayCastImageDisplayHelper
 */

#ifndef svtkRayCastImageDisplayHelper_h
#define svtkRayCastImageDisplayHelper_h

#include "svtkObject.h"
#include "svtkRenderingVolumeModule.h" // For export macro

class svtkFixedPointRayCastImage;
class svtkRenderer;
class svtkVolume;
class svtkWindow;

class SVTKRENDERINGVOLUME_EXPORT svtkRayCastImageDisplayHelper : public svtkObject
{
public:
  static svtkRayCastImageDisplayHelper* New();
  svtkTypeMacro(svtkRayCastImageDisplayHelper, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  virtual void RenderTexture(svtkVolume* vol, svtkRenderer* ren, int imageMemorySize[2],
    int imageViewportSize[2], int imageInUseSize[2], int imageOrigin[2], float requestedDepth,
    unsigned char* image) = 0;

  virtual void RenderTexture(svtkVolume* vol, svtkRenderer* ren, int imageMemorySize[2],
    int imageViewportSize[2], int imageInUseSize[2], int imageOrigin[2], float requestedDepth,
    unsigned short* image) = 0;

  virtual void RenderTexture(
    svtkVolume* vol, svtkRenderer* ren, svtkFixedPointRayCastImage* image, float requestedDepth) = 0;

  svtkSetClampMacro(PreMultipliedColors, svtkTypeBool, 0, 1);
  svtkGetMacro(PreMultipliedColors, svtkTypeBool);
  svtkBooleanMacro(PreMultipliedColors, svtkTypeBool);

  //@{
  /**
   * Set / Get the pixel scale to be applied to the image before display.
   * Can be set to scale the incoming pixel values - for example the
   * fixed point mapper uses the unsigned short API but with 15 bit
   * values so needs a scale of 2.0.
   */
  svtkSetMacro(PixelScale, float);
  svtkGetMacro(PixelScale, float);
  //@}

  /**
   * Derived class should implement this if needed
   */
  virtual void ReleaseGraphicsResources(svtkWindow*) {}

protected:
  svtkRayCastImageDisplayHelper();
  ~svtkRayCastImageDisplayHelper() override;

  /**
   * Have the colors already been multiplied by alpha?
   */
  svtkTypeBool PreMultipliedColors;

  float PixelScale;

private:
  svtkRayCastImageDisplayHelper(const svtkRayCastImageDisplayHelper&) = delete;
  void operator=(const svtkRayCastImageDisplayHelper&) = delete;
};

#endif
