/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEdgeLayoutStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "svtkEdgeLayoutStrategy.h"

#include "svtkGraph.h"

void svtkEdgeLayoutStrategy::SetGraph(svtkGraph* graph)
{
  // This method is a cut and paste of svtkCxxSetObjectMacro
  // except for the call to Initialize in the middle :)
  if (graph != this->Graph)
  {
    svtkGraph* tmp = this->Graph;
    this->Graph = graph;
    if (this->Graph != nullptr)
    {
      this->Graph->Register(this);
      this->Initialize();
    }
    if (tmp != nullptr)
    {
      tmp->UnRegister(this);
    }
    this->Modified();
  }
}

svtkEdgeLayoutStrategy::svtkEdgeLayoutStrategy()
{
  this->Graph = nullptr;
  this->EdgeWeightArrayName = nullptr;
}

svtkEdgeLayoutStrategy::~svtkEdgeLayoutStrategy()
{
  // Unregister svtk objects that were passed in
  this->SetGraph(nullptr);
  this->SetEdgeWeightArrayName(nullptr);
}

void svtkEdgeLayoutStrategy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Graph: " << (this->Graph ? "" : "(none)") << endl;
  if (this->Graph)
  {
    this->Graph->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "EdgeWeightArrayName: "
     << (this->EdgeWeightArrayName ? this->EdgeWeightArrayName : "(none)") << endl;
}
