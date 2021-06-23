/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFixedPointVolumeRayCastCompositeGOShadeHelper.h
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkFixedPointVolumeRayCastCompositeGOShadeHelper
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

#ifndef svtkFixedPointVolumeRayCastCompositeGOShadeHelper_h
#define svtkFixedPointVolumeRayCastCompositeGOShadeHelper_h

#include "svtkFixedPointVolumeRayCastHelper.h"
#include "svtkRenderingVolumeModule.h" // For export macro

class svtkFixedPointVolumeRayCastMapper;
class svtkVolume;

class SVTKRENDERINGVOLUME_EXPORT svtkFixedPointVolumeRayCastCompositeGOShadeHelper
  : public svtkFixedPointVolumeRayCastHelper
{
public:
  static svtkFixedPointVolumeRayCastCompositeGOShadeHelper* New();
  svtkTypeMacro(svtkFixedPointVolumeRayCastCompositeGOShadeHelper, svtkFixedPointVolumeRayCastHelper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void GenerateImage(int threadID, int threadCount, svtkVolume* vol,
    svtkFixedPointVolumeRayCastMapper* mapper) override;

protected:
  svtkFixedPointVolumeRayCastCompositeGOShadeHelper();
  ~svtkFixedPointVolumeRayCastCompositeGOShadeHelper() override;

private:
  svtkFixedPointVolumeRayCastCompositeGOShadeHelper(
    const svtkFixedPointVolumeRayCastCompositeGOShadeHelper&) = delete;
  void operator=(const svtkFixedPointVolumeRayCastCompositeGOShadeHelper&) = delete;
};

#endif
