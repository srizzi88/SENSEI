/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPHardwareSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPHardwareSelector
 * @brief   svtkHardwareSelector useful for parallel
 * rendering.
 *
 * svtkPHardwareSelector is a svtkHardwareSelector that is parallel aware. It
 * relies on the fact that the application is going to use some other mechanism
 * to ensure that renders are synchronized among windows on all processes. The
 * synchronization happens from the root node. When the root node renders, all
 * processes render. Only svtkPHardwareSelector instance on the root node
 * triggers the renders. All other processes, simply listen to the StartEvent
 * fired and beginning of the render to ensure that svtkHardwareSelector's
 * CurrentPass is updated appropriately.
 */

#ifndef svtkPHardwareSelector_h
#define svtkPHardwareSelector_h

#include "svtkOpenGLHardwareSelector.h"
#include "svtkRenderingParallelModule.h" // For export macro

class SVTKRENDERINGPARALLEL_EXPORT svtkPHardwareSelector : public svtkOpenGLHardwareSelector
{
public:
  static svtkPHardwareSelector* New();
  svtkTypeMacro(svtkPHardwareSelector, svtkOpenGLHardwareSelector);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the is the root process. The root processes
   * is the only processes which has the composited result and hence the only
   * processes that capture buffers and builds selected list ids.
   */
  svtkSetMacro(ProcessIsRoot, bool);
  svtkGetMacro(ProcessIsRoot, bool);
  svtkBooleanMacro(ProcessIsRoot, bool);
  //@}

  /**
   * Overridden to only allow the superclass implementation on the root node. On
   * all other processes, the updating the internal state of the
   * svtkHardwareSelector as the capturing of buffers progresses is done as a
   * slave to the master render.
   */
  bool CaptureBuffers() override;

protected:
  svtkPHardwareSelector();
  ~svtkPHardwareSelector() override;

  void StartRender();
  void EndRender();

  bool ProcessIsRoot;

private:
  svtkPHardwareSelector(const svtkPHardwareSelector&) = delete;
  void operator=(const svtkPHardwareSelector&) = delete;

  class svtkObserver;
  friend class svtkObserver;
  svtkObserver* Observer;
};

#endif
