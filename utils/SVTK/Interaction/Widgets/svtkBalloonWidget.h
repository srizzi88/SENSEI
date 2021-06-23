/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBalloonWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBalloonWidget
 * @brief   popup text balloons above instance of svtkProp when hovering occurs
 *
 * The svtkBalloonWidget is used to popup text and/or an image when the mouse
 * hovers over an instance of svtkProp. The widget keeps track of
 * (svtkProp,svtkBalloon) pairs (where the internal svtkBalloon class is defined
 * by a pair of svtkStdString and svtkImageData), and when the mouse stops
 * moving for a user-specified period of time over the svtkProp, then the
 * svtkBalloon is drawn nearby the svtkProp. Note that an instance of
 * svtkBalloonRepresentation is used to draw the balloon.
 *
 * To use this widget, specify an instance of svtkBalloonWidget and a
 * representation (e.g., svtkBalloonRepresentation). Then list all instances
 * of svtkProp, a text string, and/or an instance of svtkImageData to be
 * associated with each svtkProp. (Note that you can specify both text and an
 * image, or just one or the other.) You may also wish to specify the hover
 * delay (i.e., set in the superclass svtkHoverWidget).
 *
 * @par Event Bindings:
 * By default, the widget observes the following SVTK events (i.e., it
 * watches the svtkRenderWindowInteractor for these events):
 * <pre>
 *   MouseMoveEvent - occurs when mouse is moved in render window.
 *   TimerEvent - occurs when the time between events (e.g., mouse move)
 *                is greater than TimerDuration.
 *   KeyPressEvent - when the "Enter" key is pressed after the balloon appears,
 *                   a callback is activated (e.g., WidgetActivateEvent).
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's svtkWidgetEventTranslator. This class translates SVTK events
 * into the svtkBalloonWidget's widget events:
 * <pre>
 *   svtkWidgetEvent::Move -- start the timer
 *   svtkWidgetEvent::TimedOut -- when hovering occurs,
 *   svtkWidgetEvent::SelectAction -- activate any callbacks associated
 *                                   with the balloon.
 * </pre>
 *
 * @par Event Bindings:
 * This widget invokes the following SVTK events on itself (which observers
 * can listen for):
 * <pre>
 *   svtkCommand::TimerEvent (when hovering is determined to occur)
 *   svtkCommand::EndInteractionEvent (after a hover has occurred and the
 *                                    mouse begins moving again).
 *   svtkCommand::WidgetActivateEvent (when the balloon is selected with a
 *                                    keypress).
 * </pre>
 *
 * @sa
 * svtkAbstractWidget
 */

#ifndef svtkBalloonWidget_h
#define svtkBalloonWidget_h

#include "svtkHoverWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkBalloonRepresentation;
class svtkProp;
class svtkAbstractPropPicker;
class svtkStdString;
class svtkPropMap;
class svtkImageData;

class SVTKINTERACTIONWIDGETS_EXPORT svtkBalloonWidget : public svtkHoverWidget
{
public:
  /**
   * Instantiate this class.
   */
  static svtkBalloonWidget* New();

  //@{
  /**
   * Standard methods for a SVTK class.
   */
  svtkTypeMacro(svtkBalloonWidget, svtkHoverWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * The method for activating and deactivating this widget. This method
   * must be overridden because it performs special timer-related operations.
   */
  void SetEnabled(int) override;

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkBalloonRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a svtkBalloonRepresentation.
   */
  svtkBalloonRepresentation* GetBalloonRepresentation()
  {
    return reinterpret_cast<svtkBalloonRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  //@{
  /**
   * Add and remove text and/or an image to be associated with a svtkProp. You
   * may add one or both of them.
   */
  void AddBalloon(svtkProp* prop, svtkStdString* str, svtkImageData* img);
  void AddBalloon(svtkProp* prop, const char* str, svtkImageData* img);
  void AddBalloon(svtkProp* prop, const char* str) // for wrapping
  {
    this->AddBalloon(prop, str, nullptr);
  }
  void RemoveBalloon(svtkProp* prop);
  //@}

  //@{
  /**
   * Methods to retrieve the information associated with each svtkProp (i.e.,
   * the information that makes up each balloon). A nullptr will be returned if
   * the svtkProp does not exist, or if a string or image have not been
   * associated with the specified svtkProp.
   */
  const char* GetBalloonString(svtkProp* prop);
  svtkImageData* GetBalloonImage(svtkProp* prop);
  //@}

  //@{
  /**
   * Update the balloon string or image. If the specified prop does not exist,
   * then nothing is added not changed.
   */
  void UpdateBalloonString(svtkProp* prop, const char* str);
  void UpdateBalloonImage(svtkProp* prop, svtkImageData* image);
  //@}

  /**
   * Return the current svtkProp that is being hovered over. Note that the
   * value may be nullptr (if hovering over nothing or the mouse is moving).
   */
  virtual svtkProp* GetCurrentProp() { return this->CurrentProp; }

  //@{
  /**
   * Set/Get the object used to perform pick operations. Since the
   * svtkBalloonWidget operates on svtkProps, the picker must be a subclass of
   * svtkAbstractPropPicker. (Note: if not specified, an instance of
   * svtkPropPicker is used.)
   */
  void SetPicker(svtkAbstractPropPicker*);
  svtkGetObjectMacro(Picker, svtkAbstractPropPicker);
  //@}

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

protected:
  svtkBalloonWidget();
  ~svtkBalloonWidget() override;

  // This class implements the method called from its superclass.
  int SubclassEndHoverAction() override;
  int SubclassHoverAction() override;

  // Classes for managing balloons
  svtkPropMap* PropMap; // PIMPL'd map of (svtkProp,svtkStdString)

  // Support for picking
  svtkAbstractPropPicker* Picker;

  // The svtkProp that is being hovered over (which may be nullptr)
  svtkProp* CurrentProp;

private:
  svtkBalloonWidget(const svtkBalloonWidget&) = delete;
  void operator=(const svtkBalloonWidget&) = delete;
};

#endif
