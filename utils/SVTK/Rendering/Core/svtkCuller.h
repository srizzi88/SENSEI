/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCuller.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCuller
 * @brief   a superclass for prop cullers
 *
 * A culler has a cull method called by the svtkRenderer. The cull
 * method is called before any rendering is performed,
 * and it allows the culler to do some processing on the props and
 * to modify their AllocatedRenderTime and re-order them in the prop list.
 *
 * @sa
 * svtkFrustumCoverageCuller
 */

#ifndef svtkCuller_h
#define svtkCuller_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkProp;
class svtkRenderer;

class SVTKRENDERINGCORE_EXPORT svtkCuller : public svtkObject
{
public:
  svtkTypeMacro(svtkCuller, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * This is called outside the render loop by svtkRenderer
   */
  virtual double Cull(svtkRenderer* ren, svtkProp** propList, int& listLength, int& initialized) = 0;

protected:
  svtkCuller();
  ~svtkCuller() override;

private:
  svtkCuller(const svtkCuller&) = delete;
  void operator=(const svtkCuller&) = delete;
};

#endif
