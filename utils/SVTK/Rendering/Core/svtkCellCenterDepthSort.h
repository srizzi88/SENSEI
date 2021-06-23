/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCellCenterDepthSort.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

/**
 * @class   svtkCellCenterDepthSort
 * @brief   A simple implementation of svtkCellDepthSort.
 *
 *
 * svtkCellCenterDepthSort is a simple and fast implementation of depth
 * sort, but it only provides approximate results.  The sorting algorithm
 * finds the centroids of all the cells.  It then performs the dot product
 * of the centroids against a vector pointing in the direction of the
 * camera transformed into object space.  It then performs an ordinary sort
 * on the result.
 *
 */

#ifndef svtkCellCenterDepthSort_h
#define svtkCellCenterDepthSort_h

#include "svtkRenderingCoreModule.h" // For export macro
#include "svtkVisibilitySort.h"

class svtkFloatArray;

class svtkCellCenterDepthSortStack;

class SVTKRENDERINGCORE_EXPORT svtkCellCenterDepthSort : public svtkVisibilitySort
{
public:
  svtkTypeMacro(svtkCellCenterDepthSort, svtkVisibilitySort);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkCellCenterDepthSort* New();

  void InitTraversal() override;
  svtkIdTypeArray* GetNextCells() override;

protected:
  svtkCellCenterDepthSort();
  ~svtkCellCenterDepthSort() override;

  svtkIdTypeArray* SortedCells;
  svtkIdTypeArray* SortedCellPartition;

  svtkFloatArray* CellCenters;
  svtkFloatArray* CellDepths;
  svtkFloatArray* CellPartitionDepths;

  virtual float* ComputeProjectionVector();
  virtual void ComputeCellCenters();
  virtual void ComputeDepths();

private:
  svtkCellCenterDepthSortStack* ToSort;

  svtkCellCenterDepthSort(const svtkCellCenterDepthSort&) = delete;
  void operator=(const svtkCellCenterDepthSort&) = delete;
};

#endif
