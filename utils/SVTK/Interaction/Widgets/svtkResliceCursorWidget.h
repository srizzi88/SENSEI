/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResliceCursorWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkResliceCursorWidget
 * @brief   represent a reslice cursor
 *
 * This class represents a reslice cursor that can be used to
 * perform interactive thick slab MPR's through data. It
 * consists of two cross sectional hairs, with an optional thickness. The
 * hairs may have a hole in the center. These may be translated or rotated
 * independent of each other in the view. The result is used to reslice
 * the data along these cross sections. This allows the user to perform
 * multi-planar thin or thick reformat of the data on an image view, rather
 * than a 3D view. The class internally uses svtkImageSlabReslice
 * or svtkImageReslice depending on the modes in svtkResliceCursor to
 * do its reslicing. The slab thickness is set interactively from
 * the widget. The slab resolution (ie the number of blend points) is
 * set as the minimum spacing along any dimension from the dataset.
 * @sa
 * svtkImageSlabReslice svtkResliceCursorLineRepresentation
 * svtkResliceCursor
 */

#ifndef svtkResliceCursorWidget_h
#define svtkResliceCursorWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkResliceCursorRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkResliceCursorWidget : public svtkAbstractWidget
{
public:
  /**
   * Instantiate this class.
   */
  static svtkResliceCursorWidget* New();

  //@{
  /**
   * Standard SVTK class macros.
   */
  svtkTypeMacro(svtkResliceCursorWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkResliceCursorRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a svtkResliceCursorRepresentation.
   */
  svtkResliceCursorRepresentation* GetResliceCursorRepresentation()
  {
    return reinterpret_cast<svtkResliceCursorRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Methods for activiating this widget. This implementation extends the
   * superclasses' in order to resize the widget handles due to a render
   * start event.
   */
  void SetEnabled(int) override;

  //@{
  /**
   * Also perform window level ?
   */
  svtkSetMacro(ManageWindowLevel, svtkTypeBool);
  svtkGetMacro(ManageWindowLevel, svtkTypeBool);
  svtkBooleanMacro(ManageWindowLevel, svtkTypeBool);
  //@}

  /**
   * Events
   */
  enum
  {
    WindowLevelEvent = 1055,
    ResliceAxesChangedEvent,
    ResliceThicknessChangedEvent,
    ResetCursorEvent
  };

  /**
   * Reset the cursor back to its initial state
   */
  virtual void ResetResliceCursor();

protected:
  svtkResliceCursorWidget();
  ~svtkResliceCursorWidget() override;

  // These are the callbacks for this widget
  static void SelectAction(svtkAbstractWidget*);
  static void RotateAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);
  static void ResizeThicknessAction(svtkAbstractWidget*);
  static void EndResizeThicknessAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);
  static void ResetResliceCursorAction(svtkAbstractWidget*);

  // helper methods for cursor management
  void SetCursor(int state) override;

  // Start Window Level
  void StartWindowLevel();

  // Invoke the appropriate event based on state
  void InvokeAnEvent();

  // Manage the state of the widget
  int WidgetState;
  enum _WidgetState
  {
    Start = 0,
    Active
  };

  // Keep track whether key modifier key is pressed
  int ModifierActive;
  svtkTypeBool ManageWindowLevel;

private:
  svtkResliceCursorWidget(const svtkResliceCursorWidget&) = delete;
  void operator=(const svtkResliceCursorWidget&) = delete;
};

#endif
