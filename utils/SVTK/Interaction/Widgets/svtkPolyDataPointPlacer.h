/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataPointPlacer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolyDataPointPlacer
 * @brief   Base class to place points given constraints on polygonal data
 *
 *
 * svtkPolyDataPointPlacer is a base class to place points on the surface of
 * polygonal data.
 *
 * @par Usage:
 * The actors that render polygonal data and wish to be considered
 * for placement by this placer are added to the list as
 * \code
 * placer->AddProp( polyDataActor );
 * \endcode
 *
 * @sa
 * svtkPolygonalSurfacePointPlacer
 */

#ifndef svtkPolyDataPointPlacer_h
#define svtkPolyDataPointPlacer_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkPointPlacer.h"

class svtkRenderer;
class svtkPropCollection;
class svtkProp;
class svtkPropPicker;

class SVTKINTERACTIONWIDGETS_EXPORT svtkPolyDataPointPlacer : public svtkPointPlacer
{
public:
  /**
   * Instantiate this class.
   */
  static svtkPolyDataPointPlacer* New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkPolyDataPointPlacer, svtkPointPlacer);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  // Descuription:
  // Add an actor (that represents a terrain in a rendererd scene) to the
  // list. Only props in this list are considered by the PointPlacer
  virtual void AddProp(svtkProp*);
  virtual void RemoveViewProp(svtkProp* prop);
  virtual void RemoveAllProps();
  int HasProp(svtkProp*);
  int GetNumberOfProps();

  /**
   * Given a renderer and a display position in pixel coordinates,
   * compute the world position and orientation where this point
   * will be placed. This method is typically used by the
   * representation to place the point initially.
   * For the Terrain point placer this computes world points that
   * lie at the specified height above the terrain.
   */
  int ComputeWorldPosition(
    svtkRenderer* ren, double displayPos[2], double worldPos[3], double worldOrient[9]) override;

  /**
   * Given a renderer, a display position, and a reference world
   * position, compute the new world position and orientation
   * of this point. This method is typically used by the
   * representation to move the point.
   */
  int ComputeWorldPosition(svtkRenderer* ren, double displayPos[2], double refWorldPos[3],
    double worldPos[3], double worldOrient[9]) override;

  /**
   * Given a world position check the validity of this
   * position according to the constraints of the placer
   */
  int ValidateWorldPosition(double worldPos[3]) override;

  /**
   * Given a display position, check the validity of this position.
   */
  int ValidateDisplayPosition(svtkRenderer*, double displayPos[2]) override;

  /**
   * Given a world position and a world orientation,
   * validate it according to the constraints of the placer.
   */
  int ValidateWorldPosition(double worldPos[3], double worldOrient[9]) override;

  //@{
  /**
   * Get the Prop picker.
   */
  svtkGetObjectMacro(PropPicker, svtkPropPicker);
  //@}

protected:
  svtkPolyDataPointPlacer();
  ~svtkPolyDataPointPlacer() override;

  // The props that represents the terrain data (one or more) in a rendered
  // scene
  svtkPropCollection* SurfaceProps;
  svtkPropPicker* PropPicker;

private:
  svtkPolyDataPointPlacer(const svtkPolyDataPointPlacer&) = delete;
  void operator=(const svtkPolyDataPointPlacer&) = delete;
};

#endif
