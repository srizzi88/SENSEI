/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositedSynchronizedRenderers.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCompositedSynchronizedRenderers
 *
 * svtkCompositedSynchronizedRenderers is svtkSynchronizedRenderers that uses
 * svtkCompositer to composite the images on the root node.
 */

#ifndef svtkCompositedSynchronizedRenderers_h
#define svtkCompositedSynchronizedRenderers_h

#include "svtkRenderingParallelModule.h" // For export macro
#include "svtkSynchronizedRenderers.h"

class svtkFloatArray;
class svtkCompositer;

class SVTKRENDERINGPARALLEL_EXPORT svtkCompositedSynchronizedRenderers
  : public svtkSynchronizedRenderers
{
public:
  static svtkCompositedSynchronizedRenderers* New();
  svtkTypeMacro(svtkCompositedSynchronizedRenderers, svtkSynchronizedRenderers);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the composite. svtkTreeCompositer is used by default.
   */
  void SetCompositer(svtkCompositer*);
  svtkGetObjectMacro(Compositer, svtkCompositer);
  //@}

protected:
  svtkCompositedSynchronizedRenderers();
  ~svtkCompositedSynchronizedRenderers() override;

  void MasterEndRender() override;
  void SlaveEndRender() override;
  void CaptureRenderedDepthBuffer(svtkFloatArray* depth_buffer);

  svtkCompositer* Compositer;

private:
  svtkCompositedSynchronizedRenderers(const svtkCompositedSynchronizedRenderers&) = delete;
  void operator=(const svtkCompositedSynchronizedRenderers&) = delete;
};

#endif
