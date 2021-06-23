/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMaskPointsFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMaskPointsFilter
 * @brief   extract points within an image/volume mask
 *
 * svtkMaskPointsFilter extracts points that are inside an image mask. The
 * image mask is a second input to the filter. Points that are inside a voxel
 * marked "inside" are copied to the output. The image mask can be generated
 * by svtkPointOccupancyFilter, with optional image processing steps performed
 * on the mask. Thus svtkPointOccupancyFilter and svtkMaskPointsFilter are
 * generally used together, with a pipeline of image processing algorithms
 * in between the two filters.
 *
 * Note also that this filter is a subclass of svtkPointCloudFilter which has
 * the ability to produce an output mask indicating which points were
 * selected for output. It also has an optional second output containing the
 * points that were masked out (i.e., outliers) during processing.
 *
 * Finally, the mask value indicating non-selection of points (i.e., the
 * empty value) may be specified. The second input, masking image, is
 * typically of type unsigned char so the empty value is of this type as
 * well.
 *
 * @warning
 * During processing, points not within the masking image/volume are
 * considered outside and never extracted.
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkPointOccupancyFilter svtkPointCloudFilter
 */

#ifndef svtkMaskPointsFilter_h
#define svtkMaskPointsFilter_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkPointCloudFilter.h"

class svtkImageData;
class svtkPointSet;

class SVTKFILTERSPOINTS_EXPORT svtkMaskPointsFilter : public svtkPointCloudFilter
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static svtkMaskPointsFilter* New();
  svtkTypeMacro(svtkMaskPointsFilter, svtkPointCloudFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the masking image. It must be of type svtkImageData.
   */
  void SetMaskData(svtkDataObject* source);
  svtkDataObject* GetMask();
  //@}

  /**
   * Specify the masking image. It is svtkImageData output from an algorithm.
   */
  void SetMaskConnection(svtkAlgorithmOutput* algOutput);

  //@{
  /**
   * Set / get the values indicating whether a voxel is empty. By default, an
   * empty voxel is marked with a zero value. Any point inside a voxel marked
   * empty is not selected for output. All other voxels with a value that is
   * not equal to the empty value are selected for output.
   */
  svtkSetMacro(EmptyValue, unsigned char);
  svtkGetMacro(EmptyValue, unsigned char);
  //@}

protected:
  svtkMaskPointsFilter();
  ~svtkMaskPointsFilter() override;

  unsigned char EmptyValue; // what value indicates a voxel is empty

  // All derived classes must implement this method. Note that a side effect of
  // the class is to populate the PointMap. Zero is returned if there is a failure.
  int FilterPoints(svtkPointSet* input) override;

  // Support second input
  int FillInputPortInformation(int port, svtkInformation* info) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  svtkImageData* Mask; // just a placeholder during execution

private:
  svtkMaskPointsFilter(const svtkMaskPointsFilter&) = delete;
  void operator=(const svtkMaskPointsFilter&) = delete;
};

#endif
