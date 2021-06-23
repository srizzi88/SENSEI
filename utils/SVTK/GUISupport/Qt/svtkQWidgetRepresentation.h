/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQWidgetRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkQWidgetRepresentation
 * @brief   a class defining the representation for a svtkQWidgetWidget
 *
 * This class renders a QWidget as a simple svtkPlaneSource with a svtkTexture
 * that contains a svtkQWidgetTexture which imports the OpenGL texture handle
 * from Qt into the svtk scene. Qt and SVTK may need to be useing the same
 * graphics context.
 */

#ifndef svtkQWidgetRepresentation_h
#define svtkQWidgetRepresentation_h

#include "svtkGUISupportQtModule.h" // For export macro
#include "svtkWidgetRepresentation.h"

class QWidget;
class svtkActor;
class svtkCellPicker;
class svtkOpenGLTexture;
class svtkPlaneSource;
class svtkPolyDataAlgorithm;
class svtkPolyDataMapper;
class svtkQWidgetTexture;

class SVTKGUISUPPORTQT_EXPORT svtkQWidgetRepresentation : public svtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static svtkQWidgetRepresentation* New();

  //@{
  /**
   * Standard methods for the class.
   */
  svtkTypeMacro(svtkQWidgetRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Satisfies superclass API.  This returns a pointer to the underlying
   * PolyData (which represents the plane).
   */
  svtkPolyDataAlgorithm* GetPolyDataAlgorithm();

  /**
   * Satisfies the superclass API.  This will change the state of the widget
   * to match changes that have been made to the underlying PolyDataSource
   */
  void UpdatePlacement(void);

  //@{
  /**
   * Methods to interface with the svtkImplicitPlaneWidget2.
   */
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override;
  int ComputeComplexInteractionState(svtkRenderWindowInteractor* iren, svtkAbstractWidget* widget,
    unsigned long event, void* calldata, int modify = 0) override;
  //@}

  //@{
  /**
   * Methods supporting the rendering process.
   */
  double* GetBounds() SVTK_SIZEHINT(6) override;
  void GetActors(svtkPropCollection* pc) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  // Manage the state of the widget
  enum _InteractionState
  {
    Outside = 0,
    Inside
  };

  //@{
  /**
   * The interaction state may be set from a widget (e.g.,
   * svtkQWidgetWidget) or other object. This controls how the
   * interaction with the widget proceeds. Normally this method is used as
   * part of a handshaking process with the widget: First
   * ComputeInteractionState() is invoked that returns a state based on
   * geometric considerations (i.e., cursor near a widget feature), then
   * based on events, the widget may modify this further.
   */
  svtkSetClampMacro(InteractionState, int, Outside, Inside);
  //@}

  /**
   * Set the QWidget this representation will render
   */
  void SetWidget(QWidget* w);

  /**
   * Get the QWidgetTexture used by the representation
   */
  svtkGetObjectMacro(QWidgetTexture, svtkQWidgetTexture);

  /**
   * Get the svtkPlaneSouce used by this representation. This can be useful
   * to set the Origin, Point1, Point2 of the plane source directly.
   */
  svtkGetObjectMacro(PlaneSource, svtkPlaneSource);

  /**
   * Get the widget coordinates as computed in the last call to
   * ComputeComplexInteractionState.
   */
  svtkGetVector2Macro(WidgetCoordinates, int);

protected:
  svtkQWidgetRepresentation();
  ~svtkQWidgetRepresentation() override;

  int WidgetCoordinates[2];

  svtkPlaneSource* PlaneSource;
  svtkPolyDataMapper* PlaneMapper;
  svtkActor* PlaneActor;
  svtkOpenGLTexture* PlaneTexture;
  svtkQWidgetTexture* QWidgetTexture;

  svtkCellPicker* Picker;

  // Register internal Pickers within PickingManager
  void RegisterPickers() override;

private:
  svtkQWidgetRepresentation(const svtkQWidgetRepresentation&) = delete;
  void operator=(const svtkQWidgetRepresentation&) = delete;
};

#endif
