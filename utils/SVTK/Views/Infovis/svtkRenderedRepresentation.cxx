/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderedRepresentation.cxx

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

#include "svtkRenderedRepresentation.h"

#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkProp.h"
#include "svtkRenderView.h"
#include "svtkRenderer.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"

#include <vector>

svtkStandardNewMacro(svtkRenderedRepresentation);

class svtkRenderedRepresentation::Internals
{
public:
  // Convenience vectors for storing props to add/remove until the next render,
  // where they are added/removed by PrepareForRendering().
  std::vector<svtkSmartPointer<svtkProp> > PropsToAdd;
  std::vector<svtkSmartPointer<svtkProp> > PropsToRemove;
};

svtkRenderedRepresentation::svtkRenderedRepresentation()
{
  this->Implementation = new Internals();
  this->LabelRenderMode = svtkRenderView::FREETYPE;
}

svtkRenderedRepresentation::~svtkRenderedRepresentation()
{
  delete this->Implementation;
}

void svtkRenderedRepresentation::AddPropOnNextRender(svtkProp* p)
{
  this->Implementation->PropsToAdd.push_back(p);
}

void svtkRenderedRepresentation::RemovePropOnNextRender(svtkProp* p)
{
  this->Implementation->PropsToRemove.push_back(p);
}

void svtkRenderedRepresentation::PrepareForRendering(svtkRenderView* view)
{
  // Add props scheduled to be added on next render.
  for (size_t i = 0; i < this->Implementation->PropsToAdd.size(); ++i)
  {
    view->GetRenderer()->AddViewProp(this->Implementation->PropsToAdd[i]);
  }
  this->Implementation->PropsToAdd.clear();

  // Remove props scheduled to be removed on next render.
  for (size_t i = 0; i < this->Implementation->PropsToRemove.size(); ++i)
  {
    view->GetRenderer()->RemoveViewProp(this->Implementation->PropsToRemove[i]);
  }
  this->Implementation->PropsToRemove.clear();
}

svtkUnicodeString svtkRenderedRepresentation::GetHoverText(
  svtkView* view, svtkProp* prop, svtkIdType cell)
{
  svtkSmartPointer<svtkSelection> cellSelect = svtkSmartPointer<svtkSelection>::New();
  svtkSmartPointer<svtkSelectionNode> cellNode = svtkSmartPointer<svtkSelectionNode>::New();
  cellNode->GetProperties()->Set(svtkSelectionNode::PROP(), prop);
  cellNode->SetFieldType(svtkSelectionNode::CELL);
  cellNode->SetContentType(svtkSelectionNode::INDICES);
  svtkSmartPointer<svtkIdTypeArray> idArr = svtkSmartPointer<svtkIdTypeArray>::New();
  idArr->InsertNextValue(cell);
  cellNode->SetSelectionList(idArr);
  cellSelect->AddNode(cellNode);
  svtkSelection* converted = this->ConvertSelection(view, cellSelect);
  svtkUnicodeString text = this->GetHoverTextInternal(converted);
  if (converted != cellSelect)
  {
    converted->Delete();
  }
  return text;
}

void svtkRenderedRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LabelRenderMode: " << this->LabelRenderMode << endl;
}
