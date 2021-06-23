/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractRectilinearGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractRectilinearGrid
 * @brief   Extract a sub grid (VOI) from the structured rectilinear dataset.
 *
 * svtkExtractRectilinearGrid rounds out the set of filters that extract
 * a subgrid out of a larger structured data set.  RIght now, this filter
 * only supports extracting a VOI.  In the future, it might support
 * strides like the svtkExtract grid filter.
 *
 * @sa
 * svtkExtractGrid svtkImageClip svtkGeometryFilter svtkExtractGeometry svtkExtractVOI
 * svtkStructuredGridGeometryFilter
 */

#ifndef svtkExtractRectilinearGrid_h
#define svtkExtractRectilinearGrid_h

#include "svtkFiltersExtractionModule.h" // For export macro
#include "svtkRectilinearGridAlgorithm.h"

// Forward Declarations
class svtkExtractStructuredGridHelper;

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractRectilinearGrid : public svtkRectilinearGridAlgorithm
{
public:
  static svtkExtractRectilinearGrid* New();
  svtkTypeMacro(svtkExtractRectilinearGrid, svtkRectilinearGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify i-j-k (min,max) pairs to extract. The resulting structured grid
   * dataset can be of any topological dimension (i.e., point, line, plane,
   * or 3D grid).
   */
  svtkSetVector6Macro(VOI, int);
  svtkGetVectorMacro(VOI, int, 6);
  //@}

  //@{
  /**
   * Set the sampling rate in the i, j, and k directions. If the rate is > 1,
   * then the resulting VOI will be subsampled representation of the input.
   * For example, if the SampleRate=(2,2,2), every other point will be
   * selected, resulting in a volume 1/8th the original size.
   * Initial value is (1,1,1).
   */
  svtkSetVector3Macro(SampleRate, int);
  svtkGetVectorMacro(SampleRate, int, 3);
  //@}

  //@{
  /**
   * Control whether to enforce that the "boundary" of the grid is output in
   * the subsampling process. (This ivar only has effect when the SampleRate
   * in any direction is not equal to 1.) When this ivar IncludeBoundary is
   * on, the subsampling will always include the boundary of the grid even
   * though the sample rate is not an even multiple of the grid
   * dimensions. (By default IncludeBoundary is off.)
   */
  svtkSetMacro(IncludeBoundary, svtkTypeBool);
  svtkGetMacro(IncludeBoundary, svtkTypeBool);
  svtkBooleanMacro(IncludeBoundary, svtkTypeBool);
  //@}

protected:
  svtkExtractRectilinearGrid();
  ~svtkExtractRectilinearGrid() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Implementation for RequestData using a specified VOI. This is because the
   * parallel filter needs to muck around with the VOI to get spacing and
   * partitioning to play nice. The VOI is calculated from the output
   * data object's extents in this implementation.
   */
  bool RequestDataImpl(svtkInformationVector** inputVector, svtkInformationVector* outputVector);

  int VOI[6];
  int SampleRate[3];
  svtkTypeBool IncludeBoundary;

  svtkExtractStructuredGridHelper* Internal;

private:
  svtkExtractRectilinearGrid(const svtkExtractRectilinearGrid&) = delete;
  void operator=(const svtkExtractRectilinearGrid&) = delete;
};

#endif
