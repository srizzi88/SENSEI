/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayViewNodeFactory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOSPRayViewNodeFactory
 * @brief   matches svtk rendering classes to
 * specific ospray ViewNode classes
 *
 * Ensures that svtkOSPRayPass makes ospray specific translator instances
 * for every SVTK rendering pipeline class instance it encounters.
 */

#ifndef svtkOSPRayViewNodeFactory_h
#define svtkOSPRayViewNodeFactory_h

#include "svtkRenderingRayTracingModule.h" // For export macro
#include "svtkViewNodeFactory.h"

class SVTKRENDERINGRAYTRACING_EXPORT svtkOSPRayViewNodeFactory : public svtkViewNodeFactory
{
public:
  static svtkOSPRayViewNodeFactory* New();
  svtkTypeMacro(svtkOSPRayViewNodeFactory, svtkViewNodeFactory);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkOSPRayViewNodeFactory();
  ~svtkOSPRayViewNodeFactory() override;

private:
  svtkOSPRayViewNodeFactory(const svtkOSPRayViewNodeFactory&) = delete;
  void operator=(const svtkOSPRayViewNodeFactory&) = delete;
};

#endif
