/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSignedDistance.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSignedDistance
 * @brief   compute signed distances from an input point cloud
 *
 * svtkSignedDistance is a filter that computes signed distances over a volume
 * from an input point cloud. The input point cloud must have point normals
 * defined, as well as an optional weighting function (e.g., probabilities
 * that the point measurements are accurate). Once the signed distance
 * function is computed, then the output volume may be isocontoured to with
 * svtkExtractSurface to extract a approximating surface to the point cloud.
 *
 * To use this filter, specify the input svtkPolyData (which represents the
 * point cloud); define the sampling volume; specify a radius (which limits
 * the radius of influence of each point); and set an optional point locator
 * (to accelerate proximity operations, a svtkStaticPointLocator is used by
 * default). Note that large radius values may have significant impact on
 * performance. The volume is defined by specifying dimensions in the x-y-z
 * directions, as well as a domain bounds. By default the model bounds are
 * defined from the input points, but the user can also manually specify
 * them.
 *
 * This filter has one other unusual capability: it is possible to append
 * data in a sequence of operations to generate a single output. This is
 * useful when you have multiple point clouds (e.g., possibly from multiple
 * acqusition scans) and want to incrementally accumulate all the data.
 * However, the user must be careful to either specify the Bounds or
 * order the input such that the bounds of the first input completely
 * contains all other input data.  This is because the geometry and topology
 * of the output sampling volume cannot be changed after the initial Append
 * operation.
 *
 * This algorithm loosely follows the most excellent paper by Curless and
 * Levoy: "A Volumetric Method for Building Complex Models from Range
 * Images." As described in this paper it may produce a signed distance
 * volume that may contain the three data states for each voxel: near
 * surface, empty, or unseen (see svtkExtractSurface for additional
 * information). Note in this implementation the initial values of the volume
 * are set to < this->Radius. This indicates that these voxels are
 * "empty". Of course voxels with value -this->Radius <= d <= this->Radius
 * are "near" the surface. (Voxels with values > this->Radius are "unseen" --
 * this filter does not produce such values.)
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @warning
 * Empty voxel values are set to -this->Radius.
 *
 * @sa
 * svtkExtractSurface svtkImplicitModeller
 */

#ifndef svtkSignedDistance_h
#define svtkSignedDistance_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkImageAlgorithm.h"

class svtkPolyData;
class svtkAbstractPointLocator;

class SVTKFILTERSPOINTS_EXPORT svtkSignedDistance : public svtkImageAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiating the class, providing type information,
   * and printing.
   */
  static svtkSignedDistance* New();
  svtkTypeMacro(svtkSignedDistance, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Set/Get the i-j-k dimensions on which to computer the distance function.
   */
  svtkGetVectorMacro(Dimensions, int, 3);
  void SetDimensions(int i, int j, int k);
  void SetDimensions(const int dim[3]);
  //@}

  //@{
  /**
   * Set / get the region in space in which to perform the sampling. If
   * not specified, it will be computed automatically.
   */
  svtkSetVector6Macro(Bounds, double);
  svtkGetVectorMacro(Bounds, double, 6);
  //@}

  //@{
  /**
   * Set / get the radius of influence of each point. Smaller values
   * generally improve performance markedly. Note that after the signed
   * distance function is computed, any voxel taking on the value >= Radius
   * is presumed to be "unseen" or uninitialized.
   */
  svtkSetClampMacro(Radius, double, 0.0, SVTK_FLOAT_MAX);
  svtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Specify a point locator. By default a svtkStaticPointLocator is
   * used. The locator performs efficient searches to locate points
   * surrounding a voxel (within the specified radius).
   */
  void SetLocator(svtkAbstractPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkAbstractPointLocator);
  //@}

  /**
   * Initialize the filter for appending data. You must invoke the
   * StartAppend() method before doing successive Appends(). It's also a
   * good idea to manually specify the model bounds; otherwise the input
   * bounds for the data will be used.
   */
  void StartAppend();

  /**
   * Append a data set to the existing output. To use this function,
   * you'll have to invoke the StartAppend() method before doing
   * successive appends. It's also a good idea to specify the model
   * bounds; otherwise the input model bounds is used. When you've
   * finished appending, use the EndAppend() method.
   */
  void Append(svtkPolyData* input);

  /**
   * Method completes the append process.
   */
  void EndAppend();

  // See the svtkAlgorithm for a description of what these do
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkSignedDistance();
  ~svtkSignedDistance() override;

  int Dimensions[3];
  double Bounds[6];
  double Radius;
  svtkAbstractPointLocator* Locator;

  // Flag tracks whether process needs initialization
  int Initialized;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

private:
  svtkSignedDistance(const svtkSignedDistance&) = delete;
  void operator=(const svtkSignedDistance&) = delete;
};

#endif
