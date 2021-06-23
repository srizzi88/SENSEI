/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageOrthoPlanes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageOrthoPlanes
 * @brief   Connect three svtkImagePlaneWidgets together
 *
 * svtkImageOrthoPlanes is an event observer class that listens to the
 * events from three svtkImagePlaneWidgets and keeps their orientations
 * and scales synchronized.
 * @sa
 * svtkImagePlaneWidget
 * @par Thanks:
 * Thanks to Atamai Inc. for developing and contributing this class.
 */

#ifndef svtkImageOrthoPlanes_h
#define svtkImageOrthoPlanes_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkObject.h"

class svtkImagePlaneWidget;
class svtkTransform;
class svtkMatrix4x4;

class SVTKINTERACTIONWIDGETS_EXPORT svtkImageOrthoPlanes : public svtkObject
{
public:
  static svtkImageOrthoPlanes* New();
  svtkTypeMacro(svtkImageOrthoPlanes, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * You must set three planes for the widget.
   */
  void SetPlane(int i, svtkImagePlaneWidget* imagePlaneWidget);
  svtkImagePlaneWidget* GetPlane(int i);
  //@}

  /**
   * Reset the planes to original scale, rotation, and location.
   */
  void ResetPlanes();

  /**
   * Get the transform for the planes.
   */
  svtkTransform* GetTransform() { return this->Transform; }

  /**
   * A public method to be used only by the event callback.
   */
  void HandlePlaneEvent(svtkImagePlaneWidget* imagePlaneWidget);

protected:
  svtkImageOrthoPlanes();
  ~svtkImageOrthoPlanes() override;

  void HandlePlaneRotation(svtkImagePlaneWidget* imagePlaneWidget, int indexOfModifiedPlane);
  void HandlePlanePush(svtkImagePlaneWidget* imagePlaneWidget, int indexOfModifiedPlane);
  void HandlePlaneTranslate(svtkImagePlaneWidget* imagePlaneWidget, int indexOfModifiedPlane);
  void HandlePlaneScale(svtkImagePlaneWidget* imagePlaneWidget, int indexOfModifiedPlane);

  void SetTransformMatrix(
    svtkMatrix4x4* matrix, svtkImagePlaneWidget* currentImagePlane, int indexOfModifiedPlane);

  void GetBounds(double bounds[3]);

  // The plane definitions prior to any rotations or scales
  double Origin[3][3];
  double Point1[3][3];
  double Point2[3][3];

  // The current position and orientation of the bounding box with
  // respect to the origin.
  svtkTransform* Transform;

  // An array to hold the planes
  svtkImagePlaneWidget** Planes;

  // The number of planes.
  int NumberOfPlanes;

  // The observer tags for these planes
  long* ObserverTags;

private:
  svtkImageOrthoPlanes(const svtkImageOrthoPlanes&) = delete;
  void operator=(const svtkImageOrthoPlanes&) = delete;
};

#endif
