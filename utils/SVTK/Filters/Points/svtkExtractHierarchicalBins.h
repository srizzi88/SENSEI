/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractHierarchicalBins.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractHierarchicalBins
 * @brief   manipulate the output of
 * svtkHierarchicalBinningFilter
 *
 *
 * svtkExtractHierarchicalBins enables users to extract data from the output
 * of svtkHierarchicalBinningFilter. Points at a particular level, or at a
 * level and bin number, can be filtered to the output. To perform these
 * operations, the output must contain points sorted into bins (the
 * svtkPoints), with offsets pointing to the beginning of each bin (a
 * svtkFieldData array named "BinOffsets").
 *
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkFiltersPointsFilter svtkRadiusOutlierRemoval svtkStatisticalOutlierRemoval
 * svtkThresholdPoints svtkImplicitFunction svtkExtractGeometry
 * svtkFitImplicitFunction
 */

#ifndef svtkExtractHierarchicalBins_h
#define svtkExtractHierarchicalBins_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkPointCloudFilter.h"

class svtkHierarchicalBinningFilter;
class svtkPointSet;

class SVTKFILTERSPOINTS_EXPORT svtkExtractHierarchicalBins : public svtkPointCloudFilter
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static svtkExtractHierarchicalBins* New();
  svtkTypeMacro(svtkExtractHierarchicalBins, svtkPointCloudFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the level to extract. If non-negative, with a negative bin
   * number, then all points at this level are extracted and sent to the
   * output. If negative, then the points from the specified bin are sent to
   * the output. If both the level and bin number are negative values, then
   * the input is sent to the output. By default the 0th level is
   * extracted. Note that requesting a level greater than the associated
   * svtkHierarchicalBinningFilter will clamp the level to the maximum
   * possible level of the binning filter.
   */
  svtkSetMacro(Level, int);
  svtkGetMacro(Level, int);
  //@}

  //@{
  /**
   * Specify the bin number to extract. If a non-negative value, then the
   * points from the bin number specified are extracted. If negative, then
   * entire levels of points are extacted (assuming the Level is
   * non-negative). Note that the bin tree is flattened, a particular bin
   * number may refer to a bin on any level. Note that requesting a bin
   * greater than the associated svtkHierarchicalBinningFilter will clamp the
   * bin to the maximum possible bin of the binning filter.
   */
  svtkSetMacro(Bin, int);
  svtkGetMacro(Bin, int);
  //@}

  //@{
  /**
   * Specify the svtkHierarchicalBinningFilter to query for relevant
   * information. Make sure that this filter has executed prior to the execution of
   * this filter. (This is generally a safe bet if connected in a pipeline.)
   */
  virtual void SetBinningFilter(svtkHierarchicalBinningFilter*);
  svtkGetObjectMacro(BinningFilter, svtkHierarchicalBinningFilter);
  //@}

protected:
  svtkExtractHierarchicalBins();
  ~svtkExtractHierarchicalBins() override;

  // Users can extract points from a particular level or bin.
  int Level;
  int Bin;
  svtkHierarchicalBinningFilter* BinningFilter;

  // for the binning filter
  void ReportReferences(svtkGarbageCollector*) override;

  // All derived classes must implement this method. Note that a side effect of
  // the class is to populate the PointMap. Zero is returned if there is a failure.
  int FilterPoints(svtkPointSet* input) override;

private:
  svtkExtractHierarchicalBins(const svtkExtractHierarchicalBins&) = delete;
  void operator=(const svtkExtractHierarchicalBins&) = delete;
};

#endif
