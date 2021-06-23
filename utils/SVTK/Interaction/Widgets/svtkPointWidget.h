/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPointWidget
 * @brief   position a point in 3D space
 *
 * This 3D widget allows the user to position a point in 3D space using a 3D
 * cursor. The cursor has an outline bounding box, axes-aligned cross-hairs,
 * and axes shadows. (The outline and shadows can be turned off.) Any of
 * these can be turned off. A nice feature of the object is that the
 * svtkPointWidget, like any 3D widget, will work with the current interactor
 * style. That is, if svtkPointWidget does not handle an event, then all other
 * registered observers (including the interactor style) have an opportunity
 * to process the event. Otherwise, the svtkPointWidget will terminate the
 * processing of the event that it handles.
 *
 * To use this object, just invoke SetInteractor() with the argument of the
 * method a svtkRenderWindowInteractor.  You may also wish to invoke
 * "PlaceWidget()" to initially position the widget. The interactor will act
 * normally until the "i" key (for "interactor") is pressed, at which point
 * the svtkPointWidget will appear. (See superclass documentation for
 * information about changing this behavior.) To move the point, the user can
 * grab (left mouse) on any widget line and "slide" the point into
 * position. Scaling is achieved by using the right mouse button "up" the
 * render window (makes the widget bigger) or "down" the render window (makes
 * the widget smaller). To translate the widget use the middle mouse button.
 * (Note: all of the translation interactions can be constrained to one of
 * the x-y-z axes by using the "shift" key.) The svtkPointWidget produces as
 * output a polydata with a single point and a vertex cell.
 *
 * Some additional features of this class include the ability to control the
 * rendered properties of the widget. You can set the properties of the
 * selected and unselected representations of the parts of the widget. For
 * example, you can set the property of the 3D cursor in its normal and
 * selected states.
 *
 *
 * The constrained translation/sliding action (i.e., when the "shift" key is
 * depressed) along the axes is based on a combination of a "hot" spot around
 * the cursor focus plus the initial mouse motion after selection. That is,
 * if the user selects an axis outside of the hot spot, then the motion is
 * constrained along that axis. If the user selects the point widget near the
 * focus (within the hot spot), the initial motion defines a vector which is
 * compared to the x-y-z axes. The motion is constrained to the axis that is
 * most parallel to the initial motion vector.
 *
 *
 * @sa
 * svtk3DWidget svtkLineWidget svtkBoxWidget svtkPlaneWidget
 */

#ifndef svtkPointWidget_h
#define svtkPointWidget_h

#include "svtk3DWidget.h"
#include "svtkCursor3D.h"                 // Needed for faster access to the Cursor3D
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkActor;
class svtkPolyDataMapper;
class svtkCellPicker;
class svtkPolyData;
class svtkProperty;

class SVTKINTERACTIONWIDGETS_EXPORT svtkPointWidget : public svtk3DWidget
{
public:
  /**
   * Instantiate this widget
   */
  static svtkPointWidget* New();

