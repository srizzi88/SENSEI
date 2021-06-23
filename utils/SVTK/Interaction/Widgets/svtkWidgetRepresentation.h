/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWidgetRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWidgetRepresentation
 * @brief   abstract class defines interface between the widget and widget representation classes
 *
 * This class is used to define the API for, and partially implement, a
 * representation for different types of widgets. Note that the widget
 * representation (i.e., subclasses of svtkWidgetRepresentation) are a type of
 * svtkProp; meaning that they can be associated with a svtkRenderer end
 * embedded in a scene like any other svtkActor. However,
 * svtkWidgetRepresentation also defines an API that enables it to be paired
 * with a subclass svtkAbstractWidget, meaning that it can be driven by a
 * widget, serving to represent the widget as the widget responds to
 * registered events.
 *
 * The API defined here should be regarded as a guideline for implementing
 * widgets and widget representations. Widget behavior is complex, as is the
 * way the representation responds to the registered widget events, so the API
 * may vary from widget to widget to reflect this complexity.
 *
 * @warning
 * The separation of the widget event handling and representation enables
 * users and developers to create new appearances for the widget. It also
 * facilitates parallel processing, where the client application handles
 * events, and remote representations of the widget are slaves to the
 * client (and do not handle events).
 */

#ifndef svtkWidgetRepresentation_h
#define svtkWidgetRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkNew.h"                      // for ivars
#include "svtkProp.h"
#include "svtkWeakPointer.h" // needed for svtkWeakPointer iVar.

class svtkAbstractPropPicker;
class svtkAbstractWidget;
class svtkMatrix4x4;
class svtkPickingManager;
class svtkProp3D;
class svtkRenderWindowInteractor;
class svtkRenderer;
class svtkTransform;

