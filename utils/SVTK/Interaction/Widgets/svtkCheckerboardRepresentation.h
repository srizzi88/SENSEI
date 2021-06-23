/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCheckerboardRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCheckerboardRepresentation
 * @brief   represent the svtkCheckerboardWidget
 *
 * The svtkCheckerboardRepresentation is used to implement the representation of
 * the svtkCheckerboardWidget. The user can adjust the number of divisions in
 * each of the i-j directions in a 2D image. A frame appears around the
 * svtkImageActor with sliders along each side of the frame. The user can
 * interactively adjust the sliders to the desired number of checkerboard
 * subdivisions. The representation uses four instances of
 * svtkSliderRepresentation3D to implement itself.
 *
 * @sa
 * svtkCheckerboardWidget svtkImageCheckerboard svtkImageActor svtkSliderWidget
 * svtkRectilinearWipeWidget
 */

#ifndef svtkCheckerboardRepresentation_h
#define svtkCheckerboardRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkWidgetRepresentation.h"

class svtkImageCheckerboard;
class svtkImageActor;
class svtkSliderRepresentation3D;

class SVTKINTERACTIONWIDGETS_EXPORT svtkCheckerboardRepresentation : public svtkWidgetRepresentation
{
public:
  /**
   * Instantiate class.
   */
  static svtkCheckerboardRepresentation* New();

  //@{
  /**
   * Standard SVTK methods.
   */
  svtkTypeMacro(svtkCheckerboardRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify an instance of svtkImageCheckerboard to manipulate.
   */
  void SetCheckerboard(svtkImageCheckerboard* chkrbrd);
  svtkGetObjectMacro(Checkerboard, svtkImageCheckerboard);
  //@}

  //@{
  /**
   * Specify an instance of svtkImageActor to decorate.
   */
  void SetImageActor(svtkImageActor* imageActor);
  svtkGetObjectMacro(ImageActor, svtkImageActor);
  //@}

  //@{
  /**
   * Specify the offset of the ends of the sliders (on the boundary edges of
   * the image) from the corner of the image. The offset is expressed as a
   * normalized fraction of the border edges.
   */
  svtkSetClampMacro(CornerOffset, double, 0.0, 0.4);
  svtkGetMacro(CornerOffset, double);
  //@}

  enum
  {
    TopSlider = 0,
    RightSlider,
    BottomSlider,
    LeftSlider
  };

  /**
   * This method is invoked by the svtkCheckerboardWidget() when a value of some
   * slider has changed.
   */
  void SliderValueChanged(int sliderNum);

  //@{
  /**
   * Set and get the instances of svtkSliderRepresention used to implement this
   * representation. Normally default representations are created, but you can
   * specify the ones you want to use.
   */
  void SetTopRepresentation(svtkSliderRepresentation3D*);
  void SetRightRepresentation(svtkSliderRepresentation3D*);
  void SetBottomRepresentation(svtkSliderRepresentation3D*);
  void SetLeftRepresentation(svtkSliderRepresentation3D*);
  svtkGetObjectMacro(TopRepresentation, svtkSliderRepresentation3D);
  svtkGetObjectMacro(RightRepresentation, svtkSliderRepresentation3D);
  svtkGetObjectMacro(BottomRepresentation, svtkSliderRepresentation3D);
  svtkGetObjectMacro(LeftRepresentation, svtkSliderRepresentation3D);
  //@}

  //@{
  /**
   * Methods required by superclass.
   */
  void BuildRepresentation() override;
  void GetActors(svtkPropCollection*) override;
  void ReleaseGraphicsResources(svtkWindow* w) override;
  int RenderOverlay(svtkViewport* viewport) override;
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

protected:
  svtkCheckerboardRepresentation();
  ~svtkCheckerboardRepresentation() override;

  // Instances that this class manipulates
  svtkImageCheckerboard* Checkerboard;
  svtkImageActor* ImageActor;

  // The internal widgets for each side
  svtkSliderRepresentation3D* TopRepresentation;
  svtkSliderRepresentation3D* RightRepresentation;
  svtkSliderRepresentation3D* BottomRepresentation;
  svtkSliderRepresentation3D* LeftRepresentation;

  // The corner offset
  double CornerOffset;

  // Direction index of image actor's plane normal
  int OrthoAxis;

private:
  svtkCheckerboardRepresentation(const svtkCheckerboardRepresentation&) = delete;
  void operator=(const svtkCheckerboardRepresentation&) = delete;
};

#endif
