/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMRUtilities.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkParallelAMRUtilities
 *
 *
 *  A concrete instance of svtkObject that employs a singleton design
 *  pattern and implements functionality for AMR specific operations.
 *
 * @sa
 *  svtkOverlappingAMR, svtkAMRBox
 */

#ifndef svtkParallelAMRUtilities_h
#define svtkParallelAMRUtilities_h

#include "svtkAMRUtilities.h"
#include "svtkFiltersAMRModule.h" // For export macro
#include <vector>                // For C++ vector

// Forward declarations
class svtkMultiProcessController;
class svtkOverlappingAMR;

class SVTKFILTERSAMR_EXPORT svtkParallelAMRUtilities : public svtkAMRUtilities
{
public:
  // Standard Routines
  svtkTypeMacro(svtkParallelAMRUtilities, svtkAMRUtilities);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * This method detects and strips partially overlapping cells from a
   * given AMR dataset. If ghost layers are detected, they are removed and
   * new grid instances are created to represent the stripped
   * data-set otherwise, each block is shallow-copied.

   * .SECTION Assumptions
   * 1) The ghosted AMR data must have complete metadata information.
   */
  static void StripGhostLayers(svtkOverlappingAMR* ghostedAMRData,
    svtkOverlappingAMR* strippedAMRData, svtkMultiProcessController* myController);

  /**
   * Compute map from block indices to process ids
   */
  static void DistributeProcessInformation(
    svtkOverlappingAMR* amr, svtkMultiProcessController* myController, std::vector<int>& ProcessMap);

  /**
   * Blank cells in overlapping AMR
   */
  static void BlankCells(svtkOverlappingAMR* amr, svtkMultiProcessController* myController);

private:
  svtkParallelAMRUtilities(const svtkParallelAMRUtilities&) = delete;
  void operator=(const svtkParallelAMRUtilities&) = delete;
};

#endif /* svtkParallelAMRUtilities_h */
