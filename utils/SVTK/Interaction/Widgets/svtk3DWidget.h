/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtk3DWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtk3DWidget
 * @brief   an abstract superclass for 3D widgets
 *
 * svtk3DWidget is an abstract superclass for 3D interactor observers. These
 * 3D widgets represent themselves in the scene, and have special callbacks
 * associated with them that allows interactive manipulation of the widget.
 * Inparticular, the difference between a svtk3DWidget and its abstract
 * superclass svtkInteractorObserver is that svtk3DWidgets are "placed" in 3D
 * space.  svtkInteractorObservers have no notion of where they are placed,
 * and may not exist in 3D space at all.  3D widgets also provide auxiliary
 * functions like producing a transformation, creating polydata (for seeding
 * streamlines, probes, etc.) or creating implicit functions. See the
 * concrete subclasses for particulars.
 *
 * Typically the widget is used by specifying a svtkProp3D or SVTK dataset as
 * input, and then invoking the "On" method to activate it. (You can also
 * specify a bounding box to help position the widget.) Prior to invoking the
 * On() method, the user may also wish to use the PlaceWidget() to initially
 * position it. The 'i' (for "interactor") keypresses also can be used to
 * turn the widgets on and off (methods exist to change the key value
 * and enable keypress activiation).
 *
 * To support interactive manipulation of objects, this class (and
 * subclasses) invoke the events StartInteractionEvent, InteractionEvent, and
 * EndInteractionEvent.  These events are invoked when the svtk3DWidget enters
 * a state where rapid response is desired: mouse motion, etc. The events can
 * be used, for example, to set the desired update frame rate
 * (StartInteractionEvent), operate on the svtkProp3D or other object
 * (InteractionEvent), and set the desired frame rate back to normal values
 * (EndInteractionEvent).
 *
 * Note that the Priority attribute inherited from svtkInteractorObserver has
 * a new default value which is now 0.5 so that all 3D widgets have a higher
 * priority than the usual interactor styles.
 *
 * @sa
 * svtkBoxWidget svtkPlaneWidget svtkLineWidget svtkPointWidget
 * svtkSphereWidget svtkImplicitPlaneWidget
 */

#ifndef svtk3DWidget_h
#define svtk3DWidget_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkInteractorObserver.h"

class svtk3DWidgetConnection;
class svtkAlgorithmOutput;
class svtkDataSet;
class svtkProp3D;

class SVTKINTERACTIONWIDGETS_EXPORT svtk3DWidget : public svtkInteractorObserver
{
public:
  svtkTypeMacro(svtk3DWidget, svtkInteractorObserver);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * This method is used to initially place the widget.  The placement of the
   * widget depends on whether a Prop3D or input dataset is provided. If one
   * of these two is provided, they will be used to obtain a bounding box,
   * around which the widget is placed. Otherwise, you can manually specify a
   * bounds with the PlaceWidget(bounds) method. Note: PlaceWidget(bounds)
   * is required by all subclasses; the other methods are provided as
   * convenience methods.
   */
  virtual void PlaceWidget(double bounds[6]) = 0;
  virtual void PlaceWidget();
  virtual void PlaceWidget(
    double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);
  //@}

  //@{
  /**
   * Specify a svtkProp3D around which to place the widget. This
   * is not required, but if supplied, it is used to initially
   * position the widget.
   */
  virtual void SetProp3D(svtkProp3D*);
  svtkGetObjectMacro(Prop3D, svtkProp3D);
  //@}

  //@{
  /**
   * Specify the input dataset. This is not required, but if supplied,
   * and no svtkProp3D is specified, it is used to initially position
   * the widget.
   */
  virtual void SetInputData(svtkDataSet*);
  virtual void SetInputConnection(svtkAlgorithmOutput*);
  virtual svtkDataSet* GetInput();
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
   * Set/Get the factor that controls the size of the handles that
   * appear as part of the widget. These handles (like spheres, etc.)
   * are used to manipulate the widget, and are sized as a fraction of
   * the screen diagonal.
   */
  svtkSetClampMacro(HandleSize, double, 0.001, 0.5);
  svtkGetMacro(HandleSize, double);
  //@}

protected:
  svtk3DWidget();
  ~svtk3DWidget() override;

  // Used to position and scale the widget initially
  svtkProp3D* Prop3D;

  svtk3DWidgetConnection* ConnectionHolder;

  // has the widget ever been placed
  double PlaceFactor;
  int Placed;
  void AdjustBounds(double bounds[6], double newBounds[6], double center[3]);

  // control the size of handles (if there are any)
  double InitialBounds[6];
  double InitialLength;
  double HandleSize;
  double SizeHandles(double factor);
  virtual void SizeHandles() {} // subclass in turn invokes parent's SizeHandles()

  // used to track the depth of the last pick; also interacts with handle sizing
  int ValidPick;
  double LastPickPosition[3];

  void UpdateInput();

private:
  svtk3DWidget(const svtk3DWidget&) = delete;
  void operator=(const svtk3DWidget&) = delete;
};

#endif
