/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkImplicitPlaneRepresentation.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenVRMenuRepresentation
 * @brief   Widget representation for svtkOpenVRPanelWidget
 * Implementation of the popup panel representation for the
 * svtkOpenVRPanelWidget.
 * This representation is rebuilt every time the selected/hovered prop changes.
 * Its position is set according to the camera orientation and is placed at a
 * distance defined in meters in the BuildRepresentation() method.
 *
 * WARNING: The panel might be occluded by other props.
 *   TODO: Improve placement method.
 **/

#ifndef svtkOpenVRMenuRepresentation_h
#define svtkOpenVRMenuRepresentation_h

#include "svtkRenderingOpenVRModule.h" // For export macro
#include "svtkWidgetRepresentation.h"
#include <deque> // for ivar

class svtkActor;
class svtkProperty;
class svtkPolyData;
class svtkPolyDataMapper;
class svtkCellArray;
class svtkPoints;
class svtkTextActor3D;

class SVTKRENDERINGOPENVR_EXPORT svtkOpenVRMenuRepresentation : public svtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static svtkOpenVRMenuRepresentation* New();

  //@{
  /**
   * Standard methods for the class.
   */
  svtkTypeMacro(svtkOpenVRMenuRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Methods to interface with the svtkOpenVRPanelWidget.
   */
  void BuildRepresentation() override;

  void StartComplexInteraction(svtkRenderWindowInteractor* iren, svtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  void ComplexInteraction(svtkRenderWindowInteractor* iren, svtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  void EndComplexInteraction(svtkRenderWindowInteractor* iren, svtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  //@}

  //@{
  /**
   * Methods supporting the rendering process.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  int RenderOverlay(svtkViewport*) override;
  //@}

  //@{
  /**
   * Methods to add/remove items to the menu, called by the menu widget
   */
  void PushFrontMenuItem(const char* name, const char* text, svtkCommand* cmd);
  void RenameMenuItem(const char* name, const char* text);
  void RemoveMenuItem(const char* name);
  void RemoveAllMenuItems();
  //@}

  svtkGetMacro(CurrentOption, double);

protected:
  svtkOpenVRMenuRepresentation();
  ~svtkOpenVRMenuRepresentation() override;

  class InternalElement;
  std::deque<InternalElement*> Menus;

  double CurrentOption; // count from start of the list
  double PlacedPos[3];
  double PlacedDOP[3];
  double PlacedVUP[3];
  double PlacedVRight[3];
  double PlacedOrientation[3];

private:
  svtkOpenVRMenuRepresentation(const svtkOpenVRMenuRepresentation&) = delete;
  void operator=(const svtkOpenVRMenuRepresentation&) = delete;
};

#endif
