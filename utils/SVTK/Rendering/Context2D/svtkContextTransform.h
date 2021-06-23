/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkContextTransform
 * @brief   all children of this item are transformed
 * by the svtkTransform2D of this item.
 *
 *
 * This class can be used to transform all child items of this class. The
 * default transform is the identity.
 */

#ifndef svtkContextTransform_h
#define svtkContextTransform_h

#include "svtkAbstractContextItem.h"
#include "svtkRenderingContext2DModule.h" // For export macro
#include "svtkSmartPointer.h"             // Needed for SP ivars.
#include "svtkVector.h"                   // Needed for ivars.

class svtkTransform2D;

class SVTKRENDERINGCONTEXT2D_EXPORT svtkContextTransform : public svtkAbstractContextItem
{
public:
  svtkTypeMacro(svtkContextTransform, svtkAbstractContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a svtkContextTransform object.
   */
  static svtkContextTransform* New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() override;

  /**
   * Paint event for the item, called whenever the item needs to be drawn.
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Reset the transform to the identity transformation.
   */
  virtual void Identity();

  /**
   * Translate the item by the specified amounts dx and dy in the x and y
   * directions.
   */
  virtual void Translate(float dx, float dy);

  /**
   * Scale the item by the specified amounts dx and dy in the x and y
   * directions.
   */
  virtual void Scale(float dx, float dy);

  /**
   * Rotate the item by the specified angle.
   */
  virtual void Rotate(float angle);

  /**
   * Access the svtkTransform2D that controls object transformation.
   */
  virtual svtkTransform2D* GetTransform();

  /**
   * Transforms a point to the parent coordinate system.
   */
  svtkVector2f MapToParent(const svtkVector2f& point) override;

  /**
   * Transforms a point from the parent coordinate system.
   */
  svtkVector2f MapFromParent(const svtkVector2f& point) override;

  //@{
  /**
   * The mouse button from svtkContextMouseEvent to use for panning.
   * Default is svtkContextMouseEvent::LEFT_BUTTON.
   */
  svtkSetMacro(PanMouseButton, int);
  svtkGetMacro(PanMouseButton, int);
  //@}

  //@{
  /**
   * The modifier from svtkContextMouseEvent to use for panning.
   * Default is svtkContextMouseEvent::NO_MODIFIER.
   */
  svtkSetMacro(PanModifier, int);
  svtkGetMacro(PanModifier, int);
  //@}

  //@{
  /**
   * A secondary mouse button from svtkContextMouseEvent to use for panning.
   * Default is svtkContextMouseEvent::NO_BUTTON (disabled).
   */
  svtkSetMacro(SecondaryPanMouseButton, int);
  svtkGetMacro(SecondaryPanMouseButton, int);
  //@}

  //@{
  /**
   * A secondary modifier from svtkContextMouseEvent to use for panning.
   * Default is svtkContextMouseEvent::NO_MODIFIER.
   */
  svtkSetMacro(SecondaryPanModifier, int);
  svtkGetMacro(SecondaryPanModifier, int);
  //@}

  //@{
  /**
   * The mouse button from svtkContextMouseEvent to use for panning.
   * Default is svtkContextMouseEvent::RIGHT_BUTTON.
   */
  svtkSetMacro(ZoomMouseButton, int);
  svtkGetMacro(ZoomMouseButton, int);
  //@}

  //@{
  /**
   * The modifier from svtkContextMouseEvent to use for panning.
   * Default is svtkContextMouseEvent::NO_MODIFIER.
   */
  svtkSetMacro(ZoomModifier, int);
  svtkGetMacro(ZoomModifier, int);
  //@}

  //@{
  /**
   * A secondary mouse button from svtkContextMouseEvent to use for panning.
   * Default is svtkContextMouseEvent::LEFT_BUTTON.
   */
  svtkSetMacro(SecondaryZoomMouseButton, int);
  svtkGetMacro(SecondaryZoomMouseButton, int);
  //@}

  //@{
  /**
   * A secondary modifier from svtkContextMouseEvent to use for panning.
   * Default is svtkContextMouseEvent::SHIFT_MODIFIER.
   */
  svtkSetMacro(SecondaryZoomModifier, int);
  svtkGetMacro(SecondaryZoomModifier, int);
  //@}

  //@{
  /**
   * Whether to zoom on mouse wheels. Default is true.
   */
  svtkSetMacro(ZoomOnMouseWheel, bool);
  svtkGetMacro(ZoomOnMouseWheel, bool);
  svtkBooleanMacro(ZoomOnMouseWheel, bool);
  //@}

  //@{
  /**
   * Whether to pan in the Y direction on mouse wheels. Default is false.
   */
  svtkSetMacro(PanYOnMouseWheel, bool);
  svtkGetMacro(PanYOnMouseWheel, bool);
  svtkBooleanMacro(PanYOnMouseWheel, bool);
  //@}

  /**
   * Returns true if the transform is interactive, false otherwise.
   */
  bool Hit(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse press event. Keep track of zoom anchor position.
   */
  bool MouseButtonPressEvent(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse move event. Perform pan or zoom as specified by the mouse bindings.
   */
  bool MouseMoveEvent(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse wheel event. Perform pan or zoom as specified by mouse bindings.
   */
  bool MouseWheelEvent(const svtkContextMouseEvent& mouse, int delta) override;

protected:
  svtkContextTransform();
  ~svtkContextTransform() override;

  svtkSmartPointer<svtkTransform2D> Transform;

  int PanMouseButton;
  int PanModifier;
  int ZoomMouseButton;
  int ZoomModifier;
  int SecondaryPanMouseButton;
  int SecondaryPanModifier;
  int SecondaryZoomMouseButton;
  int SecondaryZoomModifier;

  bool ZoomOnMouseWheel;
  bool PanYOnMouseWheel;

  svtkVector2f ZoomAnchor;

private:
  svtkContextTransform(const svtkContextTransform&) = delete;
  void operator=(const svtkContextTransform&) = delete;
};

#endif // svtkContextTransform_h
