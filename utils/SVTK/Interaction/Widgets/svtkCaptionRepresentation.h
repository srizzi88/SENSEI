/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCaptionRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCaptionRepresentation
 * @brief   represents svtkCaptionWidget in the scene
 *
 * This class represents svtkCaptionWidget. A caption is defined by some text
 * with a leader (e.g., arrow) that points from the text to a point in the
 * scene. The caption is defined by an instance of svtkCaptionActor2D. It uses
 * the event bindings of its superclass (svtkBorderWidget) to control the
 * placement of the text, and adds the ability to move the attachment point
 * around. In addition, when the caption text is selected, the widget emits a
 * ActivateEvent that observers can watch for. This is useful for opening GUI
 * dialogoues to adjust font characteristics, etc. (Please see the superclass
 * for a description of event bindings.)
 *
 * Note that this widget extends the behavior of its superclass
 * svtkBorderRepresentation.
 *
 * @sa
 * svtkCaptionWidget svtkBorderWidget svtkBorderRepresentation svtkCaptionActor
 */

#ifndef svtkCaptionRepresentation_h
#define svtkCaptionRepresentation_h

#include "svtkBorderRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkRenderer;
class svtkCaptionActor2D;
class svtkConeSource;
class svtkPointHandleRepresentation3D;

class SVTKINTERACTIONWIDGETS_EXPORT svtkCaptionRepresentation : public svtkBorderRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static svtkCaptionRepresentation* New();

  //@{
  /**
   * Standard SVTK class methods.
   */
  svtkTypeMacro(svtkCaptionRepresentation, svtkBorderRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the position of the anchor (i.e., the point that the caption is anchored to).
   * Note that the position should be specified in world coordinates.
   */
  void SetAnchorPosition(double pos[3]);
  void GetAnchorPosition(double pos[3]);
  //@}

  //@{
  /**
   * Specify the svtkCaptionActor2D to manage. If not specified, then one
   * is automatically created.
   */
  void SetCaptionActor2D(svtkCaptionActor2D* captionActor);
  svtkGetObjectMacro(CaptionActor2D, svtkCaptionActor2D);
  //@}

  //@{
  /**
   * Set and get the instances of svtkPointHandleRepresention3D used to implement this
   * representation. Normally default representations are created, but you can
   * specify the ones you want to use.
   */
  void SetAnchorRepresentation(svtkPointHandleRepresentation3D*);
  svtkGetObjectMacro(AnchorRepresentation, svtkPointHandleRepresentation3D);
  //@}

  /**
   * Satisfy the superclasses API.
   */
  void BuildRepresentation() override;
  void GetSize(double size[2]) override
  {
    size[0] = 2.0;
    size[1] = 2.0;
  }

  //@{
  /**
   * These methods are necessary to make this representation behave as
   * a svtkProp.
   */
  void GetActors2D(svtkPropCollection*) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOverlay(svtkViewport*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  //@{
  /**
   * Set/Get the factor that controls the overall size of the fonts
   * of the caption when the text actor's ScaledText is OFF
   */
  svtkSetClampMacro(FontFactor, double, 0.1, 10.0);
  svtkGetMacro(FontFactor, double);
  //@}

protected:
  svtkCaptionRepresentation();
  ~svtkCaptionRepresentation() override;

  // the text to manage
  svtkCaptionActor2D* CaptionActor2D;
  svtkConeSource* CaptionGlyph;

  int PointWidgetState;
  int DisplayAttachmentPoint[2];
  double FontFactor;

  // Internal representation for the anchor
  svtkPointHandleRepresentation3D* AnchorRepresentation;

  // Check and adjust boundaries according to the size of the caption text
  virtual void AdjustCaptionBoundary();

private:
  svtkCaptionRepresentation(const svtkCaptionRepresentation&) = delete;
  void operator=(const svtkCaptionRepresentation&) = delete;
};

#endif
