/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLHardwareSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLHardwareSelector
 * @brief   implements the device specific code of
 *  svtkOpenGLHardwareSelector.
 *
 *
 * Implements the device specific code of svtkOpenGLHardwareSelector.
 *
 * @sa
 * svtkHardwareSelector
 */

#ifndef svtkOpenGLHardwareSelector_h
#define svtkOpenGLHardwareSelector_h

#include "svtkHardwareSelector.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLHardwareSelector : public svtkHardwareSelector
{
public:
  static svtkOpenGLHardwareSelector* New();
  svtkTypeMacro(svtkOpenGLHardwareSelector, svtkHardwareSelector);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Called by the mapper before and after
   * rendering each prop.
   */
  void BeginRenderProp() override;
  void EndRenderProp() override;

  /**
   * Called by any svtkMapper or svtkProp subclass to render a composite-index.
   * Currently indices >= 0xffffff are not supported.
   */
  void RenderCompositeIndex(unsigned int index) override;

  /**
   * Called by any svtkMapper or subclass to render process id. This has any
   * effect when this->UseProcessIdFromData is true.
   */
  void RenderProcessId(unsigned int processid) override;

  // we need to initialize the depth buffer
  void BeginSelection() override;
  void EndSelection() override;

protected:
  svtkOpenGLHardwareSelector();
  ~svtkOpenGLHardwareSelector() override;

  void PreCapturePass(int pass) override;
  void PostCapturePass(int pass) override;

  // Called internally before each prop is rendered
  // for device specific configuration/preparation etc.
  void BeginRenderProp(svtkRenderWindow*) override;
  void EndRenderProp(svtkRenderWindow*) override;

  void SavePixelBuffer(int passNo) override;

  int OriginalMultiSample;
  bool OriginalBlending;

private:
  svtkOpenGLHardwareSelector(const svtkOpenGLHardwareSelector&) = delete;
  void operator=(const svtkOpenGLHardwareSelector&) = delete;
};

#endif
