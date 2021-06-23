/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFixedPointVolumeRayCastCompositeGOHelper.h
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkFixedPointVolumeRayCastCompositeGOHelper
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

#ifndef svtkFixedPointVolumeRayCastCompositeGOHelper_h
#define svtkFixedPointVolumeRayCastCompositeGOHelper_h

#include "svtkFixedPointVolumeRayCastHelper.h"
#include "svtkRenderingVolumeModule.h" // For export macro

class svtkFixedPointVolumeRayCastMapper;
class svtkVolume;

class SVTKRENDERINGVOLUME_EXPORT svtkFixedPointVolumeRayCastCompositeGOHelper
  : public svtkFixedPointVolumeRayCastHelper
{
public:
  static svtkFixedPointVolumeRayCastCompositeGOHelper* New();
  svtkTypeMacro(svtkFixedPointVolumeRayCastCompositeGOHelper, svtkFixedPointVolumeRayCastHelper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void GenerateImage(int threadID, int threadCount, svtkVolume* vol,
    svtkFixedPointVolumeRayCastMapper* mapper) override;

protected:
  svtkFixedPointVolumeRayCastCompositeGOHelper();
  ~svtkFixedPointVolumeRayCastCompositeGOHelper() override;

private:
  svtkFixedPointVolumeRayCastCompositeGOHelper(
    const svtkFixedPointVolumeRayCastCompositeGOHelper&) = delete;
  void operator=(const svtkFixedPointVolumeRayCastCompositeGOHelper&) = delete;
};

#endif
