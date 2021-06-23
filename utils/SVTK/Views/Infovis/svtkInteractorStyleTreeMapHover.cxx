/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleTreeMapHover.cxx

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

#include "svtkInteractorStyleTreeMapHover.h"

#include "svtkActor.h"
#include "svtkBalloonRepresentation.h"
#include "svtkCallbackCommand.h"
#include "svtkCellArray.h"
#include "svtkIdTypeArray.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRendererCollection.h"
#include "svtkStdString.h"
#include "svtkStringArray.h"
#include "svtkTreeMapLayout.h"
#include "svtkTreeMapToPolyData.h"
#include "svtkVariant.h"
#include "svtkWorldPointPicker.h"

svtkStandardNewMacro(svtkInteractorStyleTreeMapHover);

//----------------------------------------------------------------------------

svtkInteractorStyleTreeMapHover::svtkInteractorStyleTreeMapHover()
{
  this->Picker = svtkWorldPointPicker::New();
  this->Balloon = svtkBalloonRepresentation::New();
  this->Balloon->SetBalloonText("");
  this->Balloon->SetOffset(1, 1);
  // this->Balloon->SetNeedToRender(true);
  this->Layout = nullptr;
  this->LabelField = nullptr;
  this->CurrentSelectedId = -1;
  this->TreeMapToPolyData = nullptr;
  this->Layout = nullptr;

  // Setup up pipelines for highlighting and selecting vertices
  this->SelectionPoints = svtkPoints::New();
  this->SelectionPoints->SetNumberOfPoints(5);
  this->HighlightPoints = svtkPoints::New();
  this->HighlightPoints->SetNumberOfPoints(5);
  svtkCellArray* selA = svtkCellArray::New();
  selA->InsertNextCell(5);
  svtkCellArray* highA = svtkCellArray::New();
  highA->InsertNextCell(5);
  int i;
  for (i = 0; i < 5; ++i)
  {
    selA->InsertCellPoint(i);
    highA->InsertCellPoint(i);
  }
  svtkPolyData* selData = svtkPolyData::New();
  selData->SetPoints(this->SelectionPoints);
  selData->SetLines(selA);
  svtkPolyDataMapper* selMap = svtkPolyDataMapper::New();
  selMap->SetInputData(selData);
  this->SelectionActor = svtkActor::New();
  this->SelectionActor->SetMapper(selMap);
  this->SelectionActor->VisibilityOff();
  this->SelectionActor->PickableOff();
  this->SelectionActor->GetProperty()->SetLineWidth(2.0);
  svtkPolyData* highData = svtkPolyData::New();
  highData->SetPoints(this->HighlightPoints);
  highData->SetLines(highA);
  svtkPolyDataMapper* highMap = svtkPolyDataMapper::New();
  highMap->SetInputData(highData);
  this->HighlightActor = svtkActor::New();
  this->HighlightActor->SetMapper(highMap);
  this->HighlightActor->VisibilityOff();
  this->HighlightActor->PickableOff();
  this->HighlightActor->GetProperty()->SetColor(1, 1, 1);
  this->HighlightActor->GetProperty()->SetLineWidth(1.0);
  selA->Delete();
  selData->Delete();
  selMap->Delete();
  highA->Delete();
  highData->Delete();
  highMap->Delete();
}

//----------------------------------------------------------------------------

svtkInteractorStyleTreeMapHover::~svtkInteractorStyleTreeMapHover()
{
  this->SelectionPoints->Delete();
  this->HighlightPoints->Delete();
  this->SelectionActor->Delete();
  this->HighlightActor->Delete();
  this->Picker->Delete();
  this->Balloon->Delete();
  if (this->Layout != nullptr)
  {
    this->Layout->Delete();
    this->Layout = nullptr;
  }
  if (this->TreeMapToPolyData != nullptr)
  {
    this->TreeMapToPolyData->Delete();
    this->TreeMapToPolyData = nullptr;
  }
  this->SetLabelField(nullptr);
}

svtkCxxSetObjectMacro(svtkInteractorStyleTreeMapHover, Layout, svtkTreeMapLayout);

svtkCxxSetObjectMacro(svtkInteractorStyleTreeMapHover, TreeMapToPolyData, svtkTreeMapToPolyData);

