/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResliceImageViewerMeasurements.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkResliceImageViewerMeasurements
 * @brief   Manage measurements on a resliced image
 *
 * This class manages measurements on the resliced image. It toggles the
 * the visibility of the measurements based on whether the resliced image
 * is the same orientation as when the measurement was initially placed.
 * @sa
 * svtkResliceCursor svtkResliceCursorWidget svtkResliceCursorRepresentation
 */

#ifndef svtkResliceImageViewerMeasurements_h
#define svtkResliceImageViewerMeasurements_h

#include "svtkInteractionImageModule.h" // For export macro
#include "svtkObject.h"

class svtkResliceImageViewer;
class svtkAbstractWidget;
class svtkCallbackCommand;
class svtkCollection;
class svtkDistanceWidget;
class svtkAngleWidget;
class svtkBiDimensionalWidget;
class svtkHandleRepresentation;
class svtkHandleWidget;
class svtkCaptionWidget;
class svtkContourWidget;
class svtkSeedWidget;

class SVTKINTERACTIONIMAGE_EXPORT svtkResliceImageViewerMeasurements : public svtkObject
{
public:
  //@{
  /**
   * Standard SVTK methods.
   */
  static svtkResliceImageViewerMeasurements* New();
  svtkTypeMacro(svtkResliceImageViewerMeasurements, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Render the measurements.
   */
  virtual void Render();

  //@{
  /**
   * Add / remove a measurement widget
   */
  virtual void AddItem(svtkAbstractWidget*);
  virtual void RemoveItem(svtkAbstractWidget*);
  virtual void RemoveAllItems();
  //@}

  //@{
  /**
   * Methods to change whether the widget responds to interaction.
   * Set this to Off to disable interaction. On by default.
   * Subclasses must override SetProcessEvents() to make sure
   * that they pass on the flag to all component widgets.
   */
  svtkSetClampMacro(ProcessEvents, svtkTypeBool, 0, 1);
  svtkGetMacro(ProcessEvents, svtkTypeBool);
  svtkBooleanMacro(ProcessEvents, svtkTypeBool);
  //@}

  //@{
  /**
   * Tolerance for Point-in-Plane check
   */
  svtkSetMacro(Tolerance, double);
  svtkGetMacro(Tolerance, double);
  //@}

  //@{
  /**
   * Set the reslice image viewer. This is automatically done in the class
   * svtkResliceImageViewer
   */
  virtual void SetResliceImageViewer(svtkResliceImageViewer*);
  svtkGetObjectMacro(ResliceImageViewer, svtkResliceImageViewer);
  //@}

  /**
   * Update the measurements. This is automatically called when the reslice
   * cursor's axes are change.
   */
  virtual void Update();

protected:
  svtkResliceImageViewerMeasurements();
  ~svtkResliceImageViewerMeasurements() override;

  //@{
  /**
   * Check if a measurement widget is on the resliced plane.
   */
  bool IsItemOnReslicedPlane(svtkAbstractWidget* w);
  bool IsWidgetOnReslicedPlane(svtkDistanceWidget* w);
  bool IsWidgetOnReslicedPlane(svtkAngleWidget* w);
  bool IsWidgetOnReslicedPlane(svtkBiDimensionalWidget* w);
  bool IsWidgetOnReslicedPlane(svtkCaptionWidget* w);
  bool IsWidgetOnReslicedPlane(svtkContourWidget* w);
  bool IsWidgetOnReslicedPlane(svtkSeedWidget* w);
  bool IsWidgetOnReslicedPlane(svtkHandleWidget* w);
  bool IsPointOnReslicedPlane(svtkHandleRepresentation* h);
  bool IsPositionOnReslicedPlane(double p[3]);
  //@}

  // Handles the events; centralized here for all widgets.
  static void ProcessEventsHandler(
    svtkObject* object, unsigned long event, void* clientdata, void* calldata);

  svtkResliceImageViewer* ResliceImageViewer;
  svtkCollection* WidgetCollection;

  // Handle the visibility of the measurements.
  svtkCallbackCommand* EventCallbackCommand; //

  // Flag indicating if we should handle events.
  // On by default.
  svtkTypeBool ProcessEvents;

  // Tolerance for Point-in-plane computation
  double Tolerance;

private:
  svtkResliceImageViewerMeasurements(const svtkResliceImageViewerMeasurements&) = delete;
  void operator=(const svtkResliceImageViewerMeasurements&) = delete;
};

#endif
