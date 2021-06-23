/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVoxelGrid.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVoxelGrid
 * @brief   subsample points using uniform binning
 *
 *
 * svtkVoxelGrid is a filter that subsamples a point cloud based on a regular
 * binning of space. Basically the algorithm operates by dividing space into
 * a volume of M x N x O bins, and then for each bin averaging all of the
 * points positions into a single representative point. Several strategies for
 * computing the binning can be used: 1) manual configuration of a requiring
 * specifying bin dimensions (the bounds are calculated from the data); 2) by
 * explicit specification of the bin size in world coordinates (x-y-z
 * lengths); and 3) an automatic process in which the user specifies an
 * approximate, average number of points per bin and dimensions and bin size
 * are computed automatically. (Note that under the hood a
 * svtkStaticPointLocator is used.)
 *
 * While any svtkPointSet type can be provided as input, the output is
 * represented by an explicit representation of points via a
 * svtkPolyData. This output polydata will populate its instance of svtkPoints,
 * but no cells will be defined (i.e., no svtkVertex or svtkPolyVertex are
 * contained in the output).
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkStaticPointLocator svtkPointCloudFilter svtkQuadricClustering
 */

#ifndef svtkVoxelGrid_h
#define svtkVoxelGrid_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkStaticPointLocator;
class svtkInterpolationKernel;

class SVTKFILTERSPOINTS_EXPORT svtkVoxelGrid : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static svtkVoxelGrid* New();
  svtkTypeMacro(svtkVoxelGrid, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * This enum is used to configure the operation of the filter.
   */
  enum Style
  {
    MANUAL = 0,
    SPECIFY_LEAF_SIZE = 1,
    AUTOMATIC = 2
  };

  //@{
  /**
   * Configure how the filter is to operate. The user can choose to manually
   * specify the binning volume (by setting its dimensions via MANUAL style); or
   * specify a leaf bin size in the x-y-z directions (SPECIFY_LEAF_SIZE); or
   * in AUTOMATIC style, use a rough average number of points in each bin
   * guide the bin size and binning volume dimensions. By default, AUTOMATIC
   * configuration style is used.
   */
  svtkSetMacro(ConfigurationStyle, int);
  svtkGetMacro(ConfigurationStyle, int);
  void SetConfigurationStyleToManual() { this->SetConfigurationStyle(MANUAL); }
  void SetConfigurationStyleToLeafSize() { this->SetConfigurationStyle(SPECIFY_LEAF_SIZE); }
  void SetConfigurationStyleToAutomatic() { this->SetConfigurationStyle(AUTOMATIC); }
  //@}

  //@{
  /**
   * Set the number of divisions in x-y-z directions (the binning volume
   * dimensions). This data member is used when the configuration style is
   * set to MANUAL. Note that these values may be adjusted if <1 or too
   * large.
   */
  svtkSetVector3Macro(Divisions, int);
  svtkGetVectorMacro(Divisions, int, 3);
  //@}

  //@{
  /**
   * Set the bin size in the x-y-z directions. This data member is
   * used when the configuration style is set to SPECIFY_LEAF_SIZE. The class will
   * use these x-y-z lengths, within the bounding box of the point cloud,
   * to determine the binning dimensions.
   */
  svtkSetVector3Macro(LeafSize, double);
  svtkGetVectorMacro(LeafSize, double, 3);
  //@}

  //@{
  /**
   * Specify the average number of points in each bin. Larger values
   * result in higher rates of subsampling. This data member is used when the
   * configuration style is set to AUTOMATIC. The class will automatically
   * determine the binning dimensions in the x-y-z directions.
   */
  svtkSetClampMacro(NumberOfPointsPerBin, int, 1, SVTK_INT_MAX);
  svtkGetMacro(NumberOfPointsPerBin, int);
  //@}

  //@{
  /**
   * Specify an interpolation kernel to combine the point attributes. By
   * default a svtkLinearKernel is used (i.e., average values). The
   * interpolation kernel changes the basis of the interpolation.
   */
  void SetKernel(svtkInterpolationKernel* kernel);
  svtkGetObjectMacro(Kernel, svtkInterpolationKernel);
  //@}

protected:
  svtkVoxelGrid();
  ~svtkVoxelGrid() override;

  svtkStaticPointLocator* Locator;
  int ConfigurationStyle;

  int Divisions[3];
  double LeafSize[3];
  int NumberOfPointsPerBin;
  svtkInterpolationKernel* Kernel;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkVoxelGrid(const svtkVoxelGrid&) = delete;
  void operator=(const svtkVoxelGrid&) = delete;
};

#endif
