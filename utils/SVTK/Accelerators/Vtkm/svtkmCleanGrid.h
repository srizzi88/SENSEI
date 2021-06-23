//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
/**
 * @class   svtkmCleanGrid
 * @brief   removes redundant or unused cells and/or points
 *
 * svtkmCleanGrid is a filter that takes svtkDataSet data as input and
 * generates svtkUnstructuredGrid as output. svtkmCleanGrid will convert all cells
 * to an explicit representation, and if enabled, will remove unused points.
 *
 */

#ifndef svtkmCleanGrid_h
#define svtkmCleanGrid_h

#include "svtkAcceleratorsSVTKmModule.h" //required for correct implementation
#include "svtkUnstructuredGridAlgorithm.h"

class svtkDataSet;
class svtkUnstructuredGrid;

class SVTKACCELERATORSSVTKM_EXPORT svtkmCleanGrid : public svtkUnstructuredGridAlgorithm
{
public:
  svtkTypeMacro(svtkmCleanGrid, svtkUnstructuredGridAlgorithm);

  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkmCleanGrid* New();

  //@{
  /**
   * Get/Set if the points from the input that are unused in the output should
   * be removed. This will take extra time but the result dataset may use
   * less memory. Off by default.
   */
  svtkSetMacro(CompactPoints, bool);
  svtkGetMacro(CompactPoints, bool);
  svtkBooleanMacro(CompactPoints, bool);
  //@}

protected:
  svtkmCleanGrid();
  ~svtkmCleanGrid() override;

  int FillInputPortInformation(int, svtkInformation*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  bool CompactPoints;

private:
  svtkmCleanGrid(const svtkmCleanGrid&) = delete;
  void operator=(const svtkmCleanGrid&) = delete;
};

#endif // svtkmCleanGrid_h
// SVTK-HeaderTest-Exclude: svtkmCleanGrid.h
