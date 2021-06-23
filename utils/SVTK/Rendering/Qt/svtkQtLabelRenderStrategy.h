/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtLabelRenderStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkQtLabelRenderStrategy
 * @brief   Renders labels with Qt
 *
 *
 * This class uses Qt to render labels and compute sizes. The labels are
 * rendered to a QImage, then EndFrame() converts that image to a svtkImageData
 * and textures the image onto a quad spanning the render area.
 */

#ifndef svtkQtLabelRenderStrategy_h
#define svtkQtLabelRenderStrategy_h

#include "svtkLabelRenderStrategy.h"
#include "svtkRenderingQtModule.h" // For export macro

class svtkLabelSizeCalculator;
class svtkLabeledDataMapper;
class svtkPlaneSource;
class svtkPolyDataMapper2D;
class svtkQImageToImageSource;
class svtkTexture;
class svtkTexturedActor2D;
class svtkTextureMapToPlane;

class SVTKRENDERINGQT_EXPORT svtkQtLabelRenderStrategy : public svtkLabelRenderStrategy
{
public:
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkQtLabelRenderStrategy, svtkLabelRenderStrategy);
  static svtkQtLabelRenderStrategy* New();

  /**
   * Compute the bounds of a label. Must be performed after the renderer is set.
   */
  void ComputeLabelBounds(svtkTextProperty* tprop, svtkStdString label, double bds[4]) override
  {
    this->Superclass::ComputeLabelBounds(tprop, label, bds);
  }
  void ComputeLabelBounds(svtkTextProperty* tprop, svtkUnicodeString label, double bds[4]) override;

  //@{
  /**
   * Render a label at a location in world coordinates.
   * Must be performed between StartFrame() and EndFrame() calls.
   */
  void RenderLabel(int x[2], svtkTextProperty* tprop, svtkStdString label) override
  {
    this->Superclass::RenderLabel(x, tprop, label);
  }
  void RenderLabel(int x[2], svtkTextProperty* tprop, svtkStdString label, int maxWidth) override
  {
    this->Superclass::RenderLabel(x, tprop, label, maxWidth);
  }
  void RenderLabel(int x[2], svtkTextProperty* tprop, svtkUnicodeString label) override;
  void RenderLabel(int x[2], svtkTextProperty* tprop, svtkUnicodeString label, int maxWidth) override;
  //@}

  /**
   * Start a rendering frame. Renderer must be set.
   */
  void StartFrame() override;

  /**
   * End a rendering frame.
   */
  void EndFrame() override;

  /**
   * Release any graphics resources that are being consumed by this strategy.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow* window) override;

protected:
  svtkQtLabelRenderStrategy();
  ~svtkQtLabelRenderStrategy() override;

  class Internals;
  Internals* Implementation;

  svtkQImageToImageSource* QImageToImage;
  svtkPlaneSource* PlaneSource;
  svtkTextureMapToPlane* TextureMapToPlane;
  svtkTexture* Texture;
  svtkPolyDataMapper2D* Mapper;
  svtkTexturedActor2D* Actor;
  bool AntialiasText; // Should the text be antialiased, inherited from render window.

private:
  svtkQtLabelRenderStrategy(const svtkQtLabelRenderStrategy&) = delete;
  void operator=(const svtkQtLabelRenderStrategy&) = delete;
};

#endif
