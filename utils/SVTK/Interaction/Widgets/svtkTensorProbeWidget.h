/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTensorProbeWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTensorProbeWidget
 * @brief   a widget to probe tensors on a polyline
 *
 * The class is used to probe tensors on a trajectory. The representation
 * (svtkTensorProbeRepresentation) is free to choose its own method of
 * rendering the tensors. For instance svtkEllipsoidTensorProbeRepresentation
 * renders the tensors as ellipsoids. The interactions of the widget are
 * controlled by the left mouse button. A left click on the tensor selects
 * it. It can dragged around the trajectory to probe the tensors on it.
 *
 * For instance dragging the ellipsoid around with
 * svtkEllipsoidTensorProbeRepresentation will manifest itself with the
 * ellipsoid shape changing as needed along the trajectory.
 */

#ifndef svtkTensorProbeWidget_h
#define svtkTensorProbeWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkTensorProbeRepresentation;
class svtkPolyData;

class SVTKINTERACTIONWIDGETS_EXPORT svtkTensorProbeWidget : public svtkAbstractWidget
{
public:
  /**
   * Instantiate this class.
   */
  static svtkTensorProbeWidget* New();

  //@{
  /**
   * Standard SVTK class macros.
   */
  svtkTypeMacro(svtkTensorProbeWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkTensorProbeRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a svtkTensorProbeRepresentation.
   */
  svtkTensorProbeRepresentation* GetTensorProbeRepresentation()
  {
    return reinterpret_cast<svtkTensorProbeRepresentation*>(this->WidgetRep);
  }

  /**
   * See svtkWidgetRepresentation for details.
   */
  void CreateDefaultRepresentation() override;

protected:
  svtkTensorProbeWidget();
  ~svtkTensorProbeWidget() override;

  // 1 when the probe has been selected, for instance when dragging it around
  int Selected;

  int LastEventPosition[2];

  // Callback interface to capture events and respond
  static void SelectAction(svtkAbstractWidget*);
  static void MoveAction(svtkAbstractWidget*);
  static void EndSelectAction(svtkAbstractWidget*);

private:
  svtkTensorProbeWidget(const svtkTensorProbeWidget&) = delete;
  void operator=(const svtkTensorProbeWidget&) = delete;
};

#endif
