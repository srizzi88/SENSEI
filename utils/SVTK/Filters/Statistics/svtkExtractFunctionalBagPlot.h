/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractFunctionalBagPlot.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractFunctionalBagPlot
 *
 *
 * From an input table containing series on port 0 and another table
 * describing densities on port 1 (for instance obtained by applying
 * filter  svtkHighestDensityRegionsStatistics, this filter generates
 * a table containing all the columns of the input port 0 plus two 2
 * components columns containing the bag series to be used by
 * svtkFunctionalBagPlot.
 *
 * @sa
 * svtkFunctionalBagPlot svtkHighestDensityRegionsStatistics
 */

#ifndef svtkExtractFunctionalBagPlot_h
#define svtkExtractFunctionalBagPlot_h

#include "svtkFiltersStatisticsModule.h" // For export macro
#include "svtkTableAlgorithm.h"

class SVTKFILTERSSTATISTICS_EXPORT svtkExtractFunctionalBagPlot : public svtkTableAlgorithm
{
public:
  static svtkExtractFunctionalBagPlot* New();
  svtkTypeMacro(svtkExtractFunctionalBagPlot, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // Density value for the median quartile.
  svtkSetMacro(DensityForP50, double);

  //@{
  /**
   * Density value for the user defined quartile.
   */
  svtkSetMacro(DensityForPUser, double);
  svtkSetMacro(PUser, int);
  //@}

protected:
  svtkExtractFunctionalBagPlot();
  ~svtkExtractFunctionalBagPlot() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char* P50String;
  char* PUserString;
  double DensityForP50;
  double DensityForPUser;
  int PUser;

private:
  svtkExtractFunctionalBagPlot(const svtkExtractFunctionalBagPlot&) = delete;
  void operator=(const svtkExtractFunctionalBagPlot&) = delete;
};

#endif // svtkExtractFunctionalBagPlot_h
