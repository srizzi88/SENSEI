/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostConnectedComponents.cxx

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

#include "svtkBoostConnectedComponents.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

#include "svtkBoostGraphAdapter.h"
#include <boost/graph/strong_components.hpp>

using namespace boost;

svtkStandardNewMacro(svtkBoostConnectedComponents);

svtkBoostConnectedComponents::svtkBoostConnectedComponents() {}

svtkBoostConnectedComponents::~svtkBoostConnectedComponents() {}

int svtkBoostConnectedComponents::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkGraph* input = svtkGraph::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkGraph* output = svtkGraph::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Send the data to output.
  output->ShallowCopy(input);

  // Compute connected components.
  if (svtkDirectedGraph::SafeDownCast(input))
  {
    svtkDirectedGraph* g = svtkDirectedGraph::SafeDownCast(input);
    svtkIntArray* comps = svtkIntArray::New();
    comps->SetName("component");
    vector_property_map<default_color_type> color;
    vector_property_map<svtkIdType> root;
    vector_property_map<svtkIdType> discoverTime;
    strong_components(g, comps, color_map(color).root_map(root).discover_time_map(discoverTime));
    output->GetVertexData()->AddArray(comps);
    comps->Delete();
  }
  else
  {
    svtkUndirectedGraph* g = svtkUndirectedGraph::SafeDownCast(input);
    svtkIntArray* comps = svtkIntArray::New();
    comps->SetName("component");
    vector_property_map<default_color_type> color;
    connected_components(g, comps, color_map(color));
    output->GetVertexData()->AddArray(comps);
    comps->Delete();
  }

  return 1;
}

void svtkBoostConnectedComponents::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
