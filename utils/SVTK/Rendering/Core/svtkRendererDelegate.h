/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRendererDelegate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRendererDelegate
 * @brief   Render the props of a svtkRenderer
 *
 * svtkRendererDelegate is an abstract class with a pure virtual method Render.
 * This method replaces the Render method of svtkRenderer to allow custom
 * rendering from an external project. A RendererDelegate is connected to
 * a svtkRenderer with method SetDelegate(). An external project just
 * has to provide a concrete implementation of svtkRendererDelegate.
 *
 * @sa
 * svtkRenderer
 */

#ifndef svtkRendererDelegate_h
#define svtkRendererDelegate_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkRenderer;

class SVTKRENDERINGCORE_EXPORT svtkRendererDelegate : public svtkObject
{
public:
  svtkTypeMacro(svtkRendererDelegate, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Render the props of svtkRenderer if Used is on.
   */
  virtual void Render(svtkRenderer* r) = 0;

  //@{
  /**
   * Tells if the delegate has to be used by the renderer or not.
   * Initial value is off.
   */
  svtkSetMacro(Used, bool);
  svtkGetMacro(Used, bool);
  svtkBooleanMacro(Used, bool);
  //@}

protected:
  svtkRendererDelegate();
  ~svtkRendererDelegate() override;

  bool Used;

private:
  svtkRendererDelegate(const svtkRendererDelegate&) = delete;
  void operator=(const svtkRendererDelegate&) = delete;
};

#endif