  svtkTypeMacro(svtkPointWidget, svtk3DWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Methods that satisfy the superclass' API.
   */
  void SetEnabled(int) override;
  void PlaceWidget(double bounds[6]) override;
  void PlaceWidget() override { this->Superclass::PlaceWidget(); }
  void PlaceWidget(
    double xmin, double xmax, double ymin, double ymax, double zmin, double zmax) override
  {
    this->Superclass::PlaceWidget(xmin, xmax, ymin, ymax, zmin, zmax);
  }
  //@}

  /**
   * Grab the polydata (including points) that defines the point. A
   * single point and a vertex compose the svtkPolyData.
   */
  void GetPolyData(svtkPolyData* pd);

  /**
   * Set/Get the position of the point. Note that if the position is set
   * outside of the bounding box, it will be clamped to the boundary of
   * the bounding box.
   */
  void SetPosition(double x, double y, double z) { this->Cursor3D->SetFocalPoint(x, y, z); }
  void SetPosition(double x[3]) { this->SetPosition(x[0], x[1], x[2]); }
  double* GetPosition() SVTK_SIZEHINT(3) { return this->Cursor3D->GetFocalPoint(); }
  void GetPosition(double xyz[3]) { this->Cursor3D->GetFocalPoint(xyz); }

  /**
   * Turn on/off the wireframe bounding box.
   */
  void SetOutline(int o) { this->Cursor3D->SetOutline(o); }
  int GetOutline() { return this->Cursor3D->GetOutline(); }
  void OutlineOn() { this->Cursor3D->OutlineOn(); }
  void OutlineOff() { this->Cursor3D->OutlineOff(); }

  /**
   * Turn on/off the wireframe x-shadows.
   */
  void SetXShadows(int o) { this->Cursor3D->SetXShadows(o); }
  int GetXShadows() { return this->Cursor3D->GetXShadows(); }
  void XShadowsOn() { this->Cursor3D->XShadowsOn(); }
  void XShadowsOff() { this->Cursor3D->XShadowsOff(); }

  /**
   * Turn on/off the wireframe y-shadows.
   */
  void SetYShadows(int o) { this->Cursor3D->SetYShadows(o); }
  int GetYShadows() { return this->Cursor3D->GetYShadows(); }
  void YShadowsOn() { this->Cursor3D->YShadowsOn(); }
  void YShadowsOff() { this->Cursor3D->YShadowsOff(); }

  /**
   * Turn on/off the wireframe z-shadows.
   */
  void SetZShadows(int o) { this->Cursor3D->SetZShadows(o); }
  int GetZShadows() { return this->Cursor3D->GetZShadows(); }
  void ZShadowsOn() { this->Cursor3D->ZShadowsOn(); }
  void ZShadowsOff() { this->Cursor3D->ZShadowsOff(); }

  /**
   * If translation mode is on, as the widget is moved the bounding box,
   * shadows, and cursor are all translated simultaneously as the point
   * moves.
   */
  void SetTranslationMode(int mode)
  {
    this->Cursor3D->SetTranslationMode(mode);
    this->Cursor3D->Update();
  }
  int GetTranslationMode() { return this->Cursor3D->GetTranslationMode(); }
  void TranslationModeOn() { this->SetTranslationMode(1); }
  void TranslationModeOff() { this->SetTranslationMode(0); }

  //@{
  /**
   * Convenience methods to turn outline and shadows on and off.
   */
  void AllOn()
  {
    this->OutlineOn();
    this->XShadowsOn();
    this->YShadowsOn();
    this->ZShadowsOn();
  }
  void AllOff()
  {
    this->OutlineOff();
    this->XShadowsOff();
    this->YShadowsOff();
    this->ZShadowsOff();
  }
  //@}

  //@{
  /**
   * Get the handle properties (the little balls are the handles). The
   * properties of the handles when selected and normal can be
   * set.
   */
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

protected:
  svtkPointWidget();
  ~svtkPointWidget() override;

  // Manage the state of the widget
  friend class svtkLineWidget;

  int State;
  enum WidgetState
  {
    Start = 0,
    Moving,
    Scaling,
    Translating,
    Outside
  };

  // Handles the events
  static void ProcessEvents(
    svtkObject* object, unsigned long event, void* clientdata, void* calldata);

  // ProcessEvents() dispatches to these methods.
  virtual void OnMouseMove();
  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();
  virtual void OnMiddleButtonDown();
  virtual void OnMiddleButtonUp();
  virtual void OnRightButtonDown();
  virtual void OnRightButtonUp();

  // the cursor3D
  svtkActor* Actor;
  svtkPolyDataMapper* Mapper;
  svtkCursor3D* Cursor3D;
  void Highlight(int highlight);

  // Do the picking
  svtkCellPicker* CursorPicker;

  // Register internal Pickers within PickingManager
  void RegisterPickers() override;

  // Methods to manipulate the cursor
  int ConstraintAxis;
  void Translate(double* p1, double* p2);
  void Scale(double* p1, double* p2, int X, int Y);
  void MoveFocus(double* p1, double* p2);
  int TranslationMode;

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  svtkProperty* Property;
  svtkProperty* SelectedProperty;
  void CreateDefaultProperties();

  // The size of the hot spot.
  double HotSpotSize;
  int DetermineConstraintAxis(int constraint, double* x);
  int WaitingForMotion;
  int WaitCount;

private:
  svtkPointWidget(const svtkPointWidget&) = delete;
  void operator=(const svtkPointWidget&) = delete;
};

#endif
