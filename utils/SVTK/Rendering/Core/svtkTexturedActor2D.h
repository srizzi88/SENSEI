/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTexturedActor2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTexturedActor2D
 * @brief   actor that draws 2D data with texture support
 *
 * svtkTexturedActor2D is an Actor2D which has additional support for
 * textures, just like svtkActor. To use textures, the geometry must have
 * texture coordinates, and the texture must be set with SetTexture().
 *
 * @sa
 * svtkActor2D svtkProp svtkMapper2D svtkProperty2D
 */

#ifndef svtkTexturedActor2D_h
#define svtkTexturedActor2D_h

#include "svtkActor2D.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkProp;
class svtkTexture;
class svtkViewport;
class svtkWindow;

class SVTKRENDERINGCORE_EXPORT svtkTexturedActor2D : public svtkActor2D
{
public:
  static svtkTexturedActor2D* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkTexturedActor2D, svtkActor2D);

  //@{
  /**
   * Set/Get the texture object to control rendering texture maps.  This will
   * be a svtkTexture object. An actor does not need to have an associated
   * texture map and multiple actors can share one texture.
   */
  virtual void SetTexture(svtkTexture* texture);
  svtkGetObjectMacro(Texture, svtkTexture);
  //@}

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow* win) override;

  //@{
  /**
   * Support the standard render methods.
   */
  int RenderOverlay(svtkViewport* viewport) override;
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;
  //@}

  /**
   * Return this object's modified time.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Shallow copy of this svtkTexturedActor2D. Overrides svtkActor2D method.
   */
  void ShallowCopy(svtkProp* prop) override;

protected:
  svtkTexturedActor2D();
  ~svtkTexturedActor2D() override;

  svtkTexture* Texture;

private:
  svtkTexturedActor2D(const svtkTexturedActor2D&) = delete;
  void operator=(const svtkTexturedActor2D&) = delete;
};

#endif
