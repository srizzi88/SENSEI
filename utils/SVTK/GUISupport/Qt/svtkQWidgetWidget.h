/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkQWidgetWidget
 * @brief   3D SVTK widget for a QWidget
 *
 * This 3D widget handles events between SVTK and Qt for a QWidget placed
 * in a scene. It currently takes 6dof events as from VR controllers and
 * if they intersect the widghet it converts them to Qt events and fires
 * them off.
 */

#ifndef svtkQWidgetWidget_h
#define svtkQWidgetWidget_h

#include "svtkAbstractWidget.h"
#include "svtkGUISupportQtModule.h" // For export macro
#include <QPointF>                 // for ivar

class QWidget;
class svtkQWidgetRepresentation;

class SVTKGUISUPPORTQT_EXPORT svtkQWidgetWidget : public svtkAbstractWidget
{
  friend class svtkInteractionCallback;

public:
  /**
   * Instantiate the object.
   */
  static svtkQWidgetWidget* New();

  //@{
  /**
   * Standard svtkObject methods
   */
  svtkTypeMacro(svtkQWidgetWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkQWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkQWidgetRepresentation* rep);

  // Description:
  // Disable/Enable the widget if needed.
  // Unobserved the camera if the widget is disabled.
  void SetEnabled(int enabling) override;

  /**
   * Return the representation as a svtkQWidgetRepresentation
   */
  svtkQWidgetRepresentation* GetQWidgetRepresentation();

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Set the QWidget that will receive the events.
   */
  void SetWidget(QWidget* w);

protected:
  svtkQWidgetWidget();
  ~svtkQWidgetWidget() override;

  // Manage the state of the widget
  int WidgetState;
  enum _WidgetState
  {
    Start = 0,
    Active
  };

  QWidget* Widget;
  QPointF LastWidgetCoordinates;

  // These methods handle events
  static void SelectAction3D(svtkAbstractWidget*);
  static void EndSelectAction3D(svtkAbstractWidget*);
  static void MoveAction3D(svtkAbstractWidget*);

private:
  svtkQWidgetWidget(const svtkQWidgetWidget&) = delete;
  void operator=(const svtkQWidgetWidget&) = delete;
};

#endif
