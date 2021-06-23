/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSphereRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSphereRepresentation
 * @brief   a class defining the representation for the svtkSphereWidget2
 *
 * This class is a concrete representation for the svtkSphereWidget2. It
 * represents a sphere with an optional handle.  Through interaction with the
 * widget, the sphere can be arbitrarily positioned and scaled in 3D space;
 * and the handle can be moved on the surface of the sphere. Typically the
 * svtkSphereWidget2/svtkSphereRepresentation are used to position a sphere for
 * the purpose of extracting, cutting or clipping data; or the handle is
 * moved on the sphere to position a light or camera.
 *
 * To use this representation, you normally use the PlaceWidget() method
 * to position the widget at a specified region in space. It is also possible
 * to set the center of the sphere, a radius, and/or a handle position.
 *
 * @warning
 * Note that the representation is overconstrained in that the center and radius
 * of the sphere can be defined, this information plus the handle direction defines
 * the geometry of the representation. Alternatively, the user may specify the center
 * of the sphere plus the handle position.
 *
 * @warning
 * This class, and svtkSphereWidget2, are second generation SVTK widgets. An
 * earlier version of this functionality was defined in the class
 * svtkSphereWidget.
 *
 * @sa
 * svtkSphereWidget2 svtkSphereWidget
 */

#ifndef svtkSphereRepresentation_h
#define svtkSphereRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkSphereSource.h"             // Needed for fast access to the sphere source
#include "svtkWidgetRepresentation.h"

class svtkActor;
class svtkPolyDataMapper;
class svtkSphere;
class svtkSphereSource;
class svtkCellPicker;
class svtkProperty;
class svtkPolyData;
class svtkPoints;
class svtkPolyDataAlgorithm;
class svtkTransform;
class svtkDoubleArray;
class svtkMatrix4x4;
class svtkTextMapper;
class svtkActor2D;
class svtkTextProperty;
class svtkLineSource;
class svtkCursor3D;

#define SVTK_SPHERE_OFF 0
#define SVTK_SPHERE_WIREFRAME 1
#define SVTK_SPHERE_SURFACE 2

