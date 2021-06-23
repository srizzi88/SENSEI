/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSplineWidget2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSplineWidget2
 * @brief   widget for svtkSplineRepresentation.
 *
 * svtkSplineWidget2 is the svtkAbstractWidget subclass for
 * svtkSplineRepresentation which manages the interactions with
 * svtkSplineRepresentation. This is based on svtkSplineWidget.
 * @sa
 * svtkSplineRepresentation, svtkSplineWidget2
 */

#ifndef svtkSplineWidget2_h
#define svtkSplineWidget2_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkSplineRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkSplineWidget2 : public svtkAbstractWidget
{
public:
  static svtkSplineWidget2* New();
  svtkTypeMacro(svtkSplineWidget2, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of
   * svtkProp so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkSplineRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Override superclasses' SetEnabled() method because the line
   * widget must enable its internal handle widgets.
   */
  void SetEnabled(int enabling) override;

  /**
   * Create the default widget representation if one is not set. By default,
   * this is an instance of the svtkSplineRepresentation class.
   */
  void CreateDefaultRepresentation() override;

protected:
  svtkSplineWidget2();
  ~svtkSplineWidget2() override;

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
  svtkSplineWidget2(const svtkSplineWidget2&) = delete;
  void operator=(const svtkSplineWidget2&) = delete;
};

#endif
