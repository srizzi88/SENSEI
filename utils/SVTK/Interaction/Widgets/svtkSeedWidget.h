/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSeedWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSeedWidget
 * @brief   place multiple seed points
 *
 * The svtkSeedWidget is used to placed multiple seed points in the scene.
 * The seed points can be used for operations like connectivity, segmentation,
 * and region growing.
 *
 * To use this widget, specify an instance of svtkSeedWidget and a
 * representation (a subclass of svtkSeedRepresentation). The widget is
 * implemented using multiple instances of svtkHandleWidget which can be used
 * to position the seed points (after they are initially placed). The
 * representations for these handle widgets are provided by the
 * svtkSeedRepresentation.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following SVTK events (i.e., it
 * watches the svtkRenderWindowInteractor for these events):
 * <pre>
 *   LeftButtonPressEvent - add a point or select a handle (i.e., seed)
 *   RightButtonPressEvent - finish adding the seeds
 *   MouseMoveEvent - move a handle (i.e., seed)
 *   LeftButtonReleaseEvent - release the selected handle (seed)
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's svtkWidgetEventTranslator. This class translates SVTK events
 * into the svtkSeedWidget's widget events:
 * <pre>
 *   svtkWidgetEvent::AddPoint -- add one point; depending on the state
 *                               it may the first or second point added. Or,
 *                               if near handle, select handle.
 *   svtkWidgetEvent::Completed -- finished adding seeds.
 *   svtkWidgetEvent::Move -- move the second point or handle depending on the state.
 *   svtkWidgetEvent::EndSelect -- the handle manipulation process has completed.
 * </pre>
 *
 * @par Event Bindings:
 * This widget invokes the following SVTK events on itself (which observers
 * can listen for):
 * <pre>
 *   svtkCommand::StartInteractionEvent (beginning to interact)
 *   svtkCommand::EndInteractionEvent (completing interaction)
 *   svtkCommand::InteractionEvent (moving after selecting something)
 *   svtkCommand::PlacePointEvent (after point is positioned;
 *                                call data includes handle id (0,1))
 *   svtkCommand::DeletePointEvent (before point is deleted;
 *                                call data includes handle id (0,1))
 * </pre>
 *
 * @sa
 * svtkHandleWidget svtkSeedRepresentation
 */

#ifndef svtkSeedWidget_h
#define svtkSeedWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkHandleRepresentation;
class svtkHandleWidget;
class svtkSeedList;
class svtkSeedRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkSeedWidget : public svtkAbstractWidget
{
public:
  /**
   * Instantiate this class.
   */
  static svtkSeedWidget* New();

  //@{
  /**
   * Standard methods for a SVTK class.
   */
  svtkTypeMacro(svtkSeedWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * The method for activating and deactivating this widget. This method
   * must be overridden because it is a composite widget and does more than
   * its superclasses' svtkAbstractWidget::SetEnabled() method.
   */
  void SetEnabled(int) override;

  /**
   * Set the current renderer. This method also propagates to all the child
   * handle widgets, if any exist
   */
  void SetCurrentRenderer(svtkRenderer*) override;

  /**
   * Set the interactor. This method also propagates to all the child
   * handle widgets, if any exist
   */
  void SetInteractor(svtkRenderWindowInteractor*) override;

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkSeedRepresentation* rep)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(rep));
  }

  /**
   * Return the representation as a svtkSeedRepresentation.
   */
  svtkSeedRepresentation* GetSeedRepresentation()
  {
    return reinterpret_cast<svtkSeedRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Methods to change the whether the widget responds to interaction.
   * Overridden to pass the state to component widgets.
   */
  void SetProcessEvents(svtkTypeBool) override;

  /**
   * Method to be called when the seed widget should stop responding to
   * the place point interaction. The seed widget, when defined allows you
   * place seeds by clicking on the render window. Use this method to
   * indicate that you would like to stop placing seeds interactively. If
   * you'd like the widget to stop responding to *any* user interaction
   * simply disable event processing by the widget by calling
   * widget->ProcessEventsOff()
   */
  virtual void CompleteInteraction();

  /**
   * Method to be called when the seed widget should start responding
   * to the interaction.
   */
  virtual void RestartInteraction();

  /**
   * Use this method to programmatically create a new handle. In interactive
   * mode, (when the widget is in the PlacingSeeds state) this method is
   * automatically invoked. The method returns the handle created.
   * A valid seed representation must exist for the widget to create a new
   * handle.
   */
  virtual svtkHandleWidget* CreateNewHandle();

  /**
   * Delete the nth seed.
   */
  void DeleteSeed(int n);

  /**
   * Get the nth seed
   */
  svtkHandleWidget* GetSeed(int n);

  //@{
  /**
   * Get the widget state.
   */
  svtkGetMacro(WidgetState, int);
  //@}

  // The state of the widget

  enum
  {
    Start = 1,
    PlacingSeeds = 2,
    PlacedSeeds = 4,
    MovingSeed = 8
  };

protected:
  svtkSeedWidget();
  ~svtkSeedWidget() override;

  int WidgetState;

  // Callback interface to capture events when
  // placing the widget.
  static void AddPointAction(svtkAbstractWidget*);
  static void CompletedAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);
  static void DeleteAction(svtkAbstractWidget*);

  // The positioning handle widgets
  svtkSeedList* Seeds;

  // Manipulating or defining ?
  int Defining;

private:
  svtkSeedWidget(const svtkSeedWidget&) = delete;
  void operator=(const svtkSeedWidget&) = delete;
};

#endif
