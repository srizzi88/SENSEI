/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFixedPointVolumeRayCastCompositeHelper.h
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkFixedPointVolumeRayCastCompositeHelper
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

#ifndef svtkFixedPointVolumeRayCastCompositeHelper_h
#define svtkFixedPointVolumeRayCastCompositeHelper_h

#include "svtkFixedPointVolumeRayCastHelper.h"
#include "svtkRenderingVolumeModule.h" // For export macro

class svtkFixedPointVolumeRayCastMapper;
class svtkVolume;

class SVTKRENDERINGVOLUME_EXPORT svtkFixedPointVolumeRayCastCompositeHelper
  : public svtkFixedPointVolumeRayCastHelper
{
public:
  static svtkFixedPointVolumeRayCastCompositeHelper* New();
  svtkTypeMacro(svtkFixedPointVolumeRayCastCompositeHelper, svtkFixedPointVolumeRayCastHelper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void GenerateImage(int threadID, int threadCount, svtkVolume* vol,
    svtkFixedPointVolumeRayCastMapper* mapper) override;

protected:
  svtkFixedPointVolumeRayCastCompositeHelper();
  ~svtkFixedPointVolumeRayCastCompositeHelper() override;

private:
  svtkFixedPointVolumeRayCastCompositeHelper(
    const svtkFixedPointVolumeRayCastCompositeHelper&) = delete;
  void operator=(const svtkFixedPointVolumeRayCastCompositeHelper&) = delete;
};

#endif
