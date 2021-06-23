/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyle.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInteractorStyle
 * @brief   provide event-driven interface to the rendering window (defines trackball mode)
 *
 * svtkInteractorStyle is a base class implementing the majority of motion
 * control routines and defines an event driven interface to support
 * svtkRenderWindowInteractor. svtkRenderWindowInteractor implements
 * platform dependent key/mouse routing and timer control, which forwards
 * events in a neutral form to svtkInteractorStyle.
 *
 * svtkInteractorStyle implements the "joystick" style of interaction. That
 * is, holding down the mouse keys generates a stream of events that cause
 * continuous actions (e.g., rotate, translate, pan, zoom). (The class
 * svtkInteractorStyleTrackball implements a grab and move style.) The event
 * bindings for this class include the following:
 * - Keypress j / Keypress t: toggle between joystick (position sensitive) and
 * trackball (motion sensitive) styles. In joystick style, motion occurs
 * continuously as long as a mouse button is pressed. In trackball style,
 * motion occurs when the mouse button is pressed and the mouse pointer
 * moves.
 * - Keypress c / Keypress a: toggle between camera and actor modes. In
 * camera mode, mouse events affect the camera position and focal point. In
 * actor mode, mouse events affect the actor that is under the mouse pointer.
 * - Button 1: rotate the camera around its focal point (if camera mode) or
 * rotate the actor around its origin (if actor mode). The rotation is in the
 * direction defined from the center of the renderer's viewport towards
 * the mouse position. In joystick mode, the magnitude of the rotation is
 * determined by the distance the mouse is from the center of the render
 * window.
 * - Button 2: pan the camera (if camera mode) or translate the actor (if
 * actor mode). In joystick mode, the direction of pan or translation is
 * from the center of the viewport towards the mouse position. In trackball
 * mode, the direction of motion is the direction the mouse moves. (Note:
 * with 2-button mice, pan is defined as \<Shift\>-Button 1.)
 * - Button 3: zoom the camera (if camera mode) or scale the actor (if
 * actor mode). Zoom in/increase scale if the mouse position is in the top
 * half of the viewport; zoom out/decrease scale if the mouse position is in
 * the bottom half. In joystick mode, the amount of zoom is controlled by the
 * distance of the mouse pointer from the horizontal centerline of the
 * window.
 * - Keypress 3: toggle the render window into and out of stereo mode. By
 * default, red-blue stereo pairs are created. Some systems support Crystal
 * Eyes LCD stereo glasses; you have to invoke SetStereoTypeToCrystalEyes()
 * on the rendering window.
 * - Keypress e: exit the application.
 * - Keypress f: fly to the picked point
 * - Keypress p: perform a pick operation. The render window interactor has
 * an internal instance of svtkCellPicker that it uses to pick.
 * - Keypress r: reset the camera view along the current view
 * direction. Centers the actors and moves the camera so that all actors are
 * visible.
 * - Keypress s: modify the representation of all actors so that they are
 * surfaces.
 * - Keypress u: invoke the user-defined function. Typically,
 * this keypress will bring up an interactor that you can type commands in.
 * Typing u calls UserCallBack() on the svtkRenderWindowInteractor, which
 * invokes a svtkCommand::UserEvent. In other words, to define a user-defined
 * callback, just add an observer to the svtkCommand::UserEvent on the
 * svtkRenderWindowInteractor object.
 * - Keypress w: modify the representation of all actors so that they are
 * wireframe.
 *
 * svtkInteractorStyle can be subclassed to provide new interaction styles and
 * a facility to override any of the default mouse/key operations which
 * currently handle trackball or joystick styles is provided. Note that this
 * class will fire a variety of events that can be watched using an observer,
 * such as LeftButtonPressEvent, LeftButtonReleaseEvent,
 * MiddleButtonPressEvent, MiddleButtonReleaseEvent, RightButtonPressEvent,
 * RightButtonReleaseEvent, EnterEvent, LeaveEvent, KeyPressEvent,
 * KeyReleaseEvent, CharEvent, ExposeEvent, ConfigureEvent, TimerEvent,
 * MouseMoveEvent,
 *
 *
 * @sa
 * svtkInteractorStyleTrackball
 */

