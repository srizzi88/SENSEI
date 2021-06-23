/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCaptionWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCaptionWidget
 * @brief   widget for placing a caption (text plus leader)
 *
 * This class provides support for interactively placing a caption on the 2D
 * overlay plane. A caption is defined by some text with a leader (e.g.,
 * arrow) that points from the text to a point in the scene. The caption is
 * represented by a svtkCaptionRepresentation. It uses the event bindings of
 * its superclass (svtkBorderWidget) to control the placement of the text, and
 * adds the ability to move the attachment point around. In addition, when
 * the caption text is selected, the widget emits a ActivateEvent that
 * observers can watch for. This is useful for opening GUI dialogoues to
 * adjust font characteristics, etc. (Please see the superclass for a
 * description of event bindings.)
 *
 * Note that this widget extends the behavior of its superclass
 * svtkBorderWidget. The end point of the leader can be selected and
 * moved around with an internal svtkHandleWidget.
 *
 * @sa
 * svtkBorderWidget svtkTextWidget
 */

#ifndef svtkCaptionWidget_h
#define svtkCaptionWidget_h

#include "svtkBorderWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkCaptionRepresentation;
class svtkCaptionActor2D;
class svtkHandleWidget;
class svtkPointHandleRepresentation3D;
class svtkCaptionAnchorCallback;

class SVTKINTERACTIONWIDGETS_EXPORT svtkCaptionWidget : public svtkBorderWidget
{
public:
  /**
   * Instantiate this class.
   */
  static svtkCaptionWidget* New();

  //@{
  /**
   * Standard SVTK class methods.
   */
  svtkTypeMacro(svtkCaptionWidget, svtkBorderWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Override superclasses' SetEnabled() method because the caption leader
   * has its own dedicated widget.
   */
  void SetEnabled(int enabling) override;

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkCaptionRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  //@{
  /**
   * Specify a svtkCaptionActor2D to manage. This is convenient, alternative
   * method to SetRepresentation(). It internally create a svtkCaptionRepresentation
   * and then invokes svtkCaptionRepresentation::SetCaptionActor2D().
   */
  void SetCaptionActor2D(svtkCaptionActor2D* capActor);
  svtkCaptionActor2D* GetCaptionActor2D();
  //@}

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  svtkCaptionWidget();
  ~svtkCaptionWidget() override;

  // Handles callbacks from the anchor point
  svtkCaptionAnchorCallback* AnchorCallback;

  // Widget for the anchor point
  svtkHandleWidget* HandleWidget;

  // Special callbacks for the anchor interaction
  void StartAnchorInteraction();
  void AnchorInteraction();
  void EndAnchorInteraction();

  friend class svtkCaptionAnchorCallback;

private:
  svtkCaptionWidget(const svtkCaptionWidget&) = delete;
  void operator=(const svtkCaptionWidget&) = delete;
};

#endif
