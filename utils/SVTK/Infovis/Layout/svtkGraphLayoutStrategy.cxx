/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphLayoutStrategy.cxx

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
#include "svtkGraphLayoutStrategy.h"

#include "svtkGraph.h"

void svtkGraphLayoutStrategy::SetGraph(svtkGraph* graph)
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

svtkGraphLayoutStrategy::svtkGraphLayoutStrategy()
{
  this->Graph = nullptr;
  this->EdgeWeightField = nullptr;
  this->WeightEdges = false;
}

svtkGraphLayoutStrategy::~svtkGraphLayoutStrategy()
{
  // Unregister svtk objects that were passed in
  this->SetGraph(nullptr);
  this->SetEdgeWeightField(nullptr);
}

void svtkGraphLayoutStrategy::SetWeightEdges(bool state)
{
  // This method is a cut and paste of svtkSetMacro
  // except for the call to Initialize at the end :)
  if (this->WeightEdges != state)
  {
    this->WeightEdges = state;
    this->Modified();
    if (this->Graph)
    {
      this->Initialize();
    }
  }
}

void svtkGraphLayoutStrategy::SetEdgeWeightField(const char* weights)
{
  // This method is a cut and paste of svtkSetStringMacro
  // except for the call to Initialize at the end :)
  if (this->EdgeWeightField == nullptr && weights == nullptr)
  {
    return;
  }
  if (this->EdgeWeightField && weights && (!strcmp(this->EdgeWeightField, weights)))
  {
    return;
  }
  delete[] this->EdgeWeightField;
  if (weights)
  {
    size_t n = strlen(weights) + 1;
    char* cp1 = new char[n];
    const char* cp2 = (weights);
    this->EdgeWeightField = cp1;
    do
    {
      *cp1++ = *cp2++;
    } while (--n);
  }
  else
  {
    this->EdgeWeightField = nullptr;
  }

  this->Modified();

  if (this->Graph)
  {
    this->Initialize();
  }
}

void svtkGraphLayoutStrategy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Graph: " << (this->Graph ? "" : "(none)") << endl;
  if (this->Graph)
  {
    this->Graph->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "WeightEdges: " << (this->WeightEdges ? "True" : "False") << endl;
  os << indent << "EdgeWeightField: " << (this->EdgeWeightField ? this->EdgeWeightField : "(none)")
     << endl;
}
