/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderedSurfaceRepresentation.cxx

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

#include "svtkRenderedSurfaceRepresentation.h"

#include "svtkActor.h"
#include "svtkAlgorithmOutput.h"
#include "svtkApplyColors.h"
#include "svtkCommand.h"
#include "svtkConvertSelection.h"
#include "svtkDataObject.h"
#include "svtkExtractSelection.h"
#include "svtkGeometryFilter.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkLookupTable.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderView.h"
#include "svtkRenderer.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkTransformFilter.h"
#include "svtkViewTheme.h"

svtkStandardNewMacro(svtkRenderedSurfaceRepresentation);
//----------------------------------------------------------------------------
svtkRenderedSurfaceRepresentation::svtkRenderedSurfaceRepresentation()
{
  this->TransformFilter = svtkTransformFilter::New();
  this->ApplyColors = svtkApplyColors::New();
  this->GeometryFilter = svtkGeometryFilter::New();
  this->Mapper = svtkPolyDataMapper::New();
  this->Actor = svtkActor::New();

  this->CellColorArrayNameInternal = nullptr;

  // Connect pipeline
  this->ApplyColors->SetInputConnection(this->TransformFilter->GetOutputPort());
  this->GeometryFilter->SetInputConnection(this->ApplyColors->GetOutputPort());
  this->Mapper->SetInputConnection(this->GeometryFilter->GetOutputPort());
  this->Actor->SetMapper(this->Mapper);
  this->Actor->GetProperty()->SetPointSize(10);

  // Set parameters
  this->Mapper->SetScalarModeToUseCellFieldData();
  this->Mapper->SelectColorArray("svtkApplyColors color");
  this->Mapper->SetScalarVisibility(true);

  // Apply default theme
  svtkSmartPointer<svtkViewTheme> theme = svtkSmartPointer<svtkViewTheme>::New();
  theme->SetCellOpacity(1.0);
  this->ApplyViewTheme(theme);
}

//----------------------------------------------------------------------------
svtkRenderedSurfaceRepresentation::~svtkRenderedSurfaceRepresentation()
{
  this->TransformFilter->Delete();
  this->ApplyColors->Delete();
  this->GeometryFilter->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();
  this->SetCellColorArrayNameInternal(nullptr);
}

//----------------------------------------------------------------------------
int svtkRenderedSurfaceRepresentation::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector*)
{
  this->TransformFilter->SetInputConnection(0, this->GetInternalOutputPort());
  this->ApplyColors->SetInputConnection(1, this->GetInternalAnnotationOutputPort());
  return 1;
}

//----------------------------------------------------------------------------
void svtkRenderedSurfaceRepresentation::PrepareForRendering(svtkRenderView* view)
{
  this->Superclass::PrepareForRendering(view);
  this->TransformFilter->SetTransform(view->GetTransform());
}

//----------------------------------------------------------------------------
bool svtkRenderedSurfaceRepresentation::AddToView(svtkView* view)
{
  svtkRenderView* rv = svtkRenderView::SafeDownCast(view);
  if (!rv)
  {
    svtkErrorMacro("Can only add to a subclass of svtkRenderView.");
    return false;
  }
  rv->GetRenderer()->AddActor(this->Actor);
  return true;
}

//----------------------------------------------------------------------------
bool svtkRenderedSurfaceRepresentation::RemoveFromView(svtkView* view)
{
  svtkRenderView* rv = svtkRenderView::SafeDownCast(view);
  if (!rv)
  {
    return false;
  }
  rv->GetRenderer()->RemoveActor(this->Actor);
  return true;
}

