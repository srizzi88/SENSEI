/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableToTreeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkTableToTreeFilter
 * @brief   Filter that converts a svtkTable to a svtkTree
 *
 *
 *
 * svtkTableToTreeFilter is a filter for converting a svtkTable data structure
 * into a svtkTree datastructure.  Currently, this will convert the table into
 * a star, with each row of the table as a child of a new root node.
 * The columns of the table are passed as node fields of the tree.
 */

#ifndef svtkTableToTreeFilter_h
#define svtkTableToTreeFilter_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkTreeAlgorithm.h"

class SVTKINFOVISCORE_EXPORT svtkTableToTreeFilter : public svtkTreeAlgorithm
{
public:
  static svtkTableToTreeFilter* New();
  svtkTypeMacro(svtkTableToTreeFilter, svtkTreeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkTableToTreeFilter();
  ~svtkTableToTreeFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info) override;
  int FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info) override;

private:
  svtkTableToTreeFilter(const svtkTableToTreeFilter&) = delete;
  void operator=(const svtkTableToTreeFilter&) = delete;
};

#endif
