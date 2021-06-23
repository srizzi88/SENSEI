/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSphereHandleRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSphereHandleRepresentation
 * @brief   A spherical rendition of point in 3D space
 *
 *
 * This class is a concrete implementation of svtkHandleRepresentation. It
 * renders handles as spherical blobs in 3D space.
 *
 * @sa
 * svtkHandleRepresentation svtkHandleWidget svtkSphereSource
 */

#ifndef svtkSphereHandleRepresentation_h
#define svtkSphereHandleRepresentation_h

#include "svtkHandleRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkSphereSource.h"             // Needed for delegation to sphere

class svtkSphereSource;
class svtkProperty;
class svtkActor;
class svtkPolyDataMapper;
class svtkCellPicker;

class SVTKINTERACTIONWIDGETS_EXPORT svtkSphereHandleRepresentation : public svtkHandleRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static svtkSphereHandleRepresentation* New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkSphereHandleRepresentation, svtkHandleRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  using svtkHandleRepresentation::Translate;

  //@{
  /**
   * Set the position of the point in world and display coordinates. Note
   * that if the position is set outside of the bounding box, it will be
   * clamped to the boundary of the bounding box. This method overloads
   * the superclasses' SetWorldPosition() and SetDisplayPosition() in
   * order to set the focal point of the cursor properly.
   */
  void SetWorldPosition(double p[3]) override;
  void SetDisplayPosition(double p[3]) override;
  //@}

  //@{
  /**
   * If translation mode is on, as the widget is moved the bounding box,
   * shadows, and cursor are all translated simultaneously as the point moves
   * (i.e., the left and middle mouse buttons act the same).  Otherwise, only
   * the cursor focal point moves, which is constrained by the bounds of the
   * point representation. (Note that the bounds can be scaled up using the
   * right mouse button.)
   */
  svtkSetMacro(TranslationMode, svtkTypeBool);
  svtkGetMacro(TranslationMode, svtkTypeBool);
  svtkBooleanMacro(TranslationMode, svtkTypeBool);
  //@}

  void SetSphereRadius(double);
  double GetSphereRadius();

  //@{
  /**
   * Set/Get the handle properties when unselected and selected.
   */
  void SetProperty(svtkProperty*);
  void SetSelectedProperty(svtkProperty*);
  svtkGetObjectMacro(Property, svtkProperty);
  svtkGetObjectMacro(SelectedProperty, svtkProperty);
  //@}

  //@{
  /**
   * Set the "hot spot" size; i.e., the region around the focus, in which the
   * motion vector is used to control the constrained sliding action. Note the
   * size is specified as a fraction of the length of the diagonal of the
   * point widget's bounding box.
   */
  svtkSetClampMacro(HotSpotSize, double, 0.0, 1.0);
  svtkGetMacro(HotSpotSize, double);
  //@}

  /**
   * Overload the superclasses SetHandleSize() method to update internal
   * variables.
   */
  void SetHandleSize(double size) override;

  //@{
  /**
   * Methods to make this class properly act like a svtkWidgetRepresentation.
   */
  double* GetBounds() SVTK_SIZEHINT(6) override;
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double eventPos[2]) override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void PlaceWidget(double bounds[6]) override;
  //@}

  //@{
  /**
   * Methods to make this class behave as a svtkProp.
   */
  void ShallowCopy(svtkProp* prop) override;
  void DeepCopy(svtkProp* prop) override;
  void GetActors(svtkPropCollection*) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  void Highlight(int highlight) override;

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

  /**
   * Override to ensure that the internal actor's visibility is consistent with
   * this representation's visibility. Inconsistency between the two would cause
   * issues in picking logic which relies on individual view prop visibility to
   * determine whether the prop is pickable.
   */
  void SetVisibility(svtkTypeBool visible) override;

protected:
  svtkSphereHandleRepresentation();
  ~svtkSphereHandleRepresentation() override;

  // the cursor3D
  svtkActor* Actor;
  svtkPolyDataMapper* Mapper;
  svtkSphereSource* Sphere;
  // void Highlight(int highlight);

  // Do the picking
  svtkCellPicker* CursorPicker;
  double LastPickPosition[3];
  double LastEventPosition[2];

  // Methods to manipulate the cursor
  int ConstraintAxis;
  void Translate(const double* p1, const double* p2) override;
  void Scale(const double* p1, const double* p2, const double eventPos[2]);
  void MoveFocus(const double* p1, const double* p2);
  void SizeBounds();

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  svtkProperty* Property;
  svtkProperty* SelectedProperty;
  void CreateDefaultProperties();

  // The size of the hot spot.
  double HotSpotSize;
  int WaitingForMotion;
  int WaitCount;

  // Current handle sized (may reflect scaling)
  double CurrentHandleSize;

  // Control how translation works
  svtkTypeBool TranslationMode;

private:
  svtkSphereHandleRepresentation(const svtkSphereHandleRepresentation&) = delete;
  void operator=(const svtkSphereHandleRepresentation&) = delete;
};

#endif