#ifndef svtkInteractorStyle_h
#define svtkInteractorStyle_h

#include "svtkInteractorObserver.h"
#include "svtkRenderingCoreModule.h" // For export macro

// Motion flags

#define SVTKIS_START 0
#define SVTKIS_NONE 0

#define SVTKIS_ROTATE 1
#define SVTKIS_PAN 2
#define SVTKIS_SPIN 3
#define SVTKIS_DOLLY 4
#define SVTKIS_ZOOM 5
#define SVTKIS_USCALE 6
#define SVTKIS_TIMER 7
#define SVTKIS_FORWARDFLY 8
#define SVTKIS_REVERSEFLY 9
#define SVTKIS_TWO_POINTER 10
#define SVTKIS_CLIP 11
#define SVTKIS_PICK 12                 // perform a pick at the last location
#define SVTKIS_LOAD_CAMERA_POSE 13     // iterate through saved camera poses
#define SVTKIS_POSITION_PROP 14        // adjust the position, orientation of a prop
#define SVTKIS_EXIT 15                 // call exit callback
#define SVTKIS_TOGGLE_DRAW_CONTROLS 16 // draw device controls helpers
#define SVTKIS_MENU 17                 // invoke an application menu
#define SVTKIS_GESTURE 18              // touch interaction in progress
#define SVTKIS_ENV_ROTATE 19           // rotate the renderer environment texture

#define SVTKIS_ANIM_OFF 0
#define SVTKIS_ANIM_ON 1

class svtkActor2D;
class svtkActor;
class svtkCallbackCommand;
class svtkEventData;
class svtkEventForwarderCommand;
class svtkOutlineSource;
class svtkPolyDataMapper;
class svtkProp3D;
class svtkProp;
class svtkStringArray;
class svtkTDxInteractorStyle;

class SVTKRENDERINGCORE_EXPORT svtkInteractorStyle : public svtkInteractorObserver
{
public:
  /**
   * This class must be supplied with a svtkRenderWindowInteractor wrapper or
   * parent. This class should not normally be instantiated by application
   * programmers.
   */
  static svtkInteractorStyle* New();

  svtkTypeMacro(svtkInteractorStyle, svtkInteractorObserver);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set/Get the Interactor wrapper being controlled by this object.
   * (Satisfy superclass API.)
   */
  void SetInteractor(svtkRenderWindowInteractor* interactor) override;

  /**
   * Turn on/off this interactor. Interactor styles operate a little
   * bit differently than other types of interactor observers. When
   * the SetInteractor() method is invoked, the automatically enable
   * themselves. This is a legacy requirement, and convenient for the
   * user.
   */
  void SetEnabled(int) override;

  //@{
  /**
   * If AutoAdjustCameraClippingRange is on, then before each render the
   * camera clipping range will be adjusted to "fit" the whole scene. Clipping
   * will still occur if objects in the scene are behind the camera or
   * come very close. If AutoAdjustCameraClippingRange is off, no adjustment
   * will be made per render, but the camera clipping range will still
   * be reset when the camera is reset.
   */
  svtkSetClampMacro(AutoAdjustCameraClippingRange, svtkTypeBool, 0, 1);
  svtkGetMacro(AutoAdjustCameraClippingRange, svtkTypeBool);
  svtkBooleanMacro(AutoAdjustCameraClippingRange, svtkTypeBool);
  //@}

  /**
   * When an event occurs, we must determine which Renderer the event
   * occurred within, since one RenderWindow may contain multiple
   * renderers.
   */
  void FindPokedRenderer(int, int);

  //@{
  /**
   * Some useful information for interaction
   */
  svtkGetMacro(State, int);
  //@}

  //@{
  /**
   * Set/Get timer hint
   */
  svtkGetMacro(UseTimers, svtkTypeBool);
  svtkSetMacro(UseTimers, svtkTypeBool);
  svtkBooleanMacro(UseTimers, svtkTypeBool);
  //@}

