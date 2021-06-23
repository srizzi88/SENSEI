/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLabelRenderStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLabelRenderStrategy
 * @brief   Superclass for label rendering implementations.
 *
 *
 * These methods should only be called within a mapper.
 */

#ifndef svtkLabelRenderStrategy_h
#define svtkLabelRenderStrategy_h

#include "svtkObject.h"
#include "svtkRenderingLabelModule.h" // For export macro

#include "svtkStdString.h"     // For string support
#include "svtkUnicodeString.h" // For unicode string support

class svtkRenderer;
class svtkWindow;
class svtkTextProperty;

class SVTKRENDERINGLABEL_EXPORT svtkLabelRenderStrategy : public svtkObject
{
public:
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkLabelRenderStrategy, svtkObject);

  /**
   * Whether the text rendering strategy supports rotation.
   * The superclass returns true. Subclasses should override this to
   * return the appropriate value.
   */
  virtual bool SupportsRotation() { return true; }

  /**
   * Whether the text rendering strategy supports bounded size.
   * The superclass returns true. Subclasses should override this to
   * return the appropriate value. Subclasses that return true
   * from this method should implement the version of RenderLabel()
   * that takes a maximum size (see RenderLabel()).
   */
  virtual bool SupportsBoundedSize() { return true; }

  //@{
  /**
   * Set the renderer associated with this strategy.
   */
  virtual void SetRenderer(svtkRenderer* ren);
  svtkGetObjectMacro(Renderer, svtkRenderer);
  //@}

  //@{
  /**
   * Set the default text property for the strategy.
   */
  virtual void SetDefaultTextProperty(svtkTextProperty* tprop);
  svtkGetObjectMacro(DefaultTextProperty, svtkTextProperty);
  //@}

  /**
   * Compute the bounds of a label. Must be performed after the renderer is set.
   * Only the unicode string version must be implemented in subclasses.
   */
  virtual void ComputeLabelBounds(svtkTextProperty* tprop, svtkStdString label, double bds[4])
  {
    this->ComputeLabelBounds(tprop, svtkUnicodeString::from_utf8(label.c_str()), bds);
  }
  virtual void ComputeLabelBounds(
    svtkTextProperty* tprop, svtkUnicodeString label, double bds[4]) = 0;

  /**
   * Render a label at a location in display coordinates.
   * Must be performed between StartFrame() and EndFrame() calls.
   * Only the unicode string version must be implemented in subclasses.
   * The optional final parameter maxWidth specifies a maximum width for the label.
   * Longer labels can be shorted with an ellipsis (...). Only renderer strategies
   * that return true from SupportsBoundedSize must implement this version of th
   * method.
   */
  virtual void RenderLabel(int x[2], svtkTextProperty* tprop, svtkStdString label)
  {
    this->RenderLabel(x, tprop, svtkUnicodeString::from_utf8(label));
  }
  virtual void RenderLabel(int x[2], svtkTextProperty* tprop, svtkStdString label, int maxWidth)
  {
    this->RenderLabel(x, tprop, svtkUnicodeString::from_utf8(label), maxWidth);
  }
  virtual void RenderLabel(int x[2], svtkTextProperty* tprop, svtkUnicodeString label) = 0;
  virtual void RenderLabel(
    int x[2], svtkTextProperty* tprop, svtkUnicodeString label, int svtkNotUsed(maxWidth))
  {
    this->RenderLabel(x, tprop, label);
  }

  /**
   * Start a rendering frame. Renderer must be set.
   */
  virtual void StartFrame() {}

  /**
   * End a rendering frame.
   */
  virtual void EndFrame() {}

  /**
   * Release any graphics resources that are being consumed by this strategy.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(svtkWindow*) {}

protected:
  svtkLabelRenderStrategy();
  ~svtkLabelRenderStrategy() override;

  svtkRenderer* Renderer;
  svtkTextProperty* DefaultTextProperty;

private:
  svtkLabelRenderStrategy(const svtkLabelRenderStrategy&) = delete;
  void operator=(const svtkLabelRenderStrategy&) = delete;
};

#endif