//----------------------------------------------------------------------------
svtkSelection* svtkRenderedSurfaceRepresentation::ConvertSelection(
  svtkView* svtkNotUsed(view), svtkSelection* selection)
{
  svtkSmartPointer<svtkSelection> propSelection = svtkSmartPointer<svtkSelection>::New();

  // Extract the selection for the right prop
  if (selection->GetNumberOfNodes() > 1)
  {
    for (unsigned int i = 0; i < selection->GetNumberOfNodes(); i++)
    {
      svtkSelectionNode* node = selection->GetNode(i);
      svtkProp* prop = svtkProp::SafeDownCast(node->GetProperties()->Get(svtkSelectionNode::PROP()));
      if (prop == this->Actor)
      {
        svtkSmartPointer<svtkSelectionNode> nodeCopy = svtkSmartPointer<svtkSelectionNode>::New();
        nodeCopy->ShallowCopy(node);
        nodeCopy->GetProperties()->Remove(svtkSelectionNode::PROP());
        propSelection->AddNode(nodeCopy);
      }
    }
  }
  else
  {
    propSelection->ShallowCopy(selection);
  }

  // Start with an empty selection
  svtkSelection* converted = svtkSelection::New();
  svtkSmartPointer<svtkSelectionNode> node = svtkSmartPointer<svtkSelectionNode>::New();
  node->SetContentType(this->SelectionType);
  node->SetFieldType(svtkSelectionNode::CELL);
  svtkSmartPointer<svtkIdTypeArray> empty = svtkSmartPointer<svtkIdTypeArray>::New();
  node->SetSelectionList(empty);
  converted->AddNode(node);
  // Convert to the correct type of selection
  if (this->GetInput())
  {
    svtkDataObject* obj = this->GetInput();
    if (obj)
    {
      svtkSelection* index = svtkConvertSelection::ToSelectionType(
        propSelection, obj, this->SelectionType, this->SelectionArrayNames);
      converted->ShallowCopy(index);
      index->Delete();
    }
  }

  return converted;
}

//----------------------------------------------------------------------------
void svtkRenderedSurfaceRepresentation::SetCellColorArrayName(const char* arrayName)
{
  this->SetCellColorArrayNameInternal(arrayName);
  this->ApplyColors->SetInputArrayToProcess(
    1, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, arrayName);
}

//----------------------------------------------------------------------------
void svtkRenderedSurfaceRepresentation::ApplyViewTheme(svtkViewTheme* theme)
{
  this->Superclass::ApplyViewTheme(theme);

  this->ApplyColors->SetPointLookupTable(theme->GetPointLookupTable());
  this->ApplyColors->SetCellLookupTable(theme->GetCellLookupTable());

  this->ApplyColors->SetDefaultPointColor(theme->GetPointColor());
  this->ApplyColors->SetDefaultPointOpacity(theme->GetPointOpacity());
  this->ApplyColors->SetDefaultCellColor(theme->GetCellColor());
  this->ApplyColors->SetDefaultCellOpacity(theme->GetCellOpacity());
  this->ApplyColors->SetSelectedPointColor(theme->GetSelectedPointColor());
  // this->ApplyColors->SetSelectedPointOpacity(theme->GetSelectedPointOpacity());
  this->ApplyColors->SetSelectedCellColor(theme->GetSelectedCellColor());
  // this->ApplyColors->SetSelectedCellOpacity(theme->GetSelectedCellOpacity());
  this->ApplyColors->SetScalePointLookupTable(theme->GetScalePointLookupTable());
  this->ApplyColors->SetScaleCellLookupTable(theme->GetScaleCellLookupTable());

  float baseSize = static_cast<float>(theme->GetPointSize());
  float lineWidth = static_cast<float>(theme->GetLineWidth());
  this->Actor->GetProperty()->SetPointSize(baseSize);
  this->Actor->GetProperty()->SetLineWidth(lineWidth);

  // TODO: Enable labeling
  // this->VertexTextProperty->SetColor(theme->GetVertexLabelColor());
  // this->VertexTextProperty->SetLineOffset(-2*baseSize);
  // this->EdgeTextProperty->SetColor(theme->GetEdgeLabelColor());
}

//----------------------------------------------------------------------------
void svtkRenderedSurfaceRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ApplyColors:" << endl;
  this->ApplyColors->PrintSelf(os, indent.GetNextIndent());
  os << indent << "GeometryFilter:" << endl;
  this->GeometryFilter->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Mapper:" << endl;
  this->Mapper->PrintSelf(os, indent.GetNextIndent());
}
