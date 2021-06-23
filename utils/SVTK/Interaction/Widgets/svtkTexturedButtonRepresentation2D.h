/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTexturedButtonRepresentation2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTexturedButtonRepresentation2D
 * @brief   defines a representation for a svtkButtonWidget
 *
 * This class implements one type of svtkButtonRepresentation. It changes the
 * appearance of a user-provided polydata by assigning textures according to
 * the current button state. It also provides highlighting (when hovering and
 * selecting the button) by fiddling with the actor's property. Since this is
 * a 2D version, the button is rendered in the overlay plane. Typically it is
 * positioned in display coordinates, but it can be anchored to a world
 * position so it will appear to move as the camera moves.
 *
 * To use this representation, always begin by specifying the number of
 * button states.  Then provide a polydata (the polydata should have associated
 * texture coordinates), and a list of textures cooresponding to the button
 * states. Optionally, the HoveringProperty and SelectionProperty can be
 * adjusted to obtain the appropriate appearance.
 *
 * @warning
 * There are two variants of the PlaceWidget() method. The first PlaceWidget(bds[6])
 * allows the widget to be placed in the display coordinates fixed to the overlay
 * plane. The second PlaceWidget(anchor[3],size[2]) places the widget in world space;
 * hence it will appear to move as the camera moves around the scene.
 *
 * @sa
 * svtkButtonWidget svtkButtonRepresentation svtkTexturedButtonRepresentation
 * svtkProp3DButtonRepresentation
 */

#ifndef svtkTexturedButtonRepresentation2D_h
#define svtkTexturedButtonRepresentation2D_h

#include "svtkButtonRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkProperty2D;
class svtkImageData;
class svtkTextureArray; // PIMPLd
class svtkProperty2D;
class svtkAlgorithmOutput;
class svtkBalloonRepresentation;
class svtkCoordinate;

class SVTKINTERACTIONWIDGETS_EXPORT svtkTexturedButtonRepresentation2D
  : public svtkButtonRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static svtkTexturedButtonRepresentation2D* New();

  //@{
  /**
   * Standard methods for the class.
   */
  svtkTypeMacro(svtkTexturedButtonRepresentation2D, svtkButtonRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the property to use when the button is to appear "normal"
   * i.e., the mouse pointer is not hovering or selecting the button.
   */
  virtual void SetProperty(svtkProperty2D* p);
  svtkGetObjectMacro(Property, svtkProperty2D);
  //@}

  //@{
  /**
   * Specify the property to use when the hovering over the button.
   */
  virtual void SetHoveringProperty(svtkProperty2D* p);
  svtkGetObjectMacro(HoveringProperty, svtkProperty2D);
  //@}

  //@{
  /**
   * Specify the property to use when selecting the button.
   */
  virtual void SetSelectingProperty(svtkProperty2D* p);
  svtkGetObjectMacro(SelectingProperty, svtkProperty2D);
  //@}

  //@{
  /**
   * Add the ith texture corresponding to the ith button state.
   * The parameter i should be 0<=i<NumberOfStates.
   */
  void SetButtonTexture(int i, svtkImageData* image);
  svtkImageData* GetButtonTexture(int i);
  //@}

  /**
   * Grab the underlying svtkBalloonRepresentation used to position and display
   * the button texture.
   */
  svtkBalloonRepresentation* GetBalloon() { return this->Balloon; }

  //@{
  /**
   * Provide the necessary methods to satisfy the svtkWidgetRepresentation API.
   */
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void BuildRepresentation() override;
  void Highlight(int state) override;
  //@}

  /**
   * Conventional PlaceWidget() method to satisfy the svtkWidgetRepresentation API.
   * In this version, bounds[6] specifies a rectangle in *display* coordinates
   * in which to place the button. The values for bounds[4] and bounds[5] can be
   * set to zero. Note that PlaceWidget() is typically called at the end of configuring
   * the button representation.
   */
  void PlaceWidget(double bounds[6]) override;

  /**
   * This alternative PlaceWidget() method can be used to anchor the button
   * to a 3D point. In this case, the button representation will move around
   * the screen as the camera moves around the world space. The first
   * parameter anchor[3] is the world point anchor position (attached to the
   * lower left portion of the button by default); and the size[2] parameter
   * defines a x-y box in display coordinates in which the button will
   * fit. Note that you can grab the svtkBalloonRepresentation and set an
   * offset value if the anchor point is to be elsewhere on the button.
   */
  virtual void PlaceWidget(double anchor[3], int size[2]);

  //@{
  /**
   * Provide the necessary methods to satisfy the rendering API.
   */
  void ShallowCopy(svtkProp* prop) override;
  double* GetBounds() override;
  void GetActors(svtkPropCollection* pc) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOverlay(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

protected:
  svtkTexturedButtonRepresentation2D();
  ~svtkTexturedButtonRepresentation2D() override;

  // Representing the button
  svtkBalloonRepresentation* Balloon;

  // Properties of the button
  svtkProperty2D* Property;
  svtkProperty2D* HoveringProperty;
  svtkProperty2D* SelectingProperty;
  void CreateDefaultProperties();

  // Keep track of the images (textures) associated with the N
  // states of the button.
  svtkTextureArray* TextureArray;

  // Tracking world position
  svtkCoordinate* Anchor;

private:
  svtkTexturedButtonRepresentation2D(const svtkTexturedButtonRepresentation2D&) = delete;
  void operator=(const svtkTexturedButtonRepresentation2D&) = delete;
};

#endif
