/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleAreaSelectHover.cxx

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

#include "svtkInteractorStyleAreaSelectHover.h"

#include "svtkActor.h"
#include "svtkAppendPolyData.h"
#include "svtkAreaLayout.h"
#include "svtkBalloonRepresentation.h"
#include "svtkCallbackCommand.h"
#include "svtkCellArray.h"
#include "svtkExtractEdges.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRendererCollection.h"
#include "svtkSectorSource.h"
#include "svtkStdString.h"
#include "svtkStringArray.h"
#include "svtkVariant.h"
#include "svtkWorldPointPicker.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

svtkStandardNewMacro(svtkInteractorStyleAreaSelectHover);

svtkCxxSetObjectMacro(svtkInteractorStyleAreaSelectHover, Layout, svtkAreaLayout);

//----------------------------------------------------------------------------
svtkInteractorStyleAreaSelectHover::svtkInteractorStyleAreaSelectHover()
{
  this->Picker = svtkWorldPointPicker::New();
  this->Balloon = svtkBalloonRepresentation::New();
  this->Balloon->SetBalloonText("");
  this->Balloon->SetOffset(1, 1);
  this->Layout = nullptr;
  this->LabelField = nullptr;
  this->UseRectangularCoordinates = false;

  this->HighlightData = svtkPolyData::New();
  svtkPolyDataMapper* highMap = svtkPolyDataMapper::New();
  highMap->SetInputData(this->HighlightData);
  this->HighlightActor = svtkActor::New();
  this->HighlightActor->SetMapper(highMap);
  this->HighlightActor->VisibilityOff();
  this->HighlightActor->PickableOff();
  this->HighlightActor->GetProperty()->SetLineWidth(4.0);
  highMap->Delete();
}

//----------------------------------------------------------------------------
svtkInteractorStyleAreaSelectHover::~svtkInteractorStyleAreaSelectHover()
{
  this->HighlightData->Delete();
  this->HighlightActor->Delete();
  this->Picker->Delete();
  this->Balloon->Delete();
  if (this->Layout)
  {
    this->Layout->Delete();
    this->Layout = nullptr;
  }
  this->SetLabelField(nullptr);
}

//----------------------------------------------------------------------------
void svtkInteractorStyleAreaSelectHover::SetInteractor(svtkRenderWindowInteractor* rwi)
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
      ren->RemoveActor(HighlightActor);
    }
  }
  svtkInteractorStyleRubberBand2D::SetInteractor(rwi);
  if (rwi && rwi->GetRenderWindow())
  {
    this->FindPokedRenderer(0, 0);
    ren = this->CurrentRenderer;
    if (ren)
    {
      ren->AddActor(HighlightActor);
    }
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleAreaSelectHover::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Layout: " << (this->Layout ? "" : "(none)") << endl;
  if (this->Layout)
  {
    this->Layout->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "LabelField: " << (this->LabelField ? this->LabelField : "(none)") << endl;
  os << indent << "UseRectangularCoordinates: " << this->UseRectangularCoordinates << endl;
}

//----------------------------------------------------------------------------
svtkIdType svtkInteractorStyleAreaSelectHover::GetIdAtPos(int x, int y)
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

  if (this->Layout)
  {
    float posFloat[3];
    for (int i = 0; i < 3; i++)
    {
      posFloat[i] = pos[i];
    }
    id = this->Layout->FindVertex(posFloat);
  }

  return id;
}