class SVTKINTERACTIONWIDGETS_EXPORT svtkWidgetRepresentation : public svtkProp
{
public:
  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkWidgetRepresentation, svtkProp);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Enable/Disable the use of a manager to process the picking.
   * Enabled by default.
   */
  svtkBooleanMacro(PickingManaged, bool);
  void SetPickingManaged(bool managed);
  svtkGetMacro(PickingManaged, bool);
  //@}

  //@{
  /**
   * Subclasses of svtkWidgetRepresentation must implement these methods. This is
   * considered the minimum API for a widget representation.
   * <pre>
   * SetRenderer() - the renderer in which the representations draws itself.
   * Typically the renderer is set by the associated widget.
   * Use the widget's SetCurrentRenderer() method in most cases;
   * otherwise there is a risk of inconsistent behavior as events
   * and drawing may be performed in different viewports.
   * BuildRepresentation() - update the geometry of the widget based on its
   * current state.
   * </pre>
   * WARNING: The renderer is NOT reference counted by the representation,
   * in order to avoid reference loops.  Be sure that the representation
   * lifetime does not extend beyond the renderer lifetime.
   */
  virtual void SetRenderer(svtkRenderer* ren);
  virtual svtkRenderer* GetRenderer();
  virtual void BuildRepresentation() = 0;
  //@}

  /**
   * The following is a suggested API for widget representations. These methods
   * define the communication between the widget and its representation. These
   * methods are only suggestions because widgets take on so many different
   * forms that a universal API is not deemed practical. However, these methods
   * should be implemented when possible to insure that the SVTK widget hierarchy
   * remains self-consistent.
   * <pre>
   * PlaceWidget() - given a bounding box (xmin,xmax,ymin,ymax,zmin,zmax), place
   * the widget inside of it. The current orientation of the widget
   * is preserved, only scaling and translation is performed.
   * StartWidgetInteraction() - generally corresponds to a initial event (e.g.,
   * mouse down) that starts the interaction process
   * with the widget.
   * WidgetInteraction() - invoked when an event causes the widget to change
   * appearance.
   * EndWidgetInteraction() - generally corresponds to a final event (e.g., mouse up)
   * and completes the interaction sequence.
   * ComputeInteractionState() - given (X,Y) display coordinates in a renderer, with a
   * possible flag that modifies the computation,
   * what is the state of the widget?
   * GetInteractionState() - return the current state of the widget. Note that the
   * value of "0" typically refers to "outside". The
   * interaction state is strictly a function of the
   * representation, and the widget/represent must agree
   * on what they mean.
   * Highlight() - turn on or off any highlights associated with the widget.
   * Highlights are generally turned on when the widget is selected.
   * </pre>
   * Note that subclasses may ignore some of these methods and implement their own
   * depending on the specifics of the widget.
   */
  virtual void PlaceWidget(double* svtkNotUsed(bounds[6])) {}
  virtual void StartWidgetInteraction(double eventPos[2]) { (void)eventPos; }
  virtual void WidgetInteraction(double newEventPos[2]) { (void)newEventPos; }
  virtual void EndWidgetInteraction(double newEventPos[2]) { (void)newEventPos; }
  virtual int ComputeInteractionState(int X, int Y, int modify = 0);
  virtual int GetInteractionState() { return this->InteractionState; }
  virtual void Highlight(int svtkNotUsed(highlightOn)) {}

  //@{
  // Widgets were originally designed to be driven by 2D mouse events
  // With Virtual Reality and multitouch we get mnore complex events
  // that may involve multiple pointers as well as 3D pointers and
  // orientations. As such we provide pointers to the interactor
  // widget and an event type so that representations can access the
  // values they need.
  virtual void StartComplexInteraction(
    svtkRenderWindowInteractor*, svtkAbstractWidget*, unsigned long /* event */, void* /*callData*/)
  {
  }
  virtual void ComplexInteraction(
    svtkRenderWindowInteractor*, svtkAbstractWidget*, unsigned long /* event */, void* /* callData */)
  {
  }
  virtual void EndComplexInteraction(
    svtkRenderWindowInteractor*, svtkAbstractWidget*, unsigned long /* event */, void* /* callData */)
  {
  }
  virtual int ComputeComplexInteractionState(svtkRenderWindowInteractor* iren,
    svtkAbstractWidget* widget, unsigned long event, void* callData, int modify = 0);
  //@}

  //@{
  /**
   * Set/Get a factor representing the scaling of the widget upon placement
   * (via the PlaceWidget() method). Normally the widget is placed so that
   * it just fits within the bounding box defined in PlaceWidget(bounds).
   * The PlaceFactor will make the widget larger (PlaceFactor > 1) or smaller
   * (PlaceFactor < 1). By default, PlaceFactor is set to 0.5.
   */
  svtkSetClampMacro(PlaceFactor, double, 0.01, SVTK_DOUBLE_MAX);
  svtkGetMacro(PlaceFactor, double);
  //@}

  //@{
  /**
   * Set/Get the factor that controls the size of the handles that appear as
   * part of the widget (if any). These handles (like spheres, etc.)  are
   * used to manipulate the widget. The HandleSize data member allows you
   * to change the relative size of the handles. Note that while the handle
   * size is typically expressed in pixels, some subclasses may use a relative size
   * with respect to the viewport. (As a corollary, the value of this ivar is often
   * set by subclasses of this class during instance instantiation.)
   */
  svtkSetClampMacro(HandleSize, double, 0.001, 1000);
  svtkGetMacro(HandleSize, double);
  //@}

  //@{
  /**
   * Some subclasses use this data member to keep track of whether to render
   * or not (i.e., to minimize the total number of renders).
   */
  svtkGetMacro(NeedToRender, svtkTypeBool);
  svtkSetClampMacro(NeedToRender, svtkTypeBool, 0, 1);
  svtkBooleanMacro(NeedToRender, svtkTypeBool);
  //@}

  /**
   * Methods to make this class behave as a svtkProp. They are repeated here (from the
   * svtkProp superclass) as a reminder to the widget implementor. Failure to implement
   * these methods properly may result in the representation not appearing in the scene
   * (i.e., not implementing the Render() methods properly) or leaking graphics resources
   * (i.e., not implementing ReleaseGraphicsResources() properly).
   */
  double* GetBounds() SVTK_SIZEHINT(6) override { return nullptr; }
  void ShallowCopy(svtkProp* prop) override;
  void GetActors(svtkPropCollection*) override {}
  void GetActors2D(svtkPropCollection*) override {}
  void GetVolumes(svtkPropCollection*) override {}
  void ReleaseGraphicsResources(svtkWindow*) override {}
  int RenderOverlay(svtkViewport* svtkNotUsed(viewport)) override { return 0; }
  int RenderOpaqueGeometry(svtkViewport* svtkNotUsed(viewport)) override { return 0; }
  int RenderTranslucentPolygonalGeometry(svtkViewport* svtkNotUsed(viewport)) override { return 0; }
  int RenderVolumetricGeometry(svtkViewport* svtkNotUsed(viewport)) override { return 0; }
  svtkTypeBool HasTranslucentPolygonalGeometry() override { return 0; }

  /**
   * Register internal Pickers in the Picking Manager.
   * Must be reimplemented by concrete widget representations to register
   * their pickers.
   */
  virtual void RegisterPickers();

  /**
   * Unregister internal pickers from the Picking Manager.
   */
  virtual void UnRegisterPickers();

  //@{
  /**
   * Axis labels
   */
  enum Axis
  {
    NONE = -1,
    XAxis = 0,
    YAxis = 1,
    ZAxis = 2
  };
  //@}

