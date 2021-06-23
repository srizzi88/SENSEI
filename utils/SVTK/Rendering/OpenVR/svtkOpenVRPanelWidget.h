/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkOpenVRPanelWidget.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenVRPanelWidget
 * @brief   3D widget to display a panel/billboard
 *
 * Handles events for a PanelRepresentation.
 *
 * @sa
 * svtkOpenVRPanelRepresentation
 */

#ifndef svtkOpenVRPanelWidget_h
#define svtkOpenVRPanelWidget_h

#include "svtkAbstractWidget.h"
#include "svtkRenderingOpenVRModule.h" // For export macro

class svtkOpenVRPanelRepresentation;
class svtkPropMap;
class svtkProp;

class SVTKRENDERINGOPENVR_EXPORT svtkOpenVRPanelWidget : public svtkAbstractWidget
{
public:
  /**
   * Instantiate the object.
   */
  static svtkOpenVRPanelWidget* New();

  //@{
  /**
   * Standard svtkObject methods
   */
  svtkTypeMacro(svtkOpenVRPanelWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkOpenVRPanelRepresentation* rep);

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  svtkOpenVRPanelWidget();
  ~svtkOpenVRPanelWidget() override;

  // Manage the state of the widget
  int WidgetState;
  enum _WidgetState
  {
    Start = 0,
    Active
  };

  /**
   * callback
   */
  static void SelectAction3D(svtkAbstractWidget*);
  static void EndSelectAction3D(svtkAbstractWidget*);
  static void MoveAction3D(svtkAbstractWidget*);

private:
  svtkOpenVRPanelWidget(const svtkOpenVRPanelWidget&) = delete;
  void operator=(const svtkOpenVRPanelWidget&) = delete;
};
#endif
