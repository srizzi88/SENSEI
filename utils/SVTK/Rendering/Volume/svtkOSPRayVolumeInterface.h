/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayVolumeInterface.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOSPRayVolumeInterface
 * @brief   Removes link dependence
 * on optional ospray module.
 *
 * Class allows SmartVolume to use OSPRay for rendering when ospray
 * is enabled. When disabled, this class does nothing but return a warning.
 */

#ifndef svtkOSPRayVolumeInterface_h
#define svtkOSPRayVolumeInterface_h

#include "svtkRenderingVolumeModule.h" // For export macro
#include "svtkVolumeMapper.h"

class svtkRenderer;
class svtkVolume;

class SVTKRENDERINGVOLUME_EXPORT svtkOSPRayVolumeInterface : public svtkVolumeMapper
{
public:
  static svtkOSPRayVolumeInterface* New();
  svtkTypeMacro(svtkOSPRayVolumeInterface, svtkVolumeMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Overridden to warn about lack of OSPRay if not overridden.
   */
  void Render(svtkRenderer*, svtkVolume*) override;

protected:
  svtkOSPRayVolumeInterface();
  ~svtkOSPRayVolumeInterface() override;

private:
  svtkOSPRayVolumeInterface(const svtkOSPRayVolumeInterface&) = delete;
  void operator=(const svtkOSPRayVolumeInterface&) = delete;
};

#endif