protected:
  svtkWidgetRepresentation();
  ~svtkWidgetRepresentation() override;

  // The renderer in which this widget is placed
  svtkWeakPointer<svtkRenderer> Renderer;

  // The state of this representation based on a recent event
  int InteractionState;

  // These are used to track the beginning of interaction with the representation
  // It's dimensioned [3] because some events re processed in 3D.
  double StartEventPosition[3];

  // Instance variable and members supporting suclasses
  double PlaceFactor; // Used to control how widget is placed around bounding box
  int Placed;         // Indicate whether widget has been placed
  void AdjustBounds(double bounds[6], double newBounds[6], double center[3]);
  double InitialBounds[6]; // initial bounds on place widget (valid after PlaceWidget)
  double InitialLength;    // initial length on place widget

  // Sizing handles is tricky because the procedure requires information
  // relative to the last pick, as well as a live renderer to perform
  // coordinate conversions. In some cases, a pick is never made so handle
  // sizing has to follow a different path. The following ivars help with
  // this process.
  int ValidPick; // indicate when valid picks are made

  // This variable controls whether the picking is managed by the Picking
  // Manager or not. True by default.
  bool PickingManaged;

  /**
   * Return the picking manager associated on the context on which the widget
   * representation currently belong.
   */
  svtkPickingManager* GetPickingManager();

  /**
   * Proceed to a pick, whether through the PickingManager if the picking is
   * managed or directly using the registered picker, and return the assembly
   * path.
   */
  svtkAssemblyPath* GetAssemblyPath(double X, double Y, double Z, svtkAbstractPropPicker* picker);
  svtkAssemblyPath* GetAssemblyPath3DPoint(double pos[3], svtkAbstractPropPicker* picker);

  // Helper function to cull events if they are not near to the actual widget
  // representation. This is needed typically in situations of extreme zoom
  // for 3D widgets. The current event position, and 3D bounds of the widget
  // are provided.
  bool NearbyEvent(int X, int Y, double bounds[6]);

  // Members use to control handle size. The two methods return a "radius"
  // in world coordinates. Note that the HandleSize data member is used
  // internal to the SizeHandles__() methods.
  double HandleSize; // controlling relative size of widget handles
  double SizeHandlesRelativeToViewport(double factor, double pos[3]);
  double SizeHandlesInPixels(double factor, double pos[3]);

  // Try and reduce multiple renders
  svtkTypeBool NeedToRender;

  // This is the time that the representation was built. This data member
  // can be used to reduce the time spent building the widget.
  svtkTimeStamp BuildTime;

  // update the pose of a prop based on two sets of
  // position, orientation vectors
  void UpdatePropPose(svtkProp3D* prop, const double* pos1, const double* orient1,
    const double* pos2, const double* orient2);
  svtkNew<svtkTransform> TempTransform;
  svtkNew<svtkMatrix4x4> TempMatrix;

private:
  svtkWidgetRepresentation(const svtkWidgetRepresentation&) = delete;
  void operator=(const svtkWidgetRepresentation&) = delete;
};

#endif
