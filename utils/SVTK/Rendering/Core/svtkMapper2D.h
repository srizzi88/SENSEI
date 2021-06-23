/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMapper2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMapper2D
 * @brief   abstract class specifies interface for objects which render 2D actors
 *
 * svtkMapper2D is an abstract class which defines the interface for objects
 * which render two dimensional actors (svtkActor2D).
 *
 * @sa
 * svtkActor2D
 */

#ifndef svtkMapper2D_h
#define svtkMapper2D_h

#include "svtkAbstractMapper.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkViewport;
class svtkActor2D;

class SVTKRENDERINGCORE_EXPORT svtkMapper2D : public svtkAbstractMapper
{
public:
  svtkTypeMacro(svtkMapper2D, svtkAbstractMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  virtual void RenderOverlay(svtkViewport*, svtkActor2D*) {}
  virtual void RenderOpaqueGeometry(svtkViewport*, svtkActor2D*) {}
  virtual void RenderTranslucentPolygonalGeometry(svtkViewport*, svtkActor2D*) {}
  virtual svtkTypeBool HasTranslucentPolygonalGeometry() { return 0; }

protected:
  svtkMapper2D() {}
  ~svtkMapper2D() override {}

private:
  svtkMapper2D(const svtkMapper2D&) = delete;
  void operator=(const svtkMapper2D&) = delete;
};

#endif
