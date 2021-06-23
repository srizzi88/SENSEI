/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBSplineTransform.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBSplineTransform
 * @brief   a cubic b-spline deformation transformation
 *
 * svtkBSplineTransform computes a cubic b-spline transformation from a
 * grid of b-spline coefficients.
 * @warning
 * The inverse grid transform is calculated using an iterative method,
 * and is several times more expensive than the forward transform.
 * @sa
 * svtkGeneralTransform svtkTransformToGrid svtkImageBSplineCoefficients
 * @par Thanks:
 * This class was written by David Gobbi at the Seaman Family MR Research
 * Centre, Foothills Medical Centre, Calgary, Alberta.
 * DG Gobbi and YP Starreveld,
 * "Uniform B-Splines for the SVTK Imaging Pipeline,"
 * SVTK Journal, 2011,
 * http://hdl.handle.net/10380/3252
 */

#ifndef svtkBSplineTransform_h
#define svtkBSplineTransform_h

#include "svtkFiltersHybridModule.h" // For export macro
#include "svtkWarpTransform.h"

class svtkAlgorithmOutput;
class svtkBSplineTransformConnectionHolder;
class svtkImageData;

#define SVTK_BSPLINE_EDGE 0
#define SVTK_BSPLINE_ZERO 1
#define SVTK_BSPLINE_ZERO_AT_BORDER 2

class SVTKFILTERSHYBRID_EXPORT svtkBSplineTransform : public svtkWarpTransform
{
public:
  static svtkBSplineTransform* New();
  svtkTypeMacro(svtkBSplineTransform, svtkWarpTransform);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the coefficient grid for the b-spline transform.
   * The svtkBSplineTransform class will never modify the data.
   * Note that SetCoefficientData() does not setup a pipeline
   * connection whereas SetCoefficientConnection does.
   */
  virtual void SetCoefficientConnection(svtkAlgorithmOutput*);
  virtual void SetCoefficientData(svtkImageData*);
  virtual svtkImageData* GetCoefficientData();
  //@}

  //@{
  /**
   * Set/Get a scale to apply to the transformation.
   */
  svtkSetMacro(DisplacementScale, double);
  svtkGetMacro(DisplacementScale, double);
  //@}

  //@{
  /**
   * Set/Get the border mode, to alter behavior at the edge of the grid.
   * The Edge mode allows the displacement to converge to the edge
   * coefficient past the boundary, which is similar to the behavior of
   * the svtkGridTransform. The Zero mode allows the displacement to
   * smoothly converge to zero two node-spacings past the boundary,
   * which is useful when you want to create a localized transform.
   * The ZeroAtBorder mode sacrifices smoothness to further localize
   * the transform to just one node-spacing past the boundary.
   */
  svtkSetClampMacro(BorderMode, int, SVTK_BSPLINE_EDGE, SVTK_BSPLINE_ZERO_AT_BORDER);
  void SetBorderModeToEdge() { this->SetBorderMode(SVTK_BSPLINE_EDGE); }
  void SetBorderModeToZero() { this->SetBorderMode(SVTK_BSPLINE_ZERO); }
  void SetBorderModeToZeroAtBorder() { this->SetBorderMode(SVTK_BSPLINE_ZERO_AT_BORDER); }
  svtkGetMacro(BorderMode, int);
  const char* GetBorderModeAsString();
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
  svtkBSplineTransform();
  ~svtkBSplineTransform() override;

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

  void (*CalculateSpline)(const double point[3], double displacement[3], double derivatives[3][3],
    void* gridPtr, int inExt[6], svtkIdType inInc[3], int borderMode);

  double DisplacementScale;
  int BorderMode;

  void* GridPointer;
  double GridSpacing[3];
  double GridOrigin[3];
  int GridExtent[6];
  svtkIdType GridIncrements[3];

private:
  svtkBSplineTransform(const svtkBSplineTransform&) = delete;
  void operator=(const svtkBSplineTransform&) = delete;

  svtkBSplineTransformConnectionHolder* ConnectionHolder;
};

#endif