class SVTKINTERACTIONWIDGETS_EXPORT svtkSphereRepresentation : public svtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static svtkSphereRepresentation* New();

  //@{
  /**
   * Standard methods for type information and to print out the contents of the class.
   */
  svtkTypeMacro(svtkSphereRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  // Used to manage the state of the widget
  enum
  {
    Outside = 0,
    MovingHandle,
    OnSphere,
    Translating,
    Scaling
  };

  //@{
  /**
   * Set the representation (i.e., appearance) of the sphere. Different
   * representations are useful depending on the application.
   */
  svtkSetClampMacro(Representation, int, SVTK_SPHERE_OFF, SVTK_SPHERE_SURFACE);
  svtkGetMacro(Representation, int);
  void SetRepresentationToOff() { this->SetRepresentation(SVTK_SPHERE_OFF); }
  void SetRepresentationToWireframe() { this->SetRepresentation(SVTK_SPHERE_WIREFRAME); }
  void SetRepresentationToSurface() { this->SetRepresentation(SVTK_SPHERE_SURFACE); }
  //@}

  /**
   * Set/Get the resolution of the sphere in the theta direction.
   */
  void SetThetaResolution(int r) { this->SphereSource->SetThetaResolution(r); }
  int GetThetaResolution() { return this->SphereSource->GetThetaResolution(); }

  /**
   * Set/Get the resolution of the sphere in the phi direction.
   */
  void SetPhiResolution(int r) { this->SphereSource->SetPhiResolution(r); }
  int GetPhiResolution() { return this->SphereSource->GetPhiResolution(); }

  /**
   * Set/Get the center position of the sphere. Note that this may
   * adjust the direction from the handle to the center, as well as
   * the radius of the sphere.
   */
  void SetCenter(double c[3]);
  void SetCenter(double x, double y, double z)
  {
    double c[3];
    c[0] = x;
    c[1] = y;
    c[2] = z;
    this->SetCenter(c);
  }
  double* GetCenter() SVTK_SIZEHINT(3) { return this->SphereSource->GetCenter(); }
  void GetCenter(double xyz[3]) { this->SphereSource->GetCenter(xyz); }

  /**
   * Set/Get the radius of sphere. Default is 0.5. Note that this may
   * modify the position of the handle based on the handle direction.
   */
  void SetRadius(double r);
  double GetRadius() { return this->SphereSource->GetRadius(); }

  //@{
  /**
   * The handle sits on the surface of the sphere and may be moved around
   * the surface by picking (left mouse) and then moving. The position
   * of the handle can be retrieved, this is useful for positioning cameras
   * and lights. By default, the handle is turned off.
   */
  svtkSetMacro(HandleVisibility, svtkTypeBool);
  svtkGetMacro(HandleVisibility, svtkTypeBool);
  svtkBooleanMacro(HandleVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the position of the handle. Note that this may adjust the radius
   * of the sphere and the handle direction.
   */
  void SetHandlePosition(double handle[3]);
  void SetHandlePosition(double x, double y, double z)
  {
    double p[3];
    p[0] = x;
    p[1] = y;
    p[2] = z;
    this->SetHandlePosition(p);
  }
  svtkGetVector3Macro(HandlePosition, double);
  //@}

  //@{
  /**
   * Set/Get the direction vector of the handle relative to the center of
   * the sphere. Setting the direction may affect the position of the handle
   * but will not affect the radius or position of the sphere.
   */
  void SetHandleDirection(double dir[3]);
  void SetHandleDirection(double dx, double dy, double dz)
  {
    double d[3];
    d[0] = dx;
    d[1] = dy;
    d[2] = dz;
    this->SetHandleDirection(d);
  }
  svtkGetVector3Macro(HandleDirection, double);
  //@}

  //@{
  /**
   * Enable/disable a label that displays the location of the handle in
   * spherical coordinates (radius,theta,phi). The two angles, theta and
   * phi, are displayed in degrees. Note that phi is measured from the
   * north pole down towards the equator; and theta is the angle around
   * the north/south axis.
   */
  svtkSetMacro(HandleText, svtkTypeBool);
  svtkGetMacro(HandleText, svtkTypeBool);
  svtkBooleanMacro(HandleText, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable/disable a radial line segment that joins the center of the
   * outer sphere and the handle.
   */
  svtkSetMacro(RadialLine, svtkTypeBool);
  svtkGetMacro(RadialLine, svtkTypeBool);
  svtkBooleanMacro(RadialLine, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable/disable a center cursor
   * Default is disabled
   */
  svtkSetMacro(CenterCursor, bool);
  svtkGetMacro(CenterCursor, bool);
  svtkBooleanMacro(CenterCursor, bool);
  //@}

  /**
   * Grab the polydata (including points) that defines the sphere.  The
   * polydata consists of n+1 points, where n is the resolution of the
   * sphere. These point values are guaranteed to be up-to-date when either the
   * InteractionEvent or EndInteraction events are invoked. The user provides
   * the svtkPolyData and the points and polysphere are added to it.
   */
  void GetPolyData(svtkPolyData* pd);

  /**
   * Get the spherical implicit function defined by this widget.  Note that
   * svtkSphere is a subclass of svtkImplicitFunction, meaning that it can be
   * used by a variety of filters to perform clipping, cutting, and selection
   * of data.
   */
  void GetSphere(svtkSphere* sphere);

  //@{
  /**
   * Get the sphere properties. The properties of the sphere when selected
   * and unselected can be manipulated.
   */
  svtkGetObjectMacro(SphereProperty, svtkProperty);
  svtkGetObjectMacro(SelectedSphereProperty, svtkProperty);
  //@}

  //@{
  /**
   * Get the handle properties (the little ball on the sphere is the
   * handle). The properties of the handle when selected and unselected
   * can be manipulated.
   */
  svtkGetObjectMacro(HandleProperty, svtkProperty);
  svtkGetObjectMacro(SelectedHandleProperty, svtkProperty);
  //@}

  //@{
  /**
   * Get the handle text property. This can be used to control the appearance
   * of the handle text.
   */
  svtkGetObjectMacro(HandleTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Get the property of the radial line. This can be used to control the
   * appearance of the optional line connecting the center to the handle.
   */
  svtkGetObjectMacro(RadialLineProperty, svtkProperty);
  //@}

  /**
   * The interaction state may be set from a widget (e.g., svtkSphereWidget2) or
   * other object. This controls how the interaction with the widget
   * proceeds. Normally this method is used as part of a handshaking
   * process with the widget: First ComputeInteractionState() is invoked that
   * returns a state based on geometric considerations (i.e., cursor near a
   * widget feature), then based on events, the widget may modify this
   * further.
   */
  void SetInteractionState(int state);

  //@{
  /**
   * These are methods that satisfy svtkWidgetRepresentation's API. Note that a
   * version of place widget is available where the center and handle position
   * are specified.
   */
  void PlaceWidget(double bounds[6]) override;
  virtual void PlaceWidget(double center[3], double handlePosition[3]);
  void BuildRepresentation() override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void StartWidgetInteraction(double e[2]) override;
  void WidgetInteraction(double e[2]) override;
  double* GetBounds() override;
  //@}

  //@{
  /**
   * Methods supporting, and required by, the rendering process.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  int RenderOverlay(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  /*
   * Register internal Pickers within PickingManager
   */
  void RegisterPickers() override;

  //@{
  /**
   * Gets/Sets the constraint axis for translations. Returns Axis::NONE
   * if none.
   **/
  svtkGetMacro(TranslationAxis, int);
  svtkSetClampMacro(TranslationAxis, int, -1, 2);
  //@}

  //@{
  /**
   * Toggles constraint translation axis on/off.
   */
  void SetXTranslationAxisOn() { this->TranslationAxis = Axis::XAxis; }
  void SetYTranslationAxisOn() { this->TranslationAxis = Axis::YAxis; }
  void SetZTranslationAxisOn() { this->TranslationAxis = Axis::ZAxis; }
  void SetTranslationAxisOff() { this->TranslationAxis = Axis::NONE; }
  //@}

  //@{
  /**
   * Returns true if ContrainedAxis
   **/
  bool IsTranslationConstrained() { return this->TranslationAxis != Axis::NONE; }
  //@}

protected:
  svtkSphereRepresentation();
  ~svtkSphereRepresentation() override;

  // Manage how the representation appears
  double LastEventPosition[3];

  int TranslationAxis;

  // the sphere
  svtkActor* SphereActor;
  svtkPolyDataMapper* SphereMapper;
  svtkSphereSource* SphereSource;
  void HighlightSphere(int highlight);

  // The representation of the sphere
  int Representation;

  // Do the picking
  svtkCellPicker* HandlePicker;
  svtkCellPicker* SpherePicker;
  double LastPickPosition[3];

  // Methods to manipulate the sphere widget
  void Translate(const double* p1, const double* p2);
  void Scale(const double* p1, const double* p2, int X, int Y);
  void PlaceHandle(const double* center, double radius);
  virtual void SizeHandles();

  // Method to adapt the center cursor bounds
  // so it always have the same pixel size on screen
  virtual void AdaptCenterCursorBounds();

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  svtkProperty* SphereProperty;
  svtkProperty* SelectedSphereProperty;
  svtkProperty* HandleProperty;
  svtkProperty* SelectedHandleProperty;
  void CreateDefaultProperties();

  // Managing the handle
  svtkActor* HandleActor;
  svtkPolyDataMapper* HandleMapper;
  svtkSphereSource* HandleSource;
  void HighlightHandle(int);
  svtkTypeBool HandleVisibility;
  double HandleDirection[3];
  double HandlePosition[3];

  // Manage the handle label
  svtkTypeBool HandleText;
  svtkTextProperty* HandleTextProperty;
  svtkTextMapper* HandleTextMapper;
  svtkActor2D* HandleTextActor;

  // Manage the radial line segment
  svtkTypeBool RadialLine;
  svtkProperty* RadialLineProperty;
  svtkLineSource* RadialLineSource;
  svtkPolyDataMapper* RadialLineMapper;
  svtkActor* RadialLineActor;

  // Managing the center cursor
  svtkActor* CenterActor;
  svtkPolyDataMapper* CenterMapper;
  svtkCursor3D* CenterCursorSource;
  bool CenterCursor;

private:
  svtkSphereRepresentation(const svtkSphereRepresentation&) = delete;
  void operator=(const svtkSphereRepresentation&) = delete;
};

#endif
