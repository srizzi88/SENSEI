/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWidgetSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWidgetSet
 * @brief   Synchronize a collection on svtkWidgets drawn on different renderwindows using the
 * Callback - Dispatch Action mechanism.
 *
 *
 * The class synchronizes a set of svtkAbstractWidget(s). Widgets typically
 * invoke "Actions" that drive the geometry/behaviour of their representations
 * in response to interactor events. Interactor interactions on a render window
 * are mapped into "Callbacks" by the widget, from which "Actions" are
 * dispatched to the entire set. This architecture allows us to tie widgets
 * existing in different render windows together. For instance a HandleWidget
 * might exist on the sagittal view. Moving it around should update the
 * representations of the corresponding handle widget that lies on the axial
 * and coronal and volume views as well.
 *
 * @par User API:
 * A user would use this class as follows.
 * \code
 * svtkWidgetSet *set = svtkWidgetSet::New();
 * svtkParallelopipedWidget *w1 = svtkParallelopipedWidget::New();
 * set->AddWidget(w1);
 * w1->SetInteractor(axialRenderWindow->GetInteractor());
 * svtkParallelopipedWidget *w2 = svtkParallelopipedWidget::New();
 * set->AddWidget(w2);
 * w2->SetInteractor(coronalRenderWindow->GetInteractor());
 * svtkParallelopipedWidget *w3 = svtkParallelopipedWidget::New();
 * set->AddWidget(w3);
 * w3->SetInteractor(sagittalRenderWindow->GetInteractor());
 * set->SetEnabled(1);
 * \endcode
 *
 * @par Motivation:
 * The motivation for this class is really to provide a usable API to tie
 * together multiple widgets of the same kind. To enable this, subclasses
 * of svtkAbstractWidget, must be written as follows:
 *   They will generally have callback methods mapped to some user
 * interaction such as:
 * \code
 * this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent,
 *                         svtkEvent::NoModifier, 0, 0, nullptr,
 *                         svtkPaintbrushWidget::BeginDrawStrokeEvent,
 *                         this, svtkPaintbrushWidget::BeginDrawCallback);
 * \endcode
 *   The callback invoked when the left button is pressed looks like:
 * \code
 * void svtkPaintbrushWidget::BeginDrawCallback(svtkAbstractWidget *w)
 * {
 *   svtkPaintbrushWidget *self = svtkPaintbrushWidget::SafeDownCast(w);
 *   self->WidgetSet->DispatchAction(self, &svtkPaintbrushWidget::BeginDrawAction);
 * }
 * \endcode
 *   The actual code for handling the drawing is written in the BeginDrawAction
 * method.
 * \code
 * void svtkPaintbrushWidget::BeginDrawAction( svtkPaintbrushWidget *dispatcher)
 * {
 * // Do stuff to draw...
 * // Here dispatcher is the widget that was interacted with, the one that
 * // dispatched an action to all the other widgets in its group. You may, if
 * // necessary find it helpful to get parameters from it.
 * //   For instance for a ResizeAction:
 * //     if (this != dispatcher)
 * //       {
 * //       double *newsize = dispatcher->GetRepresentation()->GetSize();
 * //       this->WidgetRep->SetSize(newsize);
 * //       }
 * //     else
 * //       {
 * //       this->WidgetRep->IncrementSizeByDelta();
 * //       }
 * }
 * \endcode
 *
 * @warning
 * Actions are always dispatched first to the activeWidget, the one calling
 * the set, and then to the other widgets in the set.
 *
 */

#ifndef svtkWidgetSet_h
#define svtkWidgetSet_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkObject.h"
#include <vector> // Required for vector

class svtkAbstractWidget;

// Pointer to a member function that takes a svtkAbstractWidget (the active
// child) and another svtkAbstractWidget (the widget to dispatch an action)
// to. All "Action" functions in a widget must conform to this signature.
template <class TWidget>
struct ActionFunction
{
  typedef void (TWidget::*TActionFunctionPointer)(TWidget* dispatcher);
};

class SVTKINTERACTIONWIDGETS_EXPORT svtkWidgetSet : public svtkObject
{
public:
  /**
   * Instantiate this class.
   */
  static svtkWidgetSet* New();

  //@{
  /**
   * Standard methods for a SVTK class.
   */
  svtkTypeMacro(svtkWidgetSet, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Method for activating and deactivating all widgets in the group.
   */
  virtual void SetEnabled(svtkTypeBool);
  svtkBooleanMacro(Enabled, svtkTypeBool);
  //@}

  /**
   * Add a widget to the set.
   */
  void AddWidget(svtkAbstractWidget*);

  /**
   * Remove a widget from the set
   */
  void RemoveWidget(svtkAbstractWidget*);

  /**
   * Get number of widgets in the set.
   */
  unsigned int GetNumberOfWidgets();

  /**
   * Get the Nth widget in the set.
   */
  svtkAbstractWidget* GetNthWidget(unsigned int);

  // TODO: Move this to the protected section. The class svtkAbstractWidget
  //       should be a friend of this class.
  typedef std::vector<svtkAbstractWidget*> WidgetContainerType;
  typedef WidgetContainerType::iterator WidgetIteratorType;
  typedef WidgetContainerType::const_iterator WidgetConstIteratorType;
  WidgetContainerType Widget;

  //@{
  /**
   * Dispatch an "Action" to every widget in this set. This is meant to be
   * invoked from a "Callback" in a widget.
   */
  template <class TWidget>
  void DispatchAction(
    TWidget* caller, typename ActionFunction<TWidget>::TActionFunctionPointer action)
  {
    // Dispatch action to the caller first.
    for (WidgetIteratorType it = this->Widget.begin(); it != this->Widget.end(); ++it)
    {
      TWidget* w = static_cast<TWidget*>(*it);
      if (caller == w)
      {
        ((*w).*(action))(caller);
        break;
      }
    }
    //@}

    // Dispatch action to all other widgets
    for (WidgetIteratorType it = this->Widget.begin(); it != this->Widget.end(); ++it)
    {
      TWidget* w = static_cast<TWidget*>(*it);
      if (caller != w)
        ((*w).*(action))(caller);
    }
  }

protected:
  svtkWidgetSet();
  ~svtkWidgetSet() override;

private:
  svtkWidgetSet(const svtkWidgetSet&) = delete;
  void operator=(const svtkWidgetSet&) = delete;
};

#endif