//----------------------------------------------------------------------------
void svtkInteractorStyleAreaSelectHover::GetBoundingAreaForItem(svtkIdType id, float* sinfo)
{
  if (this->Layout)
  {
    this->Layout->GetBoundingArea(id, sinfo);
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleAreaSelectHover::OnMouseMove()
{
  if (this->Interaction == svtkInteractorStyleRubberBand2D::SELECTING)
  {
    this->Balloon->SetVisibility(false);
    this->Superclass::OnMouseMove();
    return;
  }
  this->Balloon->SetVisibility(true);

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
  float sinfo[4];
  svtkIdType id = this->GetIdAtPos(x, y);

  if (id != -1)
  {
    this->GetBoundingAreaForItem(id, sinfo);
  }

  double loc[2] = { static_cast<double>(x), static_cast<double>(y) };
  this->Balloon->EndWidgetInteraction(loc);

  if (this->Layout && this->Layout->GetOutput())
  {
    svtkAbstractArray* absArray =
      this->Layout->GetOutput()->GetVertexData()->GetAbstractArray(this->LabelField);
    // find the information for the correct sector,
    //  unless there isn't a sector or it is the root node
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
      double z = 0.02;
      if (this->UseRectangularCoordinates)
      {
        SVTK_CREATE(svtkPoints, highlightPoints);
        highlightPoints->SetNumberOfPoints(5);

        SVTK_CREATE(svtkCellArray, highA);
        highA->InsertNextCell(5);
        for (int i = 0; i < 5; ++i)
        {
          highA->InsertCellPoint(i);
        }
        highlightPoints->SetPoint(0, sinfo[0], sinfo[2], z);
        highlightPoints->SetPoint(1, sinfo[1], sinfo[2], z);
        highlightPoints->SetPoint(2, sinfo[1], sinfo[3], z);
        highlightPoints->SetPoint(3, sinfo[0], sinfo[3], z);
        highlightPoints->SetPoint(4, sinfo[0], sinfo[2], z);
        this->HighlightData->SetPoints(highlightPoints);
        this->HighlightData->SetLines(highA);
      }
      else
      {
        if (sinfo[1] - sinfo[0] != 360.)
        {
          SVTK_CREATE(svtkSectorSource, sector);
          sector->SetInnerRadius(sinfo[2]);
          sector->SetOuterRadius(sinfo[3]);
          sector->SetZCoord(z);
          sector->SetStartAngle(sinfo[0]);
          sector->SetEndAngle(sinfo[1]);

          int resolution = (int)((sinfo[1] - sinfo[0]) / 1);
          if (resolution < 1)
            resolution = 1;
          sector->SetCircumferentialResolution(resolution);
          sector->Update();

          SVTK_CREATE(svtkExtractEdges, extract);
          extract->SetInputConnection(sector->GetOutputPort());

          SVTK_CREATE(svtkAppendPolyData, append);
          append->AddInputConnection(extract->GetOutputPort());
          append->Update();

          this->HighlightData->ShallowCopy(append->GetOutput());
        }
        else
        {
          SVTK_CREATE(svtkPoints, highlightPoints);
          highlightPoints->SetNumberOfPoints(240);

          double conversion = svtkMath::Pi() / 180.;
          double current_angle = 0.;

          SVTK_CREATE(svtkCellArray, highA);
          for (int i = 0; i < 120; ++i)
          {
            highA->InsertNextCell(2);
            double current_x = sinfo[2] * cos(conversion * current_angle);
            double current_y = sinfo[2] * sin(conversion * current_angle);
            highlightPoints->SetPoint(i, current_x, current_y, z);

            current_angle += 3.;

            highA->InsertCellPoint(i);
            highA->InsertCellPoint((i + 1) % 120);
          }

          current_angle = 0.;
          for (int i = 0; i < 120; ++i)
          {
            highA->InsertNextCell(2);
            double current_x = sinfo[3] * cos(conversion * current_angle);
            double current_y = sinfo[3] * sin(conversion * current_angle);
            highlightPoints->SetPoint(120 + i, current_x, current_y, z);

            current_angle += 3.;

            highA->InsertCellPoint(120 + i);
            highA->InsertCellPoint(120 + ((i + 1) % 120));
          }
          this->HighlightData->SetPoints(highlightPoints);
          this->HighlightData->SetLines(highA);
        }
      }
      this->HighlightActor->VisibilityOn();
    }
    else
    {
      this->Balloon->SetBalloonText("");
      HighlightActor->VisibilityOff();
    }

    this->Balloon->StartWidgetInteraction(loc);

    this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
    this->GetInteractor()->Render();
  }

  this->Superclass::OnMouseMove();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleAreaSelectHover::SetHighLightColor(double r, double g, double b)
{
  this->HighlightActor->GetProperty()->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void svtkInteractorStyleAreaSelectHover::SetHighLightWidth(double lw)
{
  this->HighlightActor->GetProperty()->SetLineWidth(lw);
}

//----------------------------------------------------------------------------
double svtkInteractorStyleAreaSelectHover::GetHighLightWidth()
{
  return this->HighlightActor->GetProperty()->GetLineWidth();
}
