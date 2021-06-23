/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTerrainDataPointPlacer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTerrainDataPointPlacer
 * @brief   Place points on terrain data
 *
 *
 * svtkTerrainDataPointPlacer dictates the placement of points on height field
 * data. The class takes as input the list of props that represent the terrain
 * in a rendered scene. A height offset can be specified to dicatate the
 * placement of points at a certain height above the surface.
 *
 * @par Usage:
 * A typical usage of this class is as follows:
 * \code
 * pointPlacer->AddProp(demActor);    // the actor(s) containing the terrain.
 * rep->SetPointPlacer(pointPlacer);
 * pointPlacer->SetHeightOffset( 100 );
 * \endcode
 *
 * @sa
 * svtkPointPlacer svtkTerrainContourLineInterpolator
 */

#ifndef svtkTerrainDataPointPlacer_h
#define svtkTerrainDataPointPlacer_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkPointPlacer.h"

class svtkPropCollection;
class svtkProp;
class svtkPropPicker;

class SVTKINTERACTIONWIDGETS_EXPORT svtkTerrainDataPointPlacer : public svtkPointPlacer
{
public:
  /**
   * Instantiate this class.
   */
  static svtkTerrainDataPointPlacer* New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkTerrainDataPointPlacer, svtkPointPlacer);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  // Descuription:
  // Add an actor (that represents a terrain in a rendererd scene) to the
  // list. Only props in this list are considered by the PointPlacer
  virtual void AddProp(svtkProp*);
  virtual void RemoveAllProps();

  //@{
  /**
   * This is the height above (or below) the terrain that the dictated
   * point should be placed. Positive values indicate distances above the
   * terrain; negative values indicate distances below the terrain. The
   * default is 0.0.
   */
  svtkSetMacro(HeightOffset, double);
  svtkGetMacro(HeightOffset, double);
  //@}

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
  svtkTerrainDataPointPlacer();
  ~svtkTerrainDataPointPlacer() override;

  // The props that represents the terrain data (one or more) in a rendered
  // scene
  svtkPropCollection* TerrainProps;
  svtkPropPicker* PropPicker;
  double HeightOffset;

private:
  svtkTerrainDataPointPlacer(const svtkTerrainDataPointPlacer&) = delete;
  void operator=(const svtkTerrainDataPointPlacer&) = delete;
};

#endif
