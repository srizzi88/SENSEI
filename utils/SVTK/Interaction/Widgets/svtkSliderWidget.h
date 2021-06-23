/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSliderWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSliderWidget
 * @brief   set a value by manipulating a slider
 *
 * The svtkSliderWidget is used to set a scalar value in an application.  This
 * class assumes that a slider is moved along a 1D parameter space (e.g., a
 * spherical bead that can be moved along a tube).  Moving the slider
 * modifies the value of the widget, which can be used to set parameters on
 * other objects. Note that the actual appearance of the widget depends on
 * the specific representation for the widget.
 *
 * To use this widget, set the widget representation. The representation is
 * assumed to consist of a tube, two end caps, and a slider (the details may
 * vary depending on the particulars of the representation). Then in the
 * representation you will typically set minimum and maximum value, as well
 * as the current value. The position of the slider must also be set, as well
 * as various properties.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following SVTK events (i.e., it
 * watches the svtkRenderWindowInteractor for these events):
 * <pre>
 * If the slider bead is selected:
 *   LeftButtonPressEvent - select slider (if on slider)
 *   LeftButtonReleaseEvent - release slider (if selected)
 *   MouseMoveEvent - move slider
 * If the end caps or slider tube are selected:
 *   LeftButtonPressEvent - move (or animate) to cap or point on tube;
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's svtkWidgetEventTranslator. This class translates SVTK events
 * into the svtkSliderWidget's widget events:
 * <pre>
 *   svtkWidgetEvent::Select -- some part of the widget has been selected
 *   svtkWidgetEvent::EndSelect -- the selection process has completed
 *   svtkWidgetEvent::Move -- a request for slider motion has been invoked
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the svtkSliderWidget
 * invokes the following SVTK events on itself (which observers can listen for):
 * <pre>
 *   svtkCommand::StartInteractionEvent (on svtkWidgetEvent::Select)
 *   svtkCommand::EndInteractionEvent (on svtkWidgetEvent::EndSelect)
 *   svtkCommand::InteractionEvent (on svtkWidgetEvent::Move)
 * </pre>
 *
 */

#ifndef svtkSliderWidget_h
#define svtkSliderWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkSliderRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkSliderWidget : public svtkAbstractWidget
{
public:
  /**
   * Instantiate the class.
   */
  static svtkSliderWidget* New();

  //@{
  /**
   * Standard macros.
   */
  svtkTypeMacro(svtkSliderWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkSliderRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a svtkSliderRepresentation.
   */
  svtkSliderRepresentation* GetSliderRepresentation()
  {
    return reinterpret_cast<svtkSliderRepresentation*>(this->WidgetRep);
  }

  //@{
  /**
   * Control the behavior of the slider when selecting the tube or caps. If
   * Jump, then selecting the tube, left cap, or right cap causes the slider to
   * jump to the selection point. If the mode is Animate, the slider moves
   * towards the selection point in NumberOfAnimationSteps number of steps.
   * If Off, then the slider does not move.
   */
  svtkSetClampMacro(AnimationMode, int, AnimateOff, Animate);
  svtkGetMacro(AnimationMode, int);
  void SetAnimationModeToOff() { this->SetAnimationMode(AnimateOff); }
  void SetAnimationModeToJump() { this->SetAnimationMode(Jump); }
  void SetAnimationModeToAnimate() { this->SetAnimationMode(Animate); }
  //@}

  //@{
  /**
   * Specify the number of animation steps to take if the animation mode
   * is set to animate.
   */
  svtkSetClampMacro(NumberOfAnimationSteps, int, 1, SVTK_INT_MAX);
  svtkGetMacro(NumberOfAnimationSteps, int);
  //@}

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  svtkSliderWidget();
  ~svtkSliderWidget() override {}

  // These are the events that are handled
  static void SelectAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);
  void AnimateSlider(int selectionState);

  // Manage the state of the widget
  int WidgetState;
  enum _WidgetState
  {
    Start = 0,
    Sliding,
    Animating
  };

  int NumberOfAnimationSteps;
  int AnimationMode;
  enum AnimationState
  {
    AnimateOff,
    Jump,
    Animate
  };

private:
  svtkSliderWidget(const svtkSliderWidget&) = delete;
  void operator=(const svtkSliderWidget&) = delete;
};

#endif
