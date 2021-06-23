/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFrameBufferObjectBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkFrameBufferObjectBase
 * @brief   abstract interface to OpenGL FBOs
 *
 * API for classes that encapsulate an OpenGL Frame Buffer Object.
 */

#ifndef svtkFrameBufferObjectBase_h
#define svtkFrameBufferObjectBase_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkRenderer;
class svtkProp;
class svtkInformation;

class SVTKRENDERINGCORE_EXPORT svtkFrameBufferObjectBase : public svtkObject
{
public:
  svtkTypeMacro(svtkFrameBufferObjectBase, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Dimensions in pixels of the framebuffer.
   */
  virtual int* GetLastSize() = 0;
  virtual void GetLastSize(int& _arg1, int& _arg2) = 0;
  virtual void GetLastSize(int _arg[2]) = 0;
  //@}

protected:
  svtkFrameBufferObjectBase(); // no default constructor.
  ~svtkFrameBufferObjectBase() override;

private:
  svtkFrameBufferObjectBase(const svtkFrameBufferObjectBase&) = delete;
  void operator=(const svtkFrameBufferObjectBase&) = delete;
};

#endif
