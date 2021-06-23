/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAppendLocationAttributes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAppendLocationAttributes
 * @brief   add point locations to point data and/or cell centers cell data, respectively
 *
 * svtkAppendLocationAttributes is a filter that takes as input any dataset and
 * optionally adds points as point data and optionally adds cell center locations as
 * cell data in the output. The center of a cell is its parametric center, not necessarily
 * the geometric or bounding box center. Point and cell attributes in the input can optionally
 * be copied to the output.
 *
 * @note
 * Empty cells will have their center set to (0, 0, 0).
 *
 * @sa
 * svtkCellCenters
 */

#ifndef svtkAppendLocationAttributes_h
#define svtkAppendLocationAttributes_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkAppendLocationAttributes : public svtkPassInputTypeAlgorithm
{
public:
  static svtkAppendLocationAttributes* New();
  svtkTypeMacro(svtkAppendLocationAttributes, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Enable/disable whether input point locations should be saved as a point data array.
   * Default is `true` i.e. the points will be propagated as a point data array named
   * "PointLocations".
   */
  svtkSetMacro(AppendPointLocations, bool);
  svtkGetMacro(AppendPointLocations, bool);
  svtkBooleanMacro(AppendPointLocations, bool);
  //@}

  //@{
  /**
   * Enable/disable whether input cell center locations should be saved as a cell data array.
   * Default is `true` i.e. the cell centers will be propagated as a cell data array named
   * "CellCenters".
   */
  svtkSetMacro(AppendCellCenters, bool);
  svtkGetMacro(AppendCellCenters, bool);
  svtkBooleanMacro(AppendCellCenters, bool);
  //@}

protected:
  svtkAppendLocationAttributes() = default;
  ~svtkAppendLocationAttributes() override = default;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  bool AppendPointLocations = true;
  bool AppendCellCenters = true;

private:
  svtkAppendLocationAttributes(const svtkAppendLocationAttributes&) = delete;
  void operator=(const svtkAppendLocationAttributes&) = delete;
};

#endif
