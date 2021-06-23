/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCheckerboardWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCheckerboardWidget
 * @brief   interactively set the number of divisions in 2D image checkerboard
 *
 * The svtkCheckerboardWidget is used to interactively control an instance of
 * svtkImageCheckerboard (and an associated svtkImageActor used to display the
 * checkerboard). The user can adjust the number of divisions in each of the
 * i-j directions in a 2D image. A frame appears around the svtkImageActor
 * with sliders along each side of the frame. The user can interactively
 * adjust the sliders to the desired number of checkerboard subdivisions.
 *
 * To use this widget, specify an instance of svtkImageCheckerboard and an
 * instance of svtkImageActor. By default, the widget responds to the
 * following events:
 * <pre>
 * If the slider bead is selected:
 *   LeftButtonPressEvent - select slider (if on slider)
 *   LeftButtonReleaseEvent - release slider
 *   MouseMoveEvent - move slider
 * If the end caps or slider tube of a slider are selected:
 *   LeftButtonPressEvent - jump (or animate) to cap or point on tube;
 * </pre>
 * It is possible to change these event bindings. Please refer to the
 * documentation for svtkSliderWidget for more information. Advanced users may
 * directly access and manipulate the sliders by obtaining the instances of
 * svtkSliderWidget composing the svtkCheckerboard widget.
 *
 * @sa
 * svtkImageCheckerboard svtkImageActor svtkSliderWidget svtkRectilinearWipeWidget
 */

#ifndef svtkCheckerboardWidget_h
#define svtkCheckerboardWidget_h

#include "svtkAbstractWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkCheckerboardRepresentation;
class svtkSliderWidget;

class SVTKINTERACTIONWIDGETS_EXPORT svtkCheckerboardWidget : public svtkAbstractWidget
{
public:
  /**
   * Instantiate this class.
   */
  static svtkCheckerboardWidget* New();

  //@{
  /**
   * Standard methods for a SVTK class.
   */
  svtkTypeMacro(svtkCheckerboardWidget, svtkAbstractWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * The method for activating and deactivating this widget. This method
   * must be overridden because it is a composite widget and does more than
   * its superclasses' svtkAbstractWidget::SetEnabled() method.
   */
  void SetEnabled(int) override;

  /**
   * Specify an instance of svtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkCheckerboardRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a svtkCheckerboardRepresentation.
   */
  svtkCheckerboardRepresentation* GetCheckerboardRepresentation()
  {
    return reinterpret_cast<svtkCheckerboardRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  svtkCheckerboardWidget();
  ~svtkCheckerboardWidget() override;

  // The four slider widgets
  svtkSliderWidget* TopSlider;
  svtkSliderWidget* RightSlider;
  svtkSliderWidget* BottomSlider;
  svtkSliderWidget* LeftSlider;

  // Callback interface
  void StartCheckerboardInteraction();
  void CheckerboardInteraction(int sliderNum);
  void EndCheckerboardInteraction();

  friend class svtkCWCallback;

private:
  svtkCheckerboardWidget(const svtkCheckerboardWidget&) = delete;
  void operator=(const svtkCheckerboardWidget&) = delete;
};

#endif
