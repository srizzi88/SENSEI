/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFreeTypeLabelRenderStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkFreeTypeLabelRenderStrategy
 * @brief   Renders labels with freetype
 *
 *
 * Uses the FreeType to render labels and compute label sizes.
 * This strategy may be used with svtkLabelPlacementMapper.
 */

#ifndef svtkFreeTypeLabelRenderStrategy_h
#define svtkFreeTypeLabelRenderStrategy_h

#include "svtkLabelRenderStrategy.h"
#include "svtkRenderingLabelModule.h" // For export macro

class svtkActor2D;
class svtkTextRenderer;
class svtkTextMapper;

class SVTKRENDERINGLABEL_EXPORT svtkFreeTypeLabelRenderStrategy : public svtkLabelRenderStrategy
{
public:
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkFreeTypeLabelRenderStrategy, svtkLabelRenderStrategy);
  static svtkFreeTypeLabelRenderStrategy* New();

  /**
   * The free type render strategy currently does not support rotation.
   */
  bool SupportsRotation() override { return false; }

  /**
   * The free type render strategy currently does not support bounded size labels.
   */
  bool SupportsBoundedSize() override { return false; }

  /**
   * Compute the bounds of a label. Must be performed after the renderer is set.
   */
  void ComputeLabelBounds(svtkTextProperty* tprop, svtkStdString label, double bds[4]) override
  {
    this->Superclass::ComputeLabelBounds(tprop, label, bds);
  }
  void ComputeLabelBounds(svtkTextProperty* tprop, svtkUnicodeString label, double bds[4]) override;

  /**
   * Render a label at a location in world coordinates.
   * Must be performed between StartFrame() and EndFrame() calls.
   */
  void RenderLabel(int x[2], svtkTextProperty* tprop, svtkStdString label) override
  {
    this->Superclass::RenderLabel(x, tprop, label);
  }
  void RenderLabel(int x[2], svtkTextProperty* tprop, svtkStdString label, int width) override
  {
    this->Superclass::RenderLabel(x, tprop, label, width);
  }
  void RenderLabel(int x[2], svtkTextProperty* tprop, svtkUnicodeString label) override;
  void RenderLabel(int x[2], svtkTextProperty* tprop, svtkUnicodeString label, int width) override
  {
    this->Superclass::RenderLabel(x, tprop, label, width);
  }

  /**
   * Release any graphics resources that are being consumed by this strategy.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow* window) override;

protected:
  svtkFreeTypeLabelRenderStrategy();
  ~svtkFreeTypeLabelRenderStrategy() override;

  svtkTextRenderer* TextRenderer;
  svtkTextMapper* Mapper;
  svtkActor2D* Actor;

private:
  svtkFreeTypeLabelRenderStrategy(const svtkFreeTypeLabelRenderStrategy&) = delete;
  void operator=(const svtkFreeTypeLabelRenderStrategy&) = delete;
};

#endif