  //@{
  /**
   * If using timers, specify the default timer interval (in
   * milliseconds). Care must be taken when adjusting the timer interval from
   * the default value of 10 milliseconds--it may adversely affect the
   * interactors.
   */
  svtkSetClampMacro(TimerDuration, unsigned long, 1, 100000);
  svtkGetMacro(TimerDuration, unsigned long);
  //@}

  //@{
  /**
   * Does ProcessEvents handle observers on this class or not
   */
  svtkSetMacro(HandleObservers, svtkTypeBool);
  svtkGetMacro(HandleObservers, svtkTypeBool);
  svtkBooleanMacro(HandleObservers, svtkTypeBool);
  //@}

  /**
   * Generic event bindings can be overridden in subclasses
   */
  virtual void OnMouseMove() {}
  virtual void OnLeftButtonDown() {}
  virtual void OnLeftButtonUp() {}
  virtual void OnMiddleButtonDown() {}
  virtual void OnMiddleButtonUp() {}
  virtual void OnRightButtonDown() {}
  virtual void OnRightButtonUp() {}
  virtual void OnMouseWheelForward() {}
  virtual void OnMouseWheelBackward() {}
  virtual void OnFourthButtonDown() {}
  virtual void OnFourthButtonUp() {}
  virtual void OnFifthButtonDown() {}
  virtual void OnFifthButtonUp() {}

  /**
   * Generic 3D event bindings can be overridden in subclasses
   */
  virtual void OnMove3D(svtkEventData*) {}
  virtual void OnButton3D(svtkEventData*) {}

  /**
   * OnChar is triggered when an ASCII key is pressed. Some basic key presses
   * are handled here ('q' for Quit, 'p' for Pick, etc)
   */
  void OnChar() override;

  // OnKeyDown is triggered by pressing any key (identical to OnKeyPress()).
  // An empty implementation is provided. The behavior of this function should
  // be specified in the subclass.
  virtual void OnKeyDown() {}

  // OnKeyUp is triggered by releaseing any key (identical to OnKeyRelease()).
  // An empty implementation is provided. The behavior of this function should
  // be specified in the subclass.
  virtual void OnKeyUp() {}

  // OnKeyPress is triggered by pressing any key (identical to OnKeyDown()).
  // An empty implementation is provided. The behavior of this function should
  // be specified in the subclass.
  virtual void OnKeyPress() {}

  // OnKeyRelease is triggered by pressing any key (identical to OnKeyUp()).
  // An empty implementation is provided. The behavior of this function should
  // be specified in the subclass.
  virtual void OnKeyRelease() {}

  /**
   * These are more esoteric events, but are useful in some cases.
   */
  virtual void OnExpose() {}
  virtual void OnConfigure() {}
  virtual void OnEnter() {}
  virtual void OnLeave() {}

  /**
   * OnTimer calls Rotate, Rotate etc which should be overridden by
   * style subclasses.
   */
  virtual void OnTimer();

  /**
   * These methods for the different interactions in different modes
   * are overridden in subclasses to perform the correct motion. Since
   * they might be called from OnTimer, they do not have mouse coord parameters
   * (use interactor's GetEventPosition and GetLastEventPosition)
   */
  virtual void Rotate() {}
  virtual void Spin() {}
  virtual void Pan() {}
  virtual void Dolly() {}
  virtual void Zoom() {}
  virtual void UniformScale() {}
  virtual void EnvironmentRotate() {}

  /**
   * gesture based events
   */
  virtual void OnStartSwipe() {}
  virtual void OnSwipe() {}
  virtual void OnEndSwipe() {}
  virtual void OnStartPinch() {}
  virtual void OnPinch() {}
  virtual void OnEndPinch() {}
  virtual void OnStartRotate() {}
  virtual void OnRotate() {}
  virtual void OnEndRotate() {}
  virtual void OnStartPan() {}
  virtual void OnPan() {}
  virtual void OnEndPan() {}
  virtual void OnTap() {}
  virtual void OnLongTap() {}

  //@{
  /**
   * utility routines used by state changes
   */
  virtual void StartState(int newstate);
  virtual void StopState();
  //@}

