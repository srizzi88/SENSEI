/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPCellDataToPointData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPCellDataToPointData
 * @brief   Compute point arrays from cell arrays.
 *
 * Like it super class, this filter averages the cell data around
 * a point to get new point data.  This subclass requests a layer of
 * ghost cells to make the results invariant to pieces.  There is a
 * "PieceInvariant" flag that lets the user change the behavior
 * of the filter to that of its superclass.
 */

#ifndef svtkPCellDataToPointData_h
#define svtkPCellDataToPointData_h

#include "svtkCellDataToPointData.h"
#include "svtkFiltersParallelModule.h" // For export macro

class SVTKFILTERSPARALLEL_EXPORT svtkPCellDataToPointData : public svtkCellDataToPointData
{
public:
  svtkTypeMacro(svtkPCellDataToPointData, svtkCellDataToPointData);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPCellDataToPointData* New();

  //@{
  /**
   * To get piece invariance, this filter has to request an
   * extra ghost level.  By default piece invariance is on.
   */
  svtkSetMacro(PieceInvariant, svtkTypeBool);
  svtkGetMacro(PieceInvariant, svtkTypeBool);
  svtkBooleanMacro(PieceInvariant, svtkTypeBool);
  //@}

protected:
  svtkPCellDataToPointData();
  ~svtkPCellDataToPointData() override {}

  // Usual data generation method
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkTypeBool PieceInvariant;

private:
  svtkPCellDataToPointData(const svtkPCellDataToPointData&) = delete;
  void operator=(const svtkPCellDataToPointData&) = delete;
};

#endif
