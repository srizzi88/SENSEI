/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnsignedDistance.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkUnsignedDistance
 * @brief   compute unsigned (i.e., non-negative) distances from an input point cloud
 *
 * svtkUnsignedDistance is a filter that computes non-negative (i.e., unsigned)
 * distances over a volume from an input point cloud. This filter is distinct
 * from svtkSignedDistance in that it does not require point normals. However,
 * isocontouring a zero-valued distance function (e.g., trying to fit a
 * surface will produce unsatisfactory results). Rather this filter, when
 * combined with an isocontouring filter such as svtkFlyingEdges3D, can
 * produce an offset, bounding surface surrounding the input point cloud.
 *
 * To use this filter, specify the input svtkPolyData (which represents the
 * point cloud); define the sampling volume; specify a radius (which limits
 * the radius of influence of each point); and set an optional point locator
 * (to accelerate proximity operations, a svtkStaticPointLocator is used by
 * default). Note that large radius values may have significant impact on
 * performance. The volume is defined by specifying dimensions in the x-y-z
 * directions, as well as a domain bounds. By default the model bounds are
 * defined from the input points, but the user can also manually specify
 * them. Finally, because the radius data member limits the influence of the
 * distance calculation, some voxels may receive no contribution. These voxel
 * values are set to the CapValue.
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
 * @warning
 * Note that multiple, non-connected surfaces may be produced. For example,
 * if the point cloud is from the surface of the sphere, it is possible to
 * generate two surfaces (with isocontouring): one inside the sphere, one
 * outside the sphere. It is sometimes possible to select the surface you
 * want from the output of the contouring filter by using
 * svtkPolyDataConnectivityFilter.
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkSignedDistance svtkExtractSurface svtkImplicitModeller
 */

#ifndef svtkUnsignedDistance_h
#define svtkUnsignedDistance_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkImageAlgorithm.h"

class svtkPolyData;
class svtkAbstractPointLocator;

class SVTKFILTERSPOINTS_EXPORT svtkUnsignedDistance : public svtkImageAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiating the class, providing type information,
   * and printing.
   */
  static svtkUnsignedDistance* New();
  svtkTypeMacro(svtkUnsignedDistance, svtkImageAlgorithm);
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
   * Control how the model bounds are computed. If the ivar AdjustBounds
   * is set, then the bounds specified (or computed automatically) is modified
   * by the fraction given by AdjustDistance. This means that the model
   * bounds is expanded in each of the x-y-z directions.
   */
  svtkSetMacro(AdjustBounds, svtkTypeBool);
  svtkGetMacro(AdjustBounds, svtkTypeBool);
  svtkBooleanMacro(AdjustBounds, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the amount to grow the model bounds (if the ivar AdjustBounds
   * is set). The value is a fraction of the maximum length of the sides
   * of the box specified by the model bounds.
   */
  svtkSetClampMacro(AdjustDistance, double, -1.0, 1.0);
  svtkGetMacro(AdjustDistance, double);
  //@}

  //@{
  /**
   * Set / get the radius of influence of each point. Smaller values
   * generally improve performance markedly.
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

  //@{
  /**
   * The outer boundary of the volume can be assigned a particular value
   * after distances are computed. This can be used to close or "cap" all
   * surfaces during isocontouring.
   */
  svtkSetMacro(Capping, svtkTypeBool);
  svtkGetMacro(Capping, svtkTypeBool);
  svtkBooleanMacro(Capping, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the capping value to use. The CapValue is also used as an
   * initial distance value at each point in the dataset. By default, the
   * CapValue is SVTK_FLOAT_MAX;
   */
  svtkSetMacro(CapValue, double);
  svtkGetMacro(CapValue, double);
  //@}

  //@{
  /**
   * Set the desired output scalar type. Currently only real types are
   * supported. By default, SVTK_FLOAT scalars are created.
   */
  svtkSetMacro(OutputScalarType, int);
  svtkGetMacro(OutputScalarType, int);
  void SetOutputScalarTypeToFloat() { this->SetOutputScalarType(SVTK_FLOAT); }
  void SetOutputScalarTypeToDouble() { this->SetOutputScalarType(SVTK_DOUBLE); }
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
  svtkUnsignedDistance();
  ~svtkUnsignedDistance() override;

  int Dimensions[3];
  double Bounds[6];
  svtkTypeBool AdjustBounds;
  double AdjustDistance;
  double Radius;
  svtkAbstractPointLocator* Locator;
  svtkTypeBool Capping;
  double CapValue;
  int OutputScalarType;

  // Flag tracks whether process needs initialization
  int Initialized;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

private:
  svtkUnsignedDistance(const svtkUnsignedDistance&) = delete;
  void operator=(const svtkUnsignedDistance&) = delete;
};

#endif