  //@{
  /**
   * Interaction mode entry points used internally.
   */
  virtual void StartAnimate();
  virtual void StopAnimate();
  virtual void StartRotate();
  virtual void EndRotate();
  virtual void StartZoom();
  virtual void EndZoom();
  virtual void StartPan();
  virtual void EndPan();
  virtual void StartSpin();
  virtual void EndSpin();
  virtual void StartDolly();
  virtual void EndDolly();
  virtual void StartUniformScale();
  virtual void EndUniformScale();
  virtual void StartTimer();
  virtual void EndTimer();
  virtual void StartTwoPointer();
  virtual void EndTwoPointer();
  virtual void StartGesture();
  virtual void EndGesture();
  virtual void StartEnvRotate();
  virtual void EndEnvRotate();
  //@}

  /**
   * When the mouse location is updated while dragging files.
   * The argument contains the position relative to the window of the mouse
   * where the files are dropped.
   * It is called before OnDropFiles.
   */
  virtual void OnDropLocation(double* svtkNotUsed(position)) {}

  /**
   * When files are dropped on the render window.
   * The argument contains the list of file paths dropped.
   * It is called after OnDropLocation.
   */
  virtual void OnDropFiles(svtkStringArray* svtkNotUsed(filePaths)) {}

  //@{
  /**
   * When picking successfully selects an actor, this method highlights the
   * picked prop appropriately. Currently this is done by placing a bounding
   * box around a picked svtkProp3D, and using the PickColor to highlight a
   * svtkProp2D.
   */
  virtual void HighlightProp(svtkProp* prop);
  virtual void HighlightActor2D(svtkActor2D* actor2D);
  virtual void HighlightProp3D(svtkProp3D* prop3D);
  //@}

  //@{
  /**
   * Set/Get the pick color (used by default to color svtkActor2D's).
   * The color is expressed as red/green/blue values between (0.0,1.0).
   */
  svtkSetVector3Macro(PickColor, double);
  svtkGetVectorMacro(PickColor, double, 3);
  //@}

  //@{
  /**
   * Set/Get the mouse wheel motion factor. Default to 1.0. Set it to a
   * different value to emphasize or de-emphasize the action triggered by
   * mouse wheel motion.
   */
  svtkSetMacro(MouseWheelMotionFactor, double);
  svtkGetMacro(MouseWheelMotionFactor, double);
  //@}

  //@{
  /**
   * 3Dconnexion device interactor style. Initial value is a pointer to an
   * object of class svtkTdxInteractorStyleCamera.
   */
  svtkGetObjectMacro(TDxStyle, svtkTDxInteractorStyle);
  virtual void SetTDxStyle(svtkTDxInteractorStyle* tdxStyle);
  //@}

  /**
   * Called by the callback to process 3DConnexion device events.
   */
  void DelegateTDxEvent(unsigned long event, void* calldata);

protected:
  svtkInteractorStyle();
  ~svtkInteractorStyle() override;

  /**
   * Main process event method
   */
  static void ProcessEvents(
    svtkObject* object, unsigned long event, void* clientdata, void* calldata);

  // Keep track of current state
  int State;
  int AnimState;

  // Should observers be handled here, should we fire timers
  svtkTypeBool HandleObservers;
  svtkTypeBool UseTimers;
  int TimerId; // keep track of the timers that are created/destroyed

  svtkTypeBool AutoAdjustCameraClippingRange;

  // For picking and highlighting props
  svtkOutlineSource* Outline;
  svtkPolyDataMapper* OutlineMapper;
  svtkActor* OutlineActor;
  svtkRenderer* PickedRenderer;
  svtkProp* CurrentProp;
  svtkActor2D* PickedActor2D;
  int PropPicked;      // bool: prop picked?
  double PickColor[3]; // support 2D picking
  double MouseWheelMotionFactor;

  // Control the timer duration
  unsigned long TimerDuration; // in milliseconds

  // Forward events to the RenderWindowInteractor
  svtkEventForwarderCommand* EventForwarder;

  svtkTDxInteractorStyle* TDxStyle;

private:
  svtkInteractorStyle(const svtkInteractorStyle&) = delete;
  void operator=(const svtkInteractorStyle&) = delete;
};

#endif
