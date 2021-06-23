/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSurfaceReconstructionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSurfaceReconstructionFilter
 * @brief   reconstructs a surface from unorganized points
 *
 * svtkSurfaceReconstructionFilter takes a list of points assumed to lie on
 * the surface of a solid 3D object. A signed measure of the distance to the
 * surface is computed and sampled on a regular grid. The grid can then be
 * contoured at zero to extract the surface. The default values for
 * neighborhood size and sample spacing should give reasonable results for
 * most uses but can be set if desired. This procedure is based on the PhD
 * work of Hugues Hoppe: http://www.research.microsoft.com/~hoppe
 */

#ifndef svtkSurfaceReconstructionFilter_h
#define svtkSurfaceReconstructionFilter_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingHybridModule.h" // For export macro

class SVTKIMAGINGHYBRID_EXPORT svtkSurfaceReconstructionFilter : public svtkImageAlgorithm
{
public:
  svtkTypeMacro(svtkSurfaceReconstructionFilter, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct with NeighborhoodSize=20.
   */
  static svtkSurfaceReconstructionFilter* New();

  //@{
  /**
   * Specify the number of neighbors each point has, used for estimating the
   * local surface orientation.  The default value of 20 should be OK for
   * most applications, higher values can be specified if the spread of
   * points is uneven. Values as low as 10 may yield adequate results for
   * some surfaces. Higher values cause the algorithm to take longer. Higher
   * values will cause errors on sharp boundaries.
   */
  svtkGetMacro(NeighborhoodSize, int);
  svtkSetMacro(NeighborhoodSize, int);
  //@}

  //@{
  /**
   * Specify the spacing of the 3D sampling grid. If not set, a
   * reasonable guess will be made.
   */
  svtkGetMacro(SampleSpacing, double);
  svtkSetMacro(SampleSpacing, double);
  //@}

protected:
  svtkSurfaceReconstructionFilter();
  ~svtkSurfaceReconstructionFilter() override {}

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int NeighborhoodSize;
  double SampleSpacing;

  int FillInputPortInformation(int, svtkInformation*) override;

private:
  svtkSurfaceReconstructionFilter(const svtkSurfaceReconstructionFilter&) = delete;
  void operator=(const svtkSurfaceReconstructionFilter&) = delete;
};

#endif
