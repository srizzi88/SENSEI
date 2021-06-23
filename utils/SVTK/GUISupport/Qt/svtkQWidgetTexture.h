/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQWidgetTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef svtkQWidgetTexture_h
#define svtkQWidgetTexture_h

#include "svtkGUISupportQtModule.h" // For export macro
#include "svtkTextureObject.h"
#include <functional> // for ivar

class QGraphicsScene;
class QOffscreenSurface;
class QOpenGLFramebufferObject;
class QWidget;

/**
 * @class svtkQWidgetTexture
 * @brief Allows a QWidget to be used as a texture in SVTK with OpenGL
 *
 * This class works by rendering the QWidget into a Framebuffer
 * and then sending the OpenGL texture handle to SVTK for rendering.
 */
class SVTKGUISUPPORTQT_EXPORT svtkQWidgetTexture : public svtkTextureObject
{
public:
  static svtkQWidgetTexture* New();
  svtkTypeMacro(svtkQWidgetTexture, svtkTextureObject);

  //@{
  /**
   * Set/Get the QWidget that this TextureObject will render/use
   */
  void SetWidget(QWidget* w);
  QWidget* GetWidget() { return this->Widget; }
  //@}

  /**
   * get the QScene used for rendering, this is where events will
   * be forwarded to.
   */
  QGraphicsScene* GetScene() { return this->Scene; }

  /**
   * Activate and Bind the texture. Ovrloaded to handle the opengl related
   * setup at the same time. as We know the context will be active then.
   */
  void Activate() override;

  /**
   * Free resources
   */
  void ReleaseGraphicsResources(svtkWindow* win) override;

protected:
  svtkQWidgetTexture();
  ~svtkQWidgetTexture() override;

  QGraphicsScene* Scene;
  QOffscreenSurface* OffscreenSurface;
  QOpenGLFramebufferObject* Framebuffer;
  QWidget* Widget;

  // method called when the widget needs repainting
  std::function<void()> RedrawMethod;

  // internal method to setup the scene/framebuffer/etc
  void AllocateFromWidget();

private:
  svtkQWidgetTexture(const svtkQWidgetTexture&) = delete;
  void operator=(const svtkQWidgetTexture&) = delete;
};

#endif
