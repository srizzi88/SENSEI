/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPassThroughLayoutStrategy.cxx

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

#include "svtkPassThroughLayoutStrategy.h"

#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkPassThroughLayoutStrategy);

// ----------------------------------------------------------------------

svtkPassThroughLayoutStrategy::svtkPassThroughLayoutStrategy() = default;

// ----------------------------------------------------------------------

svtkPassThroughLayoutStrategy::~svtkPassThroughLayoutStrategy() = default;

// ----------------------------------------------------------------------
// Set the graph that will be laid out
void svtkPassThroughLayoutStrategy::Initialize() {}

// ----------------------------------------------------------------------

// Simple graph layout method
void svtkPassThroughLayoutStrategy::Layout() {}

void svtkPassThroughLayoutStrategy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
