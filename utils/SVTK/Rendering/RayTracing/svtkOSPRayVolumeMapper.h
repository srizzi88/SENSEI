/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayVolumeMapper.h
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen

  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOSPRayVolumeMapper
 * @brief   Standalone OSPRayVolumeMapper.
 *
 * This is a standalone interface for ospray volume rendering to be used
 * within otherwise OpenGL rendering contexts such as within the
 * SmartVolumeMapper.
 */

#ifndef svtkOSPRayVolumeMapper_h
#define svtkOSPRayVolumeMapper_h

#include "svtkOSPRayVolumeInterface.h"
#include "svtkRenderingRayTracingModule.h" // For export macro

class svtkOSPRayPass;
class svtkRenderer;
class svtkWindow;

class SVTKRENDERINGRAYTRACING_EXPORT svtkOSPRayVolumeMapper : public svtkOSPRayVolumeInterface
{
public:
  static svtkOSPRayVolumeMapper* New();
  svtkTypeMacro(svtkOSPRayVolumeMapper, svtkOSPRayVolumeInterface);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(svtkWindow*) override;

  // Initialize internal constructs
  virtual void Init();

  /**
   * Render the volume onto the screen.
   * Overridden to use OSPRay to do the work.
   */
  virtual void Render(svtkRenderer*, svtkVolume*) override;

protected:
  svtkOSPRayVolumeMapper();
  ~svtkOSPRayVolumeMapper() override;

  svtkOSPRayPass* InternalOSPRayPass;
  svtkRenderer* InternalRenderer;
  bool Initialized;

private:
  svtkOSPRayVolumeMapper(const svtkOSPRayVolumeMapper&) = delete;
  void operator=(const svtkOSPRayVolumeMapper&) = delete;
};

#endif
