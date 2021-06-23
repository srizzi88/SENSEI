/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCellCentersPointPlacer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCellCentersPointPlacer
 * @brief   Snaps points at the center of a cell
 *
 *
 * svtkCellCentersPointPlacer is a class to snap points on the center of cells.
 * The class has 3 modes. In the ParametricCenter mode, it snaps points
 * to the parametric center of the cell (see svtkCell). In CellPointsMean
 * mode, points are snapped to the mean of the points in the cell.
 * In 'None' mode, no snapping is performed. The computed world position
 * is the picked position within the cell.
 *
 * @par Usage:
 * The actors that render data and wish to be considered for placement
 * by this placer are added to the list as
 * \code
 * placer->AddProp( actor );
 * \endcode
 *
 * @sa
 * svtkPointPlacer
 */

#ifndef svtkCellCentersPointPlacer_h
#define svtkCellCentersPointPlacer_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkPointPlacer.h"

class svtkRenderer;
class svtkPropCollection;
class svtkProp;
class svtkCellPicker;

class SVTKINTERACTIONWIDGETS_EXPORT svtkCellCentersPointPlacer : public svtkPointPlacer
{
public:
  /**
   * Instantiate this class.
   */
  static svtkCellCentersPointPlacer* New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkCellCentersPointPlacer, svtkPointPlacer);
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
  svtkGetObjectMacro(CellPicker, svtkCellPicker);
  //@}

  //@{
  /**
   * Modes to change the point placement. Parametric center picks
   * the parametric center within the cell. CellPointsMean picks
   * the average of all points in the cell. When the mode is None,
   * the input point is passed through unmodified. Default is CellPointsMean.
   */
  svtkSetMacro(Mode, int);
  svtkGetMacro(Mode, int);
  //@}

  enum
  {
    ParametricCenter = 0,
    CellPointsMean,
    None
  };

protected:
  svtkCellCentersPointPlacer();
  ~svtkCellCentersPointPlacer() override;

  // The props that represents the terrain data (one or more) in a rendered
  // scene
  svtkPropCollection* PickProps;
  svtkCellPicker* CellPicker;
  int Mode;

private:
  svtkCellCentersPointPlacer(const svtkCellCentersPointPlacer&) = delete;
  void operator=(const svtkCellCentersPointPlacer&) = delete;
};

#endif
