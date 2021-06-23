/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkScalarTree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkScalarTree.h"

#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkGarbageCollector.h"
#include "svtkObjectFactory.h"

svtkCxxSetObjectMacro(svtkScalarTree, DataSet, svtkDataSet);
svtkCxxSetObjectMacro(svtkScalarTree, Scalars, svtkDataArray);

//-----------------------------------------------------------------------------
// Instantiate scalar tree.
svtkScalarTree::svtkScalarTree()
{
  this->DataSet = nullptr;
  this->Scalars = nullptr;
  this->ScalarValue = 0.0;
}

//-----------------------------------------------------------------------------
svtkScalarTree::~svtkScalarTree()
{
  this->SetDataSet(nullptr);
  this->SetScalars(nullptr);
}

//-----------------------------------------------------------------------------
// Shallow copy enough information for a clone to produce the same result on
// the same data.
void svtkScalarTree::ShallowCopy(svtkScalarTree* stree)
{
  this->SetDataSet(stree->GetDataSet());
  this->SetScalars(stree->GetScalars());
}

//-----------------------------------------------------------------------------
void svtkScalarTree::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->DataSet)
  {
    os << indent << "DataSet: " << this->DataSet << "\n";
  }
  else
  {
    os << indent << "DataSet: (none)\n";
  }

  if (this->Scalars)
  {
    os << indent << "Scalars: " << this->Scalars << "\n";
  }
  else
  {
    os << indent << "Scalars: (none)\n";
  }

  os << indent << "Build Time: " << this->BuildTime.GetMTime() << "\n";
}
