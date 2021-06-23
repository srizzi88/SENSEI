/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeLevelsFilter.h

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
 * @class   svtkTreeLevelsFilter
 * @brief   adds level and leaf fields to a svtkTree
 *
 *
 * The filter currently add two arrays to the incoming svtkTree datastructure.
 * 1) "levels" this is the distance from the root of the vertex. Root = 0
 * and you add 1 for each level down from the root
 * 2) "leaf" this array simply indicates whether the vertex is a leaf or not
 *
 * @par Thanks:
 * Thanks to Brian Wylie from Sandia National Laboratories for creating this
 * class.
 */

#ifndef svtkTreeLevelsFilter_h
#define svtkTreeLevelsFilter_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkTreeAlgorithm.h"

class SVTKINFOVISCORE_EXPORT svtkTreeLevelsFilter : public svtkTreeAlgorithm
{
public:
  static svtkTreeLevelsFilter* New();
  svtkTypeMacro(svtkTreeLevelsFilter, svtkTreeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkTreeLevelsFilter();
  ~svtkTreeLevelsFilter() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkTreeLevelsFilter(const svtkTreeLevelsFilter&) = delete;
  void operator=(const svtkTreeLevelsFilter&) = delete;
};

#endif
