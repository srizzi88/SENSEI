/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTexturedButtonRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTexturedButtonRepresentation
 * @brief   defines a representation for a svtkButtonWidget
 *
 * This class implements one type of svtkButtonRepresentation. It changes the
 * appearance of a user-provided polydata by assigning textures according to the
 * current button state. It also provides highlighting (when hovering and
 * selecting the button) by fiddling with the actor's property.
 *
 * To use this representation, always begin by specifying the number of
 * button states.  Then provide a polydata (the polydata should have associated
 * texture coordinates), and a list of textures cooresponding to the button
 * states. Optionally, the HoveringProperty and SelectionProperty can be
 * adjusted to obtain the appropriate appearance.
 *
 * This widget representation has two placement methods. The conventional
 * PlaceWidget() method is used to locate the textured button inside of a
 * user-specified bounding box (note that the button geometry is uniformly
 * scaled to fit, thus two of the three dimensions can be "large" and the
 * third used to perform the scaling). However this PlaceWidget() method will
 * align the geometry within x-y-z oriented bounds. To further control the
 * placement, use the additional PlaceWidget(scale,point,normal) method. This
 * scales the geometry, places its center at the specified point position,
 * and orients the geometry's z-direction parallel to the specified normal.
 * This can be used to attach "sticky notes" or "sticky buttons" to the
 * surface of objects.
 *
 * @sa
 * svtkButtonWidget svtkButtonRepresentation svtkButtonSource svtkEllipticalButtonSource
 * svtkRectangularButtonSource
 */

#ifndef svtkTexturedButtonRepresentation_h
#define svtkTexturedButtonRepresentation_h

#include "svtkButtonRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkCellPicker;
class svtkActor;
class svtkProperty;
class svtkImageData;
class svtkTextureArray; // PIMPLd
class svtkPolyData;
class svtkPolyDataMapper;
class svtkAlgorithmOutput;
class svtkTexture;
class svtkFollower;

class SVTKINTERACTIONWIDGETS_EXPORT svtkTexturedButtonRepresentation : public svtkButtonRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static svtkTexturedButtonRepresentation* New();

  //@{
  /**
   * Standard methods for instances of the class.
   */
  svtkTypeMacro(svtkTexturedButtonRepresentation, svtkButtonRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Set/Get the polydata which defines the button geometry.
   */
  void SetButtonGeometry(svtkPolyData* pd);
  void SetButtonGeometryConnection(svtkAlgorithmOutput* algOutput);
  svtkPolyData* GetButtonGeometry();
  //@}

  //@{
  /**
   * Specify whether the button should always face the camera. If enabled,
   * the button rotates as the camera moves.
   */
  svtkSetMacro(FollowCamera, svtkTypeBool);
  svtkGetMacro(FollowCamera, svtkTypeBool);
  svtkBooleanMacro(FollowCamera, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the property to use when the button is to appear "normal"
   * i.e., the mouse pointer is not hovering or selecting the button.
   */
  virtual void SetProperty(svtkProperty* p);
  svtkGetObjectMacro(Property, svtkProperty);
  //@}

  //@{
  /**
   * Specify the property to use when the hovering over the button.
   */
  virtual void SetHoveringProperty(svtkProperty* p);
  svtkGetObjectMacro(HoveringProperty, svtkProperty);
  //@}

  //@{
  /**
   * Specify the property to use when selecting the button.
   */
  virtual void SetSelectingProperty(svtkProperty* p);
  svtkGetObjectMacro(SelectingProperty, svtkProperty);
  //@}

  //@{
  /**
   * Add the ith texture corresponding to the ith button state.
   * The parameter i should be (0 <= i < NumberOfStates).
   */
  void SetButtonTexture(int i, svtkImageData* image);
  svtkImageData* GetButtonTexture(int i);
  //@}

  /**
   * Alternative method for placing a button at a given position (defined by
   * point[3]); at a given orientation (normal[3], where the z-axis of the
   * button geometry is parallel to the normal); and scaled by the scale
   * parameter. This method can bs used to attach "sticky notes" or "sticky
   * buttons" to objects. A great way to attach interactive meta-data to 3D
   * actors.
   */
  virtual void PlaceWidget(double scale, double point[3], double normal[3]);

  //@{
  /**
   * Provide the necessary methods to satisfy the svtkWidgetRepresentation API.
   */
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override;
  void Highlight(int state) override;
  //@}

  //@{
  /**
   * Provide the necessary methods to satisfy the rendering API.
   */
  void ShallowCopy(svtkProp* prop) override;
  double* GetBounds() override;
  void GetActors(svtkPropCollection* pc) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

protected:
  svtkTexturedButtonRepresentation();
  ~svtkTexturedButtonRepresentation() override;

  // Representing the button
  svtkActor* Actor;
  svtkFollower* Follower;
  svtkPolyDataMapper* Mapper;
  svtkTexture* Texture;

  // Camera
  svtkTypeBool FollowCamera;

  // Properties of the button
  svtkProperty* Property;
  svtkProperty* HoveringProperty;
  svtkProperty* SelectingProperty;
  void CreateDefaultProperties();

  // Keep track of the images (textures) associated with the N
  // states of the button. This is a PIMPLd stl map.
  svtkTextureArray* TextureArray;

  // For picking the button
  svtkCellPicker* Picker;

private:
  svtkTexturedButtonRepresentation(const svtkTexturedButtonRepresentation&) = delete;
  void operator=(const svtkTexturedButtonRepresentation&) = delete;
};

#endif
