/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCameraWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCameraWidget
 * @brief   2D widget for saving a series of camera views
 *
 * This class provides support for interactively saving a series of camera
 * views into an interpolated path (using svtkCameraInterpolator). To use the
 * class start by specifying a camera to interpolate, and then simply start
 * recording by hitting the "record" button, manipulate the camera (by using
 * an interactor, direct scripting, or any other means), and then save the
 * camera view. Repeat this process to record a series of views.  The user
 * can then play back interpolated camera views using the
 * svtkCameraInterpolator.
 *
 * @sa
 * svtkBorderWidget svtkCameraInterpolator
 */

#ifndef svtkCameraWidget_h
#define svtkCameraWidget_h

#include "svtkBorderWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkCameraRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkCameraWidget : public svtkBorderWidget
{
public:
  /**
   * Instantiate this class.
   */
  static svtkCameraWidget* New();

  //@{
  /**
   * Standard SVTK class methods.
   */
  svtkTypeMacro(svtkCameraWidget, svtkBorderWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkCameraRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  svtkCameraWidget();
  ~svtkCameraWidget() override;

  /**
   * When selecting the interior of this widget, special operations occur
   * (i.e., adding a camera view, deleting a path, animating a path). Thus
   * this methods overrides the superclasses' method.
   */
  void SelectRegion(double eventPos[2]) override;

private:
  svtkCameraWidget(const svtkCameraWidget&) = delete;
  void operator=(const svtkCameraWidget&) = delete;
};

#endif