void svtkInteractorStyleTreeMapHover::SetInteractor(svtkRenderWindowInteractor* rwi)
{
  // See if we already had one
  svtkRenderWindowInteractor* mrwi = this->GetInteractor();
  svtkRenderer* ren;
  if (mrwi && mrwi->GetRenderWindow())
  {
    this->FindPokedRenderer(0, 0);
    ren = this->CurrentRenderer;
    if (ren)
    {
      ren->RemoveActor(SelectionActor);
      ren->RemoveActor(HighlightActor);
    }
  }
  svtkInteractorStyleImage::SetInteractor(rwi);
  if (rwi && rwi->GetRenderWindow())
  {
    this->FindPokedRenderer(0, 0);
    ren = this->CurrentRenderer;
    if (ren)
    {
      ren->AddActor(SelectionActor);
      ren->AddActor(HighlightActor);
    }
  }
}

//----------------------------------------------------------------------------

void svtkInteractorStyleTreeMapHover::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Layout: " << (this->Layout ? "" : "(none)") << endl;
  if (this->Layout)
  {
    this->Layout->PrintSelf(os, indent.GetNextIndent());
  }

  os << indent << "TreeMapToPolyData: " << (this->TreeMapToPolyData ? "" : "(none)") << endl;
  if (this->TreeMapToPolyData)
  {
    this->TreeMapToPolyData->PrintSelf(os, indent.GetNextIndent());
  }

  os << indent << "LabelField: " << (this->LabelField ? this->LabelField : "(none)") << endl;
}

svtkIdType svtkInteractorStyleTreeMapHover::GetTreeMapIdAtPos(int x, int y)
{
  svtkIdType id = -1;

  svtkRenderer* r = this->CurrentRenderer;
  if (r == nullptr)
  {
    return id;
  }

  // Use the hardware picker to find a point in world coordinates.
  this->Picker->Pick(x, y, 0, r);
  double pos[3];
  this->Picker->GetPickPosition(pos);

  if (this->Layout != nullptr)
  {
    float posFloat[3];
    for (int i = 0; i < 3; i++)
    {
      posFloat[i] = pos[i];
    }
    id = Layout->FindVertex(posFloat);
  }

  return id;
}

void svtkInteractorStyleTreeMapHover::GetBoundingBoxForTreeMapItem(svtkIdType id, float* binfo)
{
  if (this->Layout)
  {
    this->Layout->GetBoundingBox(id, binfo);
  }
}

void svtkInteractorStyleTreeMapHover::OnMouseMove()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  this->FindPokedRenderer(x, y);
  svtkRenderer* r = this->CurrentRenderer;
  if (r == nullptr)
  {
    return;
  }

  if (!r->HasViewProp(this->Balloon))
  {
    r->AddActor(this->Balloon);
    this->Balloon->SetRenderer(r);
  }

  // Use the hardware picker to find a point in world coordinates.
  float binfo[4];
  svtkIdType id = this->GetTreeMapIdAtPos(x, y);

  if (id != -1)
  {
    this->GetBoundingBoxForTreeMapItem(id, binfo);
  }

  double loc[2] = { static_cast<double>(x), static_cast<double>(y) };
  this->Balloon->EndWidgetInteraction(loc);

  if ((this->Layout != nullptr) && (this->Layout->GetOutput() != nullptr))
  {

    svtkAbstractArray* absArray =
      this->Layout->GetOutput()->GetVertexData()->GetAbstractArray(this->LabelField);
    if (absArray != nullptr && id > -1)
    {
      svtkStdString str;
      if (svtkArrayDownCast<svtkStringArray>(absArray))
      {
        str = svtkArrayDownCast<svtkStringArray>(absArray)->GetValue(id);
      }
      if (svtkArrayDownCast<svtkDataArray>(absArray))
      {
        str = svtkVariant(svtkArrayDownCast<svtkDataArray>(absArray)->GetTuple(id)[0]).ToString();
      }
      this->Balloon->SetBalloonText(str);
      svtkTree* tree = this->Layout->GetOutput();
      double z;
      if (this->TreeMapToPolyData != nullptr)
      {
        z = this->TreeMapToPolyData->GetLevelDeltaZ() * (tree->GetLevel(id) + 1);
      }
      else
      {
        z = 0.02;
      }
      this->HighlightPoints->SetPoint(0, binfo[0], binfo[2], z);
      this->HighlightPoints->SetPoint(1, binfo[1], binfo[2], z);
      this->HighlightPoints->SetPoint(2, binfo[1], binfo[3], z);
      this->HighlightPoints->SetPoint(3, binfo[0], binfo[3], z);
      this->HighlightPoints->SetPoint(4, binfo[0], binfo[2], z);
      this->HighlightPoints->Modified();
      this->HighlightActor->VisibilityOn();
    }
    else
    {
      this->Balloon->SetBalloonText("");
      HighlightActor->VisibilityOff();
    }

    this->Balloon->StartWidgetInteraction(loc);

    this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
    this->Superclass::OnMouseMove();
    this->GetInteractor()->Render();
  }
}

