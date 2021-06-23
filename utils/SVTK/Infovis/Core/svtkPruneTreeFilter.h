/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPruneTreeFilter.h

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
 * @class   svtkPruneTreeFilter
 * @brief   prune a subtree out of a svtkTree
 *
 *
 * Removes a subtree rooted at a particular vertex in a svtkTree.
 *
 */

#ifndef svtkPruneTreeFilter_h
#define svtkPruneTreeFilter_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkTreeAlgorithm.h"

class svtkTree;
class svtkPVXMLElement;

class SVTKINFOVISCORE_EXPORT svtkPruneTreeFilter : public svtkTreeAlgorithm
{
public:
  static svtkPruneTreeFilter* New();
  svtkTypeMacro(svtkPruneTreeFilter, svtkTreeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the parent vertex of the subtree to remove.
   */
  svtkGetMacro(ParentVertex, svtkIdType);
  svtkSetMacro(ParentVertex, svtkIdType);
  //@}

  //@{
  /**
   * Should we remove the parent vertex, or just its descendants?
   * Default behavior is to remove the parent vertex.
   */
  svtkGetMacro(ShouldPruneParentVertex, bool);
  svtkSetMacro(ShouldPruneParentVertex, bool);
  //@}

protected:
  svtkPruneTreeFilter();
  ~svtkPruneTreeFilter() override;

  svtkIdType ParentVertex;
  bool ShouldPruneParentVertex;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkPruneTreeFilter(const svtkPruneTreeFilter&) = delete;
  void operator=(const svtkPruneTreeFilter&) = delete;
};

#endif
