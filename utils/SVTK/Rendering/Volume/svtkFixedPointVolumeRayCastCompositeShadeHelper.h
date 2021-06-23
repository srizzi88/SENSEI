/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFixedPointVolumeRayCastCompositeShadeHelper.h
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkFixedPointVolumeRayCastCompositeShadeHelper
 * @brief   A helper that generates composite images for the volume ray cast mapper
 *
 * This is one of the helper classes for the svtkFixedPointVolumeRayCastMapper.
 * It will generate composite images using an alpha blending operation.
 * This class should not be used directly, it is a helper class for
 * the mapper and has no user-level API.
 *
 * @sa
 * svtkFixedPointVolumeRayCastMapper
 */

#ifndef svtkFixedPointVolumeRayCastCompositeShadeHelper_h
#define svtkFixedPointVolumeRayCastCompositeShadeHelper_h

#include "svtkFixedPointVolumeRayCastHelper.h"
#include "svtkRenderingVolumeModule.h" // For export macro

class svtkFixedPointVolumeRayCastMapper;
class svtkVolume;

class SVTKRENDERINGVOLUME_EXPORT svtkFixedPointVolumeRayCastCompositeShadeHelper
  : public svtkFixedPointVolumeRayCastHelper
{
public:
  static svtkFixedPointVolumeRayCastCompositeShadeHelper* New();
  svtkTypeMacro(svtkFixedPointVolumeRayCastCompositeShadeHelper, svtkFixedPointVolumeRayCastHelper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void GenerateImage(int threadID, int threadCount, svtkVolume* vol,
    svtkFixedPointVolumeRayCastMapper* mapper) override;

protected:
  svtkFixedPointVolumeRayCastCompositeShadeHelper();
  ~svtkFixedPointVolumeRayCastCompositeShadeHelper() override;

private:
  svtkFixedPointVolumeRayCastCompositeShadeHelper(
    const svtkFixedPointVolumeRayCastCompositeShadeHelper&) = delete;
  void operator=(const svtkFixedPointVolumeRayCastCompositeShadeHelper&) = delete;
};

#endif
