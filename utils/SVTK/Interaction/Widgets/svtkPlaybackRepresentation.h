/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlaybackRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPlaybackRepresentation
 * @brief   represent the svtkPlaybackWidget
 *
 * This class is used to represent the svtkPlaybackWidget. Besides defining
 * geometry, this class defines a series of virtual method stubs that are
 * meant to be subclassed by applications for controlling playback.
 *
 * @sa
 * svtkPlaybackWidget
 */

#ifndef svtkPlaybackRepresentation_h
#define svtkPlaybackRepresentation_h

#include "svtkBorderRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkRenderer;
class svtkRenderWindowInteractor;
class svtkPoints;
class svtkPolyData;
class svtkTransformPolyDataFilter;
class svtkPolyDataMapper2D;
class svtkProperty2D;
class svtkActor2D;

class SVTKINTERACTIONWIDGETS_EXPORT svtkPlaybackRepresentation : public svtkBorderRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static svtkPlaybackRepresentation* New();

  //@{
  /**
   * Standard SVTK class methods.
   */
  svtkTypeMacro(svtkPlaybackRepresentation, svtkBorderRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * By obtaining this property you can specify the properties of the
   * representation.
   */
  svtkGetObjectMacro(Property, svtkProperty2D);
  //@}

  /**
   * Virtual callbacks that subclasses should implement.
   */
  virtual void Play() {}
  virtual void Stop() {}
  virtual void ForwardOneFrame() {}
  virtual void BackwardOneFrame() {}
  virtual void JumpToBeginning() {}
  virtual void JumpToEnd() {}

  /**
   * Satisfy the superclasses' API.
   */
  void BuildRepresentation() override;
  void GetSize(double size[2]) override
  {
    size[0] = 12.0;
    size[1] = 2.0;
  }

  //@{
  /**
   * These methods are necessary to make this representation behave as
   * a svtkProp.
   */
  void GetActors2D(svtkPropCollection*) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOverlay(svtkViewport*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

protected:
  svtkPlaybackRepresentation();
  ~svtkPlaybackRepresentation() override;

  // representation geometry
  svtkPoints* Points;
  svtkPolyData* PolyData;
  svtkTransformPolyDataFilter* TransformFilter;
  svtkPolyDataMapper2D* Mapper;
  svtkProperty2D* Property;
  svtkActor2D* Actor;

private:
  svtkPlaybackRepresentation(const svtkPlaybackRepresentation&) = delete;
  void operator=(const svtkPlaybackRepresentation&) = delete;
};

#endif
