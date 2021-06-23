/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGridTransform.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGridTransform
 * @brief   a nonlinear warp transformation
 *
 * svtkGridTransform describes a nonlinear warp transformation as a set
 * of displacement vectors sampled along a uniform 3D grid.
 * @warning
 * The inverse grid transform is calculated using an iterative method,
 * and is several times more expensive than the forward transform.
 * @sa
 * svtkThinPlateSplineTransform svtkGeneralTransform svtkTransformToGrid
 */

#ifndef svtkGridTransform_h
#define svtkGridTransform_h

#include "svtkFiltersHybridModule.h" // For export macro
#include "svtkWarpTransform.h"

class svtkAlgorithmOutput;
class svtkGridTransformConnectionHolder;
class svtkImageData;

#define SVTK_GRID_NEAREST SVTK_NEAREST_INTERPOLATION
#define SVTK_GRID_LINEAR SVTK_LINEAR_INTERPOLATION
#define SVTK_GRID_CUBIC SVTK_CUBIC_INTERPOLATION

class SVTKFILTERSHYBRID_EXPORT svtkGridTransform : public svtkWarpTransform
{
public:
  static svtkGridTransform* New();
  svtkTypeMacro(svtkGridTransform, svtkWarpTransform);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the grid transform (the grid transform must have three
   * components for displacement in x, y, and z respectively).
   * The svtkGridTransform class will never modify the data.
   * Note that SetDisplacementGridData() does not setup a pipeline
   * connection whereas SetDisplacementGridConnection does.
   */
  virtual void SetDisplacementGridConnection(svtkAlgorithmOutput*);
  virtual void SetDisplacementGridData(svtkImageData*);
  virtual svtkImageData* GetDisplacementGrid();
  //@}

  //@{
  /**
   * Set scale factor to be applied to the displacements.
   * This is used primarily for grids which contain integer
   * data types.  Default: 1
   */
  svtkSetMacro(DisplacementScale, double);
  svtkGetMacro(DisplacementScale, double);
  //@}

  //@{
  /**
   * Set a shift to be applied to the displacements.  The shift
   * is applied after the scale, i.e. x = scale*y + shift.
   * Default: 0
   */
  svtkSetMacro(DisplacementShift, double);
  svtkGetMacro(DisplacementShift, double);
  //@}

  //@{
  /**
   * Set interpolation mode for sampling the grid.  Higher-order
   * interpolation allows you to use a sparser grid.
   * Default: Linear.
   */
  void SetInterpolationMode(int mode);
  svtkGetMacro(InterpolationMode, int);
  void SetInterpolationModeToNearestNeighbor()
  {
    this->SetInterpolationMode(SVTK_NEAREST_INTERPOLATION);
  }
  void SetInterpolationModeToLinear() { this->SetInterpolationMode(SVTK_LINEAR_INTERPOLATION); }
  void SetInterpolationModeToCubic() { this->SetInterpolationMode(SVTK_CUBIC_INTERPOLATION); }
  const char* GetInterpolationModeAsString();
  //@}

  /**
   * Make another transform of the same type.
   */
  svtkAbstractTransform* MakeTransform() override;

  /**
   * Get the MTime.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkGridTransform();
  ~svtkGridTransform() override;

  /**
   * Update the displacement grid.
   */
  void InternalUpdate() override;

  /**
   * Copy this transform from another of the same type.
   */
  void InternalDeepCopy(svtkAbstractTransform* transform) override;

  //@{
  /**
   * Internal functions for calculating the transformation.
   */
  void ForwardTransformPoint(const float in[3], float out[3]) override;
  void ForwardTransformPoint(const double in[3], double out[3]) override;
  //@}

  void ForwardTransformDerivative(const float in[3], float out[3], float derivative[3][3]) override;
  void ForwardTransformDerivative(
    const double in[3], double out[3], double derivative[3][3]) override;

  void InverseTransformPoint(const float in[3], float out[3]) override;
  void InverseTransformPoint(const double in[3], double out[3]) override;

  void InverseTransformDerivative(const float in[3], float out[3], float derivative[3][3]) override;
  void InverseTransformDerivative(
    const double in[3], double out[3], double derivative[3][3]) override;

  void (*InterpolationFunction)(double point[3], double displacement[3], double derivatives[3][3],
    void* gridPtr, int gridType, int inExt[6], svtkIdType inInc[3]);

  int InterpolationMode;
  double DisplacementScale;
  double DisplacementShift;

  void* GridPointer;
  int GridScalarType;
  double GridSpacing[3];
  double GridOrigin[3];
  int GridExtent[6];
  svtkIdType GridIncrements[3];

private:
  svtkGridTransform(const svtkGridTransform&) = delete;
  void operator=(const svtkGridTransform&) = delete;

  svtkGridTransformConnectionHolder* ConnectionHolder;
};

//----------------------------------------------------------------------------
inline const char* svtkGridTransform::GetInterpolationModeAsString()
{
  switch (this->InterpolationMode)
  {
    case SVTK_GRID_NEAREST:
      return "NearestNeighbor";
    case SVTK_GRID_LINEAR:
      return "Linear";
    case SVTK_GRID_CUBIC:
      return "Cubic";
    default:
      return "";
  }
}

#endif
