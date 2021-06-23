/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMRSliceFilter.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkAMRSliceFilter
 *
 *
 *  A concrete instance of svtkOverlappingAMRAlgorithm which implements
 *  functionality for extracting slices from AMR data. Unlike the conventional
 *  slice filter, the output of this filter is a 2-D AMR dataset itself.
 */

#ifndef svtkAMRSliceFilter_h
#define svtkAMRSliceFilter_h

#include "svtkFiltersAMRModule.h" // For export macro
#include "svtkOverlappingAMRAlgorithm.h"

#include <vector> // For STL vector

class svtkInformation;
class svtkInformationVector;
class svtkOverlappingAMR;
class svtkMultiProcessController;
class svtkPlane;
class svtkAMRBox;
class svtkUniformGrid;

class SVTKFILTERSAMR_EXPORT svtkAMRSliceFilter : public svtkOverlappingAMRAlgorithm
{
public:
  static svtkAMRSliceFilter* New();
  svtkTypeMacro(svtkAMRSliceFilter, svtkOverlappingAMRAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // Inline Gettters & Setters

  //@{
  /**
   * Set/Get the offset-from-origin of the slicing plane.
   */
  svtkSetMacro(OffsetFromOrigin, double);
  svtkGetMacro(OffsetFromOrigin, double);
  //@}

  //@{
  /**
   * Set/Get the maximum resolution used in this instance.
   */
  svtkSetMacro(MaxResolution, unsigned int);
  svtkGetMacro(MaxResolution, unsigned int);
  //@}

  /**
   * Tags to identify normals along the X, Y and Z directions.
   */
  enum NormalTag : char
  {
    X_NORMAL = 1,
    Y_NORMAL = 2,
    Z_NORMAL = 4
  };

  //@{
  /**
   * Set/Get the Axis normal. The accpetable values are defined in the
   * NormalTag enum.
   */
  svtkSetMacro(Normal, int);
  svtkGetMacro(Normal, int);
  //@}

  //@{
  /**
   * Set/Get a multiprocess controller for paralle processing.
   * By default this parameter is set to nullptr by the constructor.
   */
  svtkSetMacro(Controller, svtkMultiProcessController*);
  svtkGetMacro(Controller, svtkMultiProcessController*);
  //@}

  // Standard Pipeline methods
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  /**
   * Makes upstream request to a source, typically, a concrete instance of
   * svtkAMRBaseReader, for which blocks to load.
   */
  int RequestInformation(svtkInformation* rqst, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Performs upstream requests to the reader
   */
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkAMRSliceFilter();
  ~svtkAMRSliceFilter() override = default;

  /**
   * Returns the cell index w.r.t. the given input grid which contains
   * the query point x. A -1 is returned if the point is not found.
   */
  int GetDonorCellIdx(double x[3], svtkUniformGrid* ug);

  /**
   * Returns the point index w.r.t. the given input grid which contains the
   * query point x. A -1 is returned if the point is not found.
   */
  int GetDonorPointIdx(double x[3], svtkUniformGrid* ug);

  /**
   * Computes the cell center of the cell corresponding to the supplied
   * cell index w.r.t. the input uniform grid.
   */
  void ComputeCellCenter(svtkUniformGrid* ug, const int cellIdx, double centroid[3]);

  /**
   * Gets the slice from the given grid given the plane origin & the
   * user-supplied normal associated with this class instance.
   */
  svtkUniformGrid* GetSlice(double origin[3], int* dims, double* gorigin, double* spacing);

  /**
   * Copies the cell data for the cells in the slice from the 3-D grid.
   */
  void GetSliceCellData(svtkUniformGrid* slice, svtkUniformGrid* grid3D);

  /**
   * Copies the point data for the cells in the slice from the 3-D grid.
   */
  void GetSlicePointData(svtkUniformGrid* slice, svtkUniformGrid* grid3D);

  /**
   * Determines if a plane intersects with an AMR box
   */
  bool PlaneIntersectsAMRBox(double plane[4], double bounds[6]);

  /**
   * Given the cut-plane and the metadata provided by a module upstream,
   * this method generates the list of linear AMR block indices that need
   * to be loaded.
   */
  void ComputeAMRBlocksToLoad(svtkPlane* p, svtkOverlappingAMR* metadata);

  /**
   * Extracts a 2-D AMR slice from the dataset.
   */
  void GetAMRSliceInPlane(svtkPlane* p, svtkOverlappingAMR* inp, svtkOverlappingAMR* out);

  /**
   * A utility function that checks if the input AMR data is 2-D.
   */
  bool IsAMRData2D(svtkOverlappingAMR* input);

  /**
   * Returns the axis-aligned cut plane.
   */
  svtkPlane* GetCutPlane(svtkOverlappingAMR* input);

  double OffsetFromOrigin;
  int Normal;
  unsigned int MaxResolution;
  svtkMultiProcessController* Controller;

  std::vector<int> BlocksToLoad;

private:
  svtkAMRSliceFilter(const svtkAMRSliceFilter&) = delete;
  void operator=(const svtkAMRSliceFilter&) = delete;
};

#endif /* svtkAMRSliceFilter_h */
