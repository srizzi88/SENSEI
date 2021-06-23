/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExpandMarkedElements.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkExpandMarkedElements
 * @brief expands marked elements to including adjacent elements.
 *
 * svtkExpandMarkedElements is intended to expand selected cells to
 * grow to include adjacent cells. The filter works across all blocks in a
 * composite dataset and across all ranks. Besides cells, the filter can be used
 * to expand selected points instead in which case adjacent points are defined
 * as points on any cell that has the source point as one of its points.
 *
 * The selected cells (or points) are indicated by a `svtkSignedCharArray` on
 * cell-data (or point-data). The array can be selected by using
 * `SetInputArrayToProcess(0, 0, 0,...)` (see
 * svtkAlgorithm::SetInputArrayToProcess).
 *
 * Currently, the filter only supports expanding marked elements for cells and
 * points.
 */

#ifndef svtkExpandMarkedElements_h
#define svtkExpandMarkedElements_h

#include "svtkFiltersExtractionModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"

class svtkMultiProcessController;

class SVTKFILTERSEXTRACTION_EXPORT svtkExpandMarkedElements : public svtkPassInputTypeAlgorithm
{
public:
  static svtkExpandMarkedElements* New();
  svtkTypeMacro(svtkExpandMarkedElements, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the controller to use. By default, is initialized to
   * `svtkMultiProcessController::GetGlobalController` in the constructor.
   */
  void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

  //@{
  /**
   * Get/Set the number of layers to expand by.
   */
  svtkSetClampMacro(NumberOfLayers, int, 1, SVTK_INT_MAX);
  svtkGetMacro(NumberOfLayers, int);
  //@}
protected:
  svtkExpandMarkedElements();
  ~svtkExpandMarkedElements() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkExpandMarkedElements(const svtkExpandMarkedElements&) = delete;
  void operator=(const svtkExpandMarkedElements&) = delete;

  svtkMultiProcessController* Controller = nullptr;
  int NumberOfLayers = 2;
};

#endif
