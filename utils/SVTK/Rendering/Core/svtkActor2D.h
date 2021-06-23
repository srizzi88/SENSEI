/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkActor2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkActor2D
 * @brief   a actor that draws 2D data
 *
 * svtkActor2D is similar to svtkActor, but it is made to be used with two
 * dimensional images and annotation.  svtkActor2D has a position but does not
 * use a transformation matrix like svtkActor (see the superclass svtkProp
 * for information on positioning svtkActor2D).  svtkActor2D has a reference to
 * a svtkMapper2D object which does the rendering.
 *
 * @sa
 * svtkProp  svtkMapper2D svtkProperty2D
 */

#ifndef svtkActor2D_h
#define svtkActor2D_h

#include "svtkCoordinate.h" // For svtkViewportCoordinateMacro
#include "svtkProp.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkMapper2D;
class svtkProperty2D;

class SVTKRENDERINGCORE_EXPORT svtkActor2D : public svtkProp
{
public:
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkActor2D, svtkProp);

  /**
   * Creates an actor2D with the following defaults:
   * position (0,0) (coordinate system is viewport);
   * at layer 0.
   */
  static svtkActor2D* New();

  //@{
  /**
   * Support the standard render methods.
   */
  int RenderOverlay(svtkViewport* viewport) override;
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

  //@{
  /**
   * Set/Get the svtkMapper2D which defines the data to be drawn.
   */
  virtual void SetMapper(svtkMapper2D* mapper);
  svtkGetObjectMacro(Mapper, svtkMapper2D);
  //@}

  //@{
  /**
   * Set/Get the layer number in the overlay planes into which to render.
   */
  svtkSetMacro(LayerNumber, int);
  svtkGetMacro(LayerNumber, int);
  //@}

  /**
   * Returns this actor's svtkProperty2D.  Creates a property if one
   * doesn't already exist.
   */
  svtkProperty2D* GetProperty();

  /**
   * Set this svtkProp's svtkProperty2D.
   */
  virtual void SetProperty(svtkProperty2D*);

  //@{
  /**
   * Get the PositionCoordinate instance of svtkCoordinate.
   * This is used for for complicated or relative positioning.
   * The position variable controls the lower left corner of the Actor2D
   */
  svtkViewportCoordinateMacro(Position);
  //@}

  /**
   * Set the Prop2D's position in display coordinates.
   */
  void SetDisplayPosition(int, int);

  //@{
  /**
   * Access the Position2 instance variable. This variable controls
   * the upper right corner of the Actor2D. It is by default
   * relative to Position and in normalized viewport coordinates.
   * Some 2D actor subclasses ignore the position2 variable
   */
  svtkViewportCoordinateMacro(Position2);
  //@}

  //@{
  /**
   * Set/Get the height and width of the Actor2D. The value is expressed
   * as a fraction of the viewport. This really is just another way of
   * setting the Position2 instance variable.
   */
  void SetWidth(double w);
  double GetWidth();
  void SetHeight(double h);
  double GetHeight();
  //@}

  /**
   * Return this objects MTime.
   */
  svtkMTimeType GetMTime() override;

  /**
   * For some exporters and other other operations we must be
   * able to collect all the actors or volumes. These methods
   * are used in that process.
   */
  void GetActors2D(svtkPropCollection* pc) override;

  /**
   * Shallow copy of this svtkActor2D. Overloads the virtual svtkProp method.
   */
  void ShallowCopy(svtkProp* prop) override;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  /**
   * Return the actual svtkCoordinate reference that the mapper should use
   * to position the actor. This is used internally by the mappers and should
   * be overridden in specialized subclasses and otherwise ignored.
   */
  virtual svtkCoordinate* GetActualPositionCoordinate(void) { return this->PositionCoordinate; }

  /**
   * Return the actual svtkCoordinate reference that the mapper should use
   * to position the actor. This is used internally by the mappers and should
   * be overridden in specialized subclasses and otherwise ignored.
   */
  virtual svtkCoordinate* GetActualPosition2Coordinate(void) { return this->Position2Coordinate; }

protected:
  svtkActor2D();
  ~svtkActor2D() override;

  svtkMapper2D* Mapper;
  int LayerNumber;
  svtkProperty2D* Property;
  svtkCoordinate* PositionCoordinate;
  svtkCoordinate* Position2Coordinate;

private:
  svtkActor2D(const svtkActor2D&) = delete;
  void operator=(const svtkActor2D&) = delete;
};

#endif
