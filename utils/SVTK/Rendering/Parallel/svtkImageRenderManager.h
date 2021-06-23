/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageRenderManager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageRenderManager
 * @brief   An object to control sort-first parallel rendering.
 *
 *
 * svtkImageRenderManager is a subclass of svtkParallelRenderManager that
 * uses RGBA compositing (blending) to do parallel rendering.
 * This is the exact opposite of svtkCompositeRenderManager.
 * It actually does nothing special. It relies on the rendering pipeline to be
 * initialized with a svtkCompositeRGBAPass.
 * Compositing makes sense only for renderers in layer 0.
 * @sa
 * svtkCompositeRGBAPass
 */

#ifndef svtkImageRenderManager_h
#define svtkImageRenderManager_h

#include "svtkParallelRenderManager.h"
#include "svtkRenderingParallelModule.h" // For export macro

class SVTKRENDERINGPARALLEL_EXPORT svtkImageRenderManager : public svtkParallelRenderManager
{
public:
  svtkTypeMacro(svtkImageRenderManager, svtkParallelRenderManager);
  static svtkImageRenderManager* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkImageRenderManager();
  ~svtkImageRenderManager() override;

  void PreRenderProcessing() override;
  void PostRenderProcessing() override;

private:
  svtkImageRenderManager(const svtkImageRenderManager&) = delete;
  void operator=(const svtkImageRenderManager&) = delete;
};

#endif // svtkImageRenderManager_h
