/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyLineWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolyLineWidget
 * @brief   widget for svtkPolyLineRepresentation.
 *
 * svtkPolyLineWidget is the svtkAbstractWidget subclass for
 * svtkPolyLineRepresentation which manages the interactions with
 * svtkPolyLineRepresentation. This is based on svtkPolyLineWidget.
 *
 * This widget allows the creation of a polyline interactively by adding or removing points
 * based on mouse position and a modifier key.
 *
 * - ctrl+click inserts a new point on the selected line
 * - shift+click deletes the selected point
 * - alt+click adds a new point anywhere depending on last selected point.
 *   If the first point is selected, the new point is added at the beginning,
 *   else it is added at the end.
 *
 * @sa
 * svtkPolyLineRepresentation, svtkPolyLineWidget
 */

#ifndef svtkPolyLineWidget_h
#define svtkPolyLineWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkPolyLineRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkPolyLineWidget : public svtkAbstractWidget
{
public:
  static svtkPolyLineWidget* New();
  svtkTypeMacro(svtkPolyLineWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of
   * svtkProp so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkPolyLineRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Create the default widget representation if one is not set. By default,
   * this is an instance of the svtkPolyLineRepresentation class.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Override superclasses' SetEnabled() method because the line
   * widget must enable its internal handle widgets.
   */
  void SetEnabled(int enabling) override;

protected:
  svtkPolyLineWidget();
  ~svtkPolyLineWidget() override;

  int WidgetState;
  enum _WidgetState
  {
    Start = 0,
    Active
  };

  // These methods handle events
  static void SelectAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);
  static void TranslateAction(svtkAbstractWidget*);
  static void ScaleAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);

  svtkCallbackCommand* KeyEventCallbackCommand;
  static void ProcessKeyEvents(svtkObject*, unsigned long, void*, void*);

private:
  svtkPolyLineWidget(const svtkPolyLineWidget&) = delete;
  void operator=(const svtkPolyLineWidget&) = delete;
};

#endif
