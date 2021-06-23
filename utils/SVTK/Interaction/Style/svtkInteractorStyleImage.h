/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInteractorStyleImage
 * @brief   interactive manipulation of the camera specialized for images
 *
 * svtkInteractorStyleImage allows the user to interactively manipulate
 * (rotate, pan, zoom etc.) the camera. svtkInteractorStyleImage is specially
 * designed to work with images that are being rendered with
 * svtkImageActor. Several events are overloaded from its superclass
 * svtkInteractorStyle, hence the mouse bindings are different. (The bindings
 * keep the camera's view plane normal perpendicular to the x-y plane.) In
 * summary the mouse events for 2D image interaction are as follows:
 * - Left Mouse button triggers window level events
 * - CTRL Left Mouse spins the camera around its view plane normal
 * - SHIFT Left Mouse pans the camera
 * - CTRL SHIFT Left Mouse dollys (a positional zoom) the camera
 * - Middle mouse button pans the camera
 * - Right mouse button dollys the camera.
 * - SHIFT Right Mouse triggers pick events
 *
 * If SetInteractionModeToImageSlicing() is called, then some of the mouse
 * events are changed as follows:
 * - CTRL Left Mouse slices through the image
 * - SHIFT Middle Mouse slices through the image
 * - CTRL Right Mouse spins the camera
 *
 * If SetInteractionModeToImage3D() is called, then some of the mouse
 * events are changed as follows:
 * - SHIFT Left Mouse rotates the camera for oblique slicing
 * - SHIFT Middle Mouse slices through the image
 * - CTRL Right Mouse also slices through the image
 *
 * In all modes, the following key bindings are in effect:
 * - R Reset the Window/Level
 * - X Reset to a sagittal view
 * - Y Reset to a coronal view
 * - Z Reset to an axial view
 *
 * Note that the renderer's actors are not moved; instead the camera is moved.
 *
 * @sa
 * svtkInteractorStyle svtkInteractorStyleTrackballActor
 * svtkInteractorStyleJoystickCamera svtkInteractorStyleJoystickActor
 */

#ifndef svtkInteractorStyleImage_h
#define svtkInteractorStyleImage_h

#include "svtkInteractionStyleModule.h" // For export macro
#include "svtkInteractorStyleTrackballCamera.h"

// Motion flags

#define SVTKIS_WINDOW_LEVEL 1024
#define SVTKIS_SLICE 1025

// Style flags

#define SVTKIS_IMAGE2D 2
#define SVTKIS_IMAGE3D 3
#define SVTKIS_IMAGE_SLICING 4

class svtkImageProperty;

class SVTKINTERACTIONSTYLE_EXPORT svtkInteractorStyleImage : public svtkInteractorStyleTrackballCamera
{
public:
  static svtkInteractorStyleImage* New();
  svtkTypeMacro(svtkInteractorStyleImage, svtkInteractorStyleTrackballCamera);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Some useful information for handling window level
   */
  svtkGetVector2Macro(WindowLevelStartPosition, int);
  svtkGetVector2Macro(WindowLevelCurrentPosition, int);
  //@}

  //@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove() override;
  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  void OnMiddleButtonDown() override;
  void OnMiddleButtonUp() override;
  void OnRightButtonDown() override;
  void OnRightButtonUp() override;
  //@}

  /**
   * Override the "fly-to" (f keypress) for images.
   */
  void OnChar() override;

  // These methods for the different interactions in different modes
  // are overridden in subclasses to perform the correct motion. Since
  // they might be called from OnTimer, they do not have mouse coord parameters
  // (use interactor's GetEventPosition and GetLastEventPosition)
  virtual void WindowLevel();
  virtual void Pick();
  virtual void Slice();

  // Interaction mode entry points used internally.
  virtual void StartWindowLevel();
  virtual void EndWindowLevel();
  virtual void StartPick();
  virtual void EndPick();
  virtual void StartSlice();
  virtual void EndSlice();

  //@{
  /**
   * Set/Get current mode to 2D or 3D.  The default is 2D.  In 3D mode,
   * it is possible to rotate the camera to view oblique slices.  In Slicing
   * mode, it is possible to slice through the data, but not to generate oblique
   * views by rotating the camera.
   */
  svtkSetClampMacro(InteractionMode, int, SVTKIS_IMAGE2D, SVTKIS_IMAGE_SLICING);
  svtkGetMacro(InteractionMode, int);
  void SetInteractionModeToImage2D() { this->SetInteractionMode(SVTKIS_IMAGE2D); }
  void SetInteractionModeToImage3D() { this->SetInteractionMode(SVTKIS_IMAGE3D); }
  void SetInteractionModeToImageSlicing() { this->SetInteractionMode(SVTKIS_IMAGE_SLICING); }
  //@}

  //@{
  /**
   * Set the orientations that will be used when the X, Y, or Z
   * keys are pressed.  See SetImageOrientation for more information.
   */
  svtkSetVector3Macro(XViewRightVector, double);
  svtkGetVector3Macro(XViewRightVector, double);
  svtkSetVector3Macro(XViewUpVector, double);
  svtkGetVector3Macro(XViewUpVector, double);
  svtkSetVector3Macro(YViewRightVector, double);
  svtkGetVector3Macro(YViewRightVector, double);
  svtkSetVector3Macro(YViewUpVector, double);
  svtkGetVector3Macro(YViewUpVector, double);
  svtkSetVector3Macro(ZViewRightVector, double);
  svtkGetVector3Macro(ZViewRightVector, double);
  svtkSetVector3Macro(ZViewUpVector, double);
  svtkGetVector3Macro(ZViewUpVector, double);
  //@}

  /**
   * Set the view orientation, in terms of the horizontal and
   * vertical directions of the computer screen.  The first
   * vector gives the direction that will correspond to moving
   * horizontally left-to-right across the screen, and the
   * second vector gives the direction that will correspond to
   * moving bottom-to-top up the screen.  This method changes
   * the position of the camera to provide the desired view.
   */
  void SetImageOrientation(const double leftToRight[3], const double bottomToTop[3]);

  /**
   * Set the image to use for WindowLevel interaction.
   * Any images for which the Pickable flag is off are ignored.
   * Images are counted back-to-front, so 0 is the rearmost image.
   * Negative values can be used to count front-to-back, so -1 is
   * the frontmost image, -2 is the image behind that one, etc.
   * The default is to use the frontmost image for interaction.
   * If the specified image does not exist, then no WindowLevel
   * interaction will take place.
   */
  virtual void SetCurrentImageNumber(int i);
  int GetCurrentImageNumber() { return this->CurrentImageNumber; }

  /**
   * Get the current image property, which is set when StartWindowLevel
   * is called immediately before StartWindowLevelEvent is generated.
   * This is the image property of the topmost svtkImageSlice in the
   * renderer or nullptr if no image actors are present.
   */
  svtkImageProperty* GetCurrentImageProperty() { return this->CurrentImageProperty; }

protected:
  svtkInteractorStyleImage();
  ~svtkInteractorStyleImage() override;

  int WindowLevelStartPosition[2];
  int WindowLevelCurrentPosition[2];
  double WindowLevelInitial[2];
  svtkImageProperty* CurrentImageProperty;
  int CurrentImageNumber;

  int InteractionMode;
  double XViewRightVector[3];
  double XViewUpVector[3];
  double YViewRightVector[3];
  double YViewUpVector[3];
  double ZViewRightVector[3];
  double ZViewUpVector[3];

private:
  svtkInteractorStyleImage(const svtkInteractorStyleImage&) = delete;
  void operator=(const svtkInteractorStyleImage&) = delete;
};

#endif
