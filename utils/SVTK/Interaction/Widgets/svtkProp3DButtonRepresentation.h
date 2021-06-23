/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProp3DButtonRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProp3DButtonRepresentation
 * @brief   defines a representation for a svtkButtonWidget
 *
 * This class implements one type of svtkButtonRepresentation. Each button
 * state can be represented with a separate instance of svtkProp3D. Thus
 * buttons can be represented with svtkActor, svtkImageActor, volumes (e.g.,
 * svtkVolume) and/or any other svtkProp3D. Also, the class invokes events when
 * highlighting occurs (i.e., hovering, selecting) so that appropriate action
 * can be taken to highlight the button (if desired).
 *
 * To use this representation, always begin by specifying the number of
 * button states.  Then provide, for each state, an instance of svtkProp3D.
 *
 * This widget representation uses the conventional placement method. The
 * button is placed inside the bounding box defined by PlaceWidget by translating
 * and scaling the svtkProp3D to fit (each svtkProp3D is transformed). Therefore,
 * you must define the number of button states and each state (i.e., svtkProp3D)
 * prior to calling svtkPlaceWidget.
 *
 * @sa
 * svtkButtonWidget svtkButtonRepresentation svtkButtonSource svtkEllipticalButtonSource
 * svtkRectangularButtonSource
 */

#ifndef svtkProp3DButtonRepresentation_h
#define svtkProp3DButtonRepresentation_h

#include "svtkButtonRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkPropPicker;
class svtkProp3D;
class svtkProp3DFollower;
class svtkPropArray; // PIMPLd

class SVTKINTERACTIONWIDGETS_EXPORT svtkProp3DButtonRepresentation : public svtkButtonRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static svtkProp3DButtonRepresentation* New();

  //@{
  /**
   * Standard methods for instances of the class.
   */
  svtkTypeMacro(svtkProp3DButtonRepresentation, svtkButtonRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Add the ith texture corresponding to the ith button state.
   * The parameter i should be (0 <= i < NumberOfStates).
   */
  void SetButtonProp(int i, svtkProp3D* prop);
  svtkProp3D* GetButtonProp(int i);
  //@}

  //@{
  /**
   * Specify whether the button should always face the camera. If enabled,
   * the button reorients itself towards the camera as the camera moves.
   */
  svtkSetMacro(FollowCamera, svtkTypeBool);
  svtkGetMacro(FollowCamera, svtkTypeBool);
  svtkBooleanMacro(FollowCamera, svtkTypeBool);
  //@}

  /**
   * Extend the svtkButtonRepresentation::SetState() method.
   */
  void SetState(int state) override;

  //@{
  /**
   * Provide the necessary methods to satisfy the svtkWidgetRepresentation API.
   */
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void BuildRepresentation() override;
  //@}

  /**
   * This method positions (translates and scales the props) into the
   * bounding box specified. Note all the button props are scaled.
   */
  void PlaceWidget(double bounds[6]) override;

  //@{
  /**
   * Provide the necessary methods to satisfy the rendering API.
   */
  void ShallowCopy(svtkProp* prop) override;
  double* GetBounds() override;
  void GetActors(svtkPropCollection* pc) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderVolumetricGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

protected:
  svtkProp3DButtonRepresentation();
  ~svtkProp3DButtonRepresentation() override;

  // The current svtkProp3D used to represent the button
  svtkProp3D* CurrentProp;

  // Follow the camera if requested
  svtkProp3DFollower* Follower;
  svtkTypeBool FollowCamera;

  // Keep track of the props associated with the N
  // states of the button. This is a PIMPLd stl map.
  svtkPropArray* PropArray;

  // For picking the button
  svtkPropPicker* Picker;

private:
  svtkProp3DButtonRepresentation(const svtkProp3DButtonRepresentation&) = delete;
  void operator=(const svtkProp3DButtonRepresentation&) = delete;
};

#endif
