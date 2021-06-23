/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRectilinearWipeRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRectilinearWipeRepresentation
 * @brief   represent a svtkRectilinearWipeWidget
 *
 * This class is used to represent and render a svtkRectilinearWipeWidget. To
 * use this class, you need to specify an instance of a
 * svtkImageRectilinearWipe and svtkImageActor. This provides the information
 * for this representation to construct and place itself.
 *
 * The class may be subclassed so that alternative representations can
 * be created.  The class defines an API and a default implementation that
 * the svtkRectilinearWipeWidget interacts with to render itself in the scene.
 *
 * @warning
 * The separation of the widget event handling and representation enables
 * users and developers to create new appearances for the widget. It also
 * facilitates parallel processing, where the client application handles
 * events, and remote representations of the widget are slaves to the
 * client (and do not handle events).
 *
 * @sa
 * svtkRectilinearWipeWidget svtkWidgetRepresentation svtkAbstractWidget
 */

#ifndef svtkRectilinearWipeRepresentation_h
#define svtkRectilinearWipeRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkWidgetRepresentation.h"

class svtkImageRectilinearWipe;
class svtkImageActor;
class svtkPoints;
class svtkCellArray;
class svtkPolyData;
class svtkProperty2D;
class svtkPolyDataMapper2D;
class svtkActor2D;

class SVTKINTERACTIONWIDGETS_EXPORT svtkRectilinearWipeRepresentation : public svtkWidgetRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static svtkRectilinearWipeRepresentation* New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkRectilinearWipeRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify an instance of svtkImageRectilinearWipe to manipulate.
   */
  void SetRectilinearWipe(svtkImageRectilinearWipe* wipe);
  svtkGetObjectMacro(RectilinearWipe, svtkImageRectilinearWipe);
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
   * The tolerance representing the distance to the widget (in pixels)
   * in which the cursor is considered to be on the widget, or on a
   * widget feature (e.g., a corner point or edge).
   */
  svtkSetClampMacro(Tolerance, int, 1, 10);
  svtkGetMacro(Tolerance, int);
  //@}

  //@{
  /**
   * Get the properties for the widget. This can be manipulated to set
   * different colors, line widths, etc.
   */
  svtkGetObjectMacro(Property, svtkProperty2D);
  //@}

  //@{
  /**
   * Subclasses of svtkRectilinearWipeRepresentation must implement these methods. These
   * are the methods that the widget and its representation use to
   * communicate with each other.
   */
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double eventPos[2]) override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  //@}

  // Enums define the state of the prop relative to the mouse pointer
  // position. Used by ComputeInteractionState() to communicate with the
  // widget.
  enum _InteractionState
  {
    Outside = 0,
    MovingHPane,
    MovingVPane,
    MovingCenter
  };

  //@{
  /**
   * Methods to make this class behave as a svtkProp.
   */
  void GetActors2D(svtkPropCollection*) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOverlay(svtkViewport* viewport) override;
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

protected:
  svtkRectilinearWipeRepresentation();
  ~svtkRectilinearWipeRepresentation() override;

  // Instances that this class manipulates
  svtkImageRectilinearWipe* RectilinearWipe;
  svtkImageActor* ImageActor;

  // The pick tolerance of the widget in pixels
  int Tolerance;

  // This is used to track the beginning of interaction with the prop
  double StartWipePosition[2];

  // Indicates which part of widget is currently active based on the
  // state of the instance of the svtkImageRectilinearWipe.
  int ActiveParts;

  // Geometric structure of widget
  svtkPoints* Points;   // The nine points defining the widget geometry
  svtkCellArray* Lines; // lines defining the boundary
  svtkPolyData* Wipe;
  svtkPolyDataMapper2D* WipeMapper;
  svtkActor2D* WipeActor;
  svtkProperty2D* Property;

  // These are used to track the coordinates (in display coordinate system)
  // of the mid-edge and center point of the widget
  double DP4[3];
  double DP5[3];
  double DP6[3];
  double DP7[3];
  double DP8[3];

  int Dims[3]; // Dimensions of the input image to the wipe
  int I;       // the i-j define the plane that is being displayed
  int J;

private:
  svtkRectilinearWipeRepresentation(const svtkRectilinearWipeRepresentation&) = delete;
  void operator=(const svtkRectilinearWipeRepresentation&) = delete;
};

#endif
