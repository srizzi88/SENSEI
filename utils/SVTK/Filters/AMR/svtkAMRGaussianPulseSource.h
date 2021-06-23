/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMRGaussianPulseSource.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkAMRGaussianPulseSource
 *
 *
 *  A source that generates sample AMR data with gaussian pulse field. The user
 *  can control the refinement ratio as well as the pulse attributes such as
 *  the pulse origin, length and amplitude.
 *
 * @sa
 *  svtkOverlappingAMR
 */

#ifndef svtkAMRGaussianPulseSource_h
#define svtkAMRGaussianPulseSource_h

#include "svtkFiltersAMRModule.h" // For export macro
#include "svtkOverlappingAMRAlgorithm.h"

#include <cmath> // For std::exp

class svtkOverlappingAMR;
class svtkUniformGrid;
class svtkInformation;
class svtkInformationVector;

class SVTKFILTERSAMR_EXPORT svtkAMRGaussianPulseSource : public svtkOverlappingAMRAlgorithm
{
public:
  static svtkAMRGaussianPulseSource* New();
  svtkTypeMacro(svtkAMRGaussianPulseSource, svtkOverlappingAMRAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Sets the dimension of the AMR dataset to generate
   */
  svtkSetMacro(Dimension, int);
  //@}

  //@{
  /**
   * Sets the number of levels to generate
   */
  svtkSetMacro(NumberOfLevels, int);
  //@}

  /**
   * Set the refinement ratio
   */
  void SetRefinementRatio(int r)
  {
    this->RefinmentRatio = r;
    this->Modified();
  }

  //@{
  /**
   * Set the root spacing
   */
  void SetRootSpacing(double h0)
  {
    this->RootSpacing[0] = this->RootSpacing[1] = this->RootSpacing[2] = h0;
    this->Modified();
  }
  //@}

  //@{
  /**
   * Set & Get macro for the pulse origin
   */
  svtkSetVector3Macro(PulseOrigin, double);
  svtkGetVector3Macro(PulseOrigin, double);
  void SetXPulseOrigin(double f)
  {
    this->PulseOrigin[0] = f;
    this->Modified();
  }
  void SetYPulseOrigin(double f)
  {
    this->PulseOrigin[1] = f;
    this->Modified();
  }
  void SetZPulseOrigin(double f)
  {
    this->PulseOrigin[2] = f;
    this->Modified();
  }
  //@}

  //@{
  /**
   * Set & Get macro for the pulse width
   */
  svtkSetVector3Macro(PulseWidth, double);
  svtkGetVector3Macro(PulseWidth, double);
  void SetXPulseWidth(double f)
  {
    this->PulseWidth[0] = f;
    this->Modified();
  }
  void SetYPulseWidth(double f)
  {
    this->PulseWidth[1] = f;
    this->Modified();
  }
  void SetZPulseWidth(double f)
  {
    this->PulseWidth[2] = f;
    this->Modified();
  }
  //@}

  //@{
  /**
   * Set & Get macro for the pulse amplitude
   */
  svtkSetMacro(PulseAmplitude, double);
  svtkGetMacro(PulseAmplitude, double);
  //@}

protected:
  svtkAMRGaussianPulseSource();
  ~svtkAMRGaussianPulseSource() override;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  //@{
  /**
   * Computes the gaussian pulse at the given location based on the user
   * supplied parameters for pulse width and origin.
   */
  double ComputePulseAt(const double x, const double y, const double z)
  {
    double xyz[3];
    xyz[0] = x;
    xyz[1] = y;
    xyz[2] = z;
    return (this->ComputePulseAt(xyz));
  }
  double ComputePulseAt(double pt[3])
  {
    double pulse = 0.0;
    double r = 0.0;
    for (int i = 0; i < this->Dimension; ++i)
    {
      double d = pt[i] - this->PulseOrigin[i];
      double d2 = d * d;
      double L2 = this->PulseWidth[i] * this->PulseWidth[i];
      r += d2 / L2;
    }
    pulse = this->PulseAmplitude * std::exp(-r);
    return (pulse);
  }
  //@}

  /**
   * Given the cell index w.r.t. to a uniform grid, this method computes the
   * cartesian coordinates of the centroid of the cell.
   */
  void ComputeCellCenter(svtkUniformGrid* grid, svtkIdType cellIdx, double centroid[3]);

  /**
   * Generates a pulse field for the given uniform grid
   */
  void GeneratePulseField(svtkUniformGrid* grid);

  /**
   * Constructs a uniform grid path with the given origin/spacing and node
   * dimensions. The return grid serves as the root grid for the domain.
   */
  svtkUniformGrid* GetGrid(double origin[3], double h[3], int ndim[3]);

  /**
   * Constructs a refined patch from the given parent grid.
   */
  svtkUniformGrid* RefinePatch(svtkUniformGrid* parent, int patchExtent[6]);

  //@{
  /**
   * Generate 2-D or 3-D DataSet
   */
  void Generate2DDataSet(svtkOverlappingAMR* amr);
  void Generate3DDataSet(svtkOverlappingAMR* amr);
  //@}

  double RootSpacing[3];
  double PulseOrigin[3];
  double PulseWidth[3];
  double PulseAmplitude;
  int RefinmentRatio;
  int Dimension;
  int NumberOfLevels;

private:
  svtkAMRGaussianPulseSource(const svtkAMRGaussianPulseSource&) = delete;
  void operator=(const svtkAMRGaussianPulseSource&) = delete;
};

#endif /* svtkAMRGaussianPulseSource_h */
