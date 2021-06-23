/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkCirclePackLayoutStrategy.cxx

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

#include "svtkCirclePackLayoutStrategy.h"

#include "svtkTree.h"

svtkCirclePackLayoutStrategy::svtkCirclePackLayoutStrategy() = default;

svtkCirclePackLayoutStrategy::~svtkCirclePackLayoutStrategy() = default;

void svtkCirclePackLayoutStrategy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
