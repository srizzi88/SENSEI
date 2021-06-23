/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeRenderManager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCompositeRenderManager
 * @brief   An object to control sort-last parallel rendering.
 *
 *
 * svtkCompositeRenderManager is a subclass of svtkParallelRenderManager that
 * uses compositing to do parallel rendering.  This class has
 * replaced svtkCompositeManager.
 *
 */

#ifndef svtkCompositeRenderManager_h
#define svtkCompositeRenderManager_h

#include "svtkParallelRenderManager.h"
#include "svtkRenderingParallelModule.h" // For export macro

class svtkCompositer;
class svtkFloatArray;

class SVTKRENDERINGPARALLEL_EXPORT svtkCompositeRenderManager : public svtkParallelRenderManager
{
public:
  svtkTypeMacro(svtkCompositeRenderManager, svtkParallelRenderManager);
  static svtkCompositeRenderManager* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the composite algorithm.
   */
  void SetCompositer(svtkCompositer* c);
  svtkGetObjectMacro(Compositer, svtkCompositer);
  //@}

protected:
  svtkCompositeRenderManager();
  ~svtkCompositeRenderManager() override;

  svtkCompositer* Compositer;

  void PreRenderProcessing() override;
  void PostRenderProcessing() override;

  svtkFloatArray* DepthData;
  svtkUnsignedCharArray* TmpPixelData;
  svtkFloatArray* TmpDepthData;

  int SavedMultiSamplesSetting;

private:
  svtkCompositeRenderManager(const svtkCompositeRenderManager&) = delete;
  void operator=(const svtkCompositeRenderManager&) = delete;
};

#endif // svtkCompositeRenderManager_h