void svtkInteractorStyleTreeMapHover::SetHighLightColor(double r, double g, double b)
{
  this->HighlightActor->GetProperty()->SetColor(r, g, b);
}

void svtkInteractorStyleTreeMapHover::SetSelectionLightColor(double r, double g, double b)
{
  this->SelectionActor->GetProperty()->SetColor(r, g, b);
}

void svtkInteractorStyleTreeMapHover::SetHighLightWidth(double lw)
{
  this->HighlightActor->GetProperty()->SetLineWidth(lw);
}

double svtkInteractorStyleTreeMapHover::GetHighLightWidth()
{
  return this->HighlightActor->GetProperty()->GetLineWidth();
}

void svtkInteractorStyleTreeMapHover::SetSelectionWidth(double lw)
{
  this->SelectionActor->GetProperty()->SetLineWidth(lw);
}

double svtkInteractorStyleTreeMapHover::GetSelectionWidth()
{
  return this->SelectionActor->GetProperty()->GetLineWidth();
}

//---------------------------------------------------------------------------
void svtkInteractorStyleTreeMapHover::OnLeftButtonUp()
{
  // Get the id of the object underneath the mouse
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  this->FindPokedRenderer(x, y);

#if 0
  svtkRenderer* r = this->CurrentRenderer;
  if (r == nullptr)
  {
    return;
  }

  if (!r->HasViewProp(this->Balloon))
  {
    r->AddActor(this->Balloon);
    this->Balloon->SetRenderer(r);
  }

  double loc[2] = {x, y};
  this->Balloon->EndWidgetInteraction(loc);
#endif

  this->CurrentSelectedId = GetTreeMapIdAtPos(x, y);

  // Get the pedigree id of this object and
  // send out an event with that id as data
  svtkIdType id = this->CurrentSelectedId;
  svtkAbstractArray* absArray =
    this->Layout->GetOutput()->GetVertexData()->GetAbstractArray("PedigreeVertexId");
  if (absArray)
  {
    svtkIdTypeArray* idArray = svtkArrayDownCast<svtkIdTypeArray>(absArray);
    if (idArray)
    {
      id = idArray->GetValue(this->CurrentSelectedId);
    }
  }
  this->InvokeEvent(svtkCommand::UserEvent, &id);

  this->HighLightCurrentSelectedItem();
  Superclass::OnLeftButtonUp();
}

void svtkInteractorStyleTreeMapHover::HighLightItem(svtkIdType id)
{
  this->CurrentSelectedId = id;
  this->HighLightCurrentSelectedItem();
}

void svtkInteractorStyleTreeMapHover::HighLightCurrentSelectedItem()
{
  float binfo[4];

  if (this->CurrentSelectedId > -1)
  {
    this->GetBoundingBoxForTreeMapItem(this->CurrentSelectedId, binfo);
    svtkTree* tree = this->Layout->GetOutput();
    double z;
    if (this->TreeMapToPolyData != nullptr)
    {
      z = this->TreeMapToPolyData->GetLevelDeltaZ() * (tree->GetLevel(this->CurrentSelectedId) + 1);
    }
    else
    {
      z = 0.01;
    }
    this->SelectionPoints->SetPoint(0, binfo[0], binfo[2], z);
    this->SelectionPoints->SetPoint(1, binfo[1], binfo[2], z);
    this->SelectionPoints->SetPoint(2, binfo[1], binfo[3], z);
    this->SelectionPoints->SetPoint(3, binfo[0], binfo[3], z);
    this->SelectionPoints->SetPoint(4, binfo[0], binfo[2], z);
    this->SelectionPoints->Modified();
    this->SelectionActor->VisibilityOn();
  }
  else
  {
    SelectionActor->VisibilityOff();
  }
  if (this->GetInteractor())
  {
    this->GetInteractor()->Render();
  }
}
