/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLightWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLightWidget
 * @brief   3D widget for showing a LightRepresentation
 *
 * To use this widget, one generally pairs it with a
 * svtkLightRepresentation. Various options are available in the representation
 * for controlling how the widget appears, and how it functions.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following SVTK events (i.e., it
 * watches the svtkRenderWindowInteractor for these events):
 * <pre>
 * Select and move the sphere to change the light position.
 * Select and move the cone or the line to change the focal point.
 * Right-Click and scale on the cone to change the cone angle.
 * </pre>
 *
 * @warning
 * Note that the widget can be picked even when it is "behind"
 * other actors.  This is an intended feature and not a bug.
 *
 * @warning
 * This class, and svtkLightRepresentation, are second generation SVTK widgets.
 *
 * @sa
 * svtkLightRepresentation svtkSphereWidget
 */

#ifndef svtkLightWidget_h
#define svtkLightWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkHandleWidget;
class svtkLightRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkLightWidget : public svtkAbstractWidget
{
public:
  static svtkLightWidget* New();
  svtkTypeMacro(svtkLightWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkLightRepresentation* r);

  /**
   * Return the representation as a svtkLightRepresentation.
   */
  svtkLightRepresentation* GetLightRepresentation();

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  svtkLightWidget();
  ~svtkLightWidget() override = default;

  bool WidgetActive = false;

  // These methods handle events
  static void SelectAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);
  static void ScaleAction(svtkAbstractWidget*);

private:
  svtkLightWidget(const svtkLightWidget&) = delete;
  void operator=(const svtkLightWidget&) = delete;
};

#endif
