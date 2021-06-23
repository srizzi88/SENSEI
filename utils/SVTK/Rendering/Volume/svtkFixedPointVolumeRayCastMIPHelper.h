/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFixedPointVolumeRayCastMIPHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkFixedPointVolumeRayCastMIPHelper
 * @brief   A helper that generates MIP images for the volume ray cast mapper
 *
 * This is one of the helper classes for the svtkFixedPointVolumeRayCastMapper.
 * It will generate maximum intensity images.
 * This class should not be used directly, it is a helper class for
 * the mapper and has no user-level API.
 *
 * @sa
 * svtkFixedPointVolumeRayCastMapper
 */

#ifndef svtkFixedPointVolumeRayCastMIPHelper_h
#define svtkFixedPointVolumeRayCastMIPHelper_h

#include "svtkFixedPointVolumeRayCastHelper.h"
#include "svtkRenderingVolumeModule.h" // For export macro

class svtkFixedPointVolumeRayCastMapper;
class svtkVolume;

class SVTKRENDERINGVOLUME_EXPORT svtkFixedPointVolumeRayCastMIPHelper
  : public svtkFixedPointVolumeRayCastHelper
{
public:
  static svtkFixedPointVolumeRayCastMIPHelper* New();
  svtkTypeMacro(svtkFixedPointVolumeRayCastMIPHelper, svtkFixedPointVolumeRayCastHelper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void GenerateImage(int threadID, int threadCount, svtkVolume* vol,
    svtkFixedPointVolumeRayCastMapper* mapper) override;

protected:
  svtkFixedPointVolumeRayCastMIPHelper();
  ~svtkFixedPointVolumeRayCastMIPHelper() override;

private:
  svtkFixedPointVolumeRayCastMIPHelper(const svtkFixedPointVolumeRayCastMIPHelper&) = delete;
  void operator=(const svtkFixedPointVolumeRayCastMIPHelper&) = delete;
};

#endif
