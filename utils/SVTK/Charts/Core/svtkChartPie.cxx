/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkChartPie.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkChartPie.h"

#include "svtkObjectFactory.h"

#include "svtkContext2D.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScene.h"
#include "svtkPoints2D.h"
#include "svtkTransform2D.h"

#include "svtkPlotPie.h"

#include "svtkChartLegend.h"
#include "svtkTooltipItem.h"

#include <sstream>

class svtkChartPiePrivate
{
public:
  svtkChartPiePrivate() = default;

  svtkSmartPointer<svtkPlotPie> Plot;
};

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkChartPie);

//-----------------------------------------------------------------------------
svtkChartPie::svtkChartPie()
{
  this->Legend = svtkChartLegend::New();
  this->Legend->SetChart(this);
  this->Legend->SetVisible(false);
  this->AddItem(this->Legend);
  this->Legend->Delete();

  this->Tooltip = svtkTooltipItem::New();
  this->Tooltip->SetVisible(false);

  this->Private = new svtkChartPiePrivate();
}

//-----------------------------------------------------------------------------
svtkChartPie::~svtkChartPie()
{
  this->Tooltip->Delete();
  delete this->Private;
}

//-----------------------------------------------------------------------------
void svtkChartPie::Update()
{
  if (this->Private->Plot && this->Private->Plot->GetVisible())
  {
    this->Private->Plot->Update();
  }

  this->Legend->Update();
  this->Legend->SetVisible(this->ShowLegend);
}

//-----------------------------------------------------------------------------
bool svtkChartPie::Paint(svtkContext2D* painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  svtkDebugMacro(<< "Paint event called.");

  int geometry[] = { this->GetScene()->GetSceneWidth(), this->GetScene()->GetSceneHeight() };
  if (geometry[0] == 0 || geometry[1] == 0 || !this->Visible)
  {
    // The geometry of the chart must be valid before anything can be drawn
    return false;
  }

  this->Update();

  if (geometry[0] != this->Geometry[0] || geometry[1] != this->Geometry[1])
  {
    // Take up the entire window right now, this could be made configurable
    this->SetGeometry(geometry);

    svtkVector2i tileScale = this->Scene->GetLogicalTileScale();
    this->SetBorders(
      20 * tileScale.GetX(), 20 * tileScale.GetY(), 20 * tileScale.GetX(), 20 * tileScale.GetY());

    // Put the legend in the top corner of the chart
    svtkRectf rect = this->Legend->GetBoundingRect(painter);
    this->Legend->SetPoint(this->Point2[0] - rect.GetWidth(), this->Point2[1] - rect.GetHeight());

    // Set the dimensions of the Plot
    if (this->Private->Plot)
    {
      this->Private->Plot->SetDimensions(20, 20, this->Geometry[0] - 40, this->Geometry[1] - 40);
    }
  }

  this->PaintChildren(painter);

  if (this->Title)
  {
    svtkPoints2D* rect = svtkPoints2D::New();
    rect->InsertNextPoint(this->Point1[0], this->Point2[1]);
    rect->InsertNextPoint(this->Point2[0] - this->Point1[0], 10);
    painter->ApplyTextProp(this->TitleProperties);
    painter->DrawStringRect(rect, this->Title);
    rect->Delete();
  }

  this->Tooltip->Paint(painter);

  return true;
}

//-----------------------------------------------------------------------------
void svtkChartPie::SetScene(svtkContextScene* scene)
{
  this->svtkAbstractContextItem::SetScene(scene);
  this->Tooltip->SetScene(scene);
}

//-----------------------------------------------------------------------------
svtkPlot* svtkChartPie::AddPlot(int /* type */)
{
  if (!this->Private->Plot)
  {
    this->Private->Plot = svtkSmartPointer<svtkPlotPie>::New();
    this->AddItem(this->Private->Plot);
  }
  return this->Private->Plot;
}

//-----------------------------------------------------------------------------
svtkPlot* svtkChartPie::GetPlot(svtkIdType index)
{
  if (index == 0)
  {
    return this->Private->Plot;
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
svtkIdType svtkChartPie::GetNumberOfPlots()
{
  if (this->Private->Plot)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

//-----------------------------------------------------------------------------
void svtkChartPie::SetShowLegend(bool visible)
{
  this->svtkChart::SetShowLegend(visible);
  this->Legend->SetVisible(visible);
}

//-----------------------------------------------------------------------------
svtkChartLegend* svtkChartPie::GetLegend()
{
  return this->Legend;
}

//-----------------------------------------------------------------------------
bool svtkChartPie::Hit(const svtkContextMouseEvent& mouse)
{
  svtkVector2i pos(mouse.GetScreenPos());
  if (pos[0] > this->Point1[0] && pos[0] < this->Point2[0] && pos[1] > this->Point1[1] &&
    pos[1] < this->Point2[1])
  {
    return true;
  }
  else
  {
    return false;
  }
}

//-----------------------------------------------------------------------------
bool svtkChartPie::MouseEnterEvent(const svtkContextMouseEvent&)
{
  return true;
}

//-----------------------------------------------------------------------------
bool svtkChartPie::MouseMoveEvent(const svtkContextMouseEvent& mouse)
{
  if (mouse.GetButton() == svtkContextMouseEvent::NO_BUTTON)
  {
    this->Scene->SetDirty(true);
    this->Tooltip->SetVisible(this->LocatePointInPlots(mouse));
  }

  return true;
}

//-----------------------------------------------------------------------------
bool svtkChartPie::MouseLeaveEvent(const svtkContextMouseEvent&)
{
  return true;
}

//-----------------------------------------------------------------------------
bool svtkChartPie::MouseButtonPressEvent(const svtkContextMouseEvent& /*mouse*/)
{
  return true;
}

//-----------------------------------------------------------------------------
bool svtkChartPie::MouseButtonReleaseEvent(const svtkContextMouseEvent& /*mouse*/)
{
  return true;
}

//-----------------------------------------------------------------------------
bool svtkChartPie::MouseWheelEvent(const svtkContextMouseEvent&, int /*delta*/)
{
  return true;
}

bool svtkChartPie::LocatePointInPlots(const svtkContextMouseEvent& mouse)
{
  if (!this->Private->Plot || !this->Private->Plot->GetVisible())
  {
    return false;
  }
  else
  {
    int dimensions[4];
    svtkVector2f position(mouse.GetScreenPos().Cast<float>().GetData());
    svtkVector2f tolerance(5, 5);
    svtkVector2f plotPos(0, 0);
    this->Private->Plot->GetDimensions(dimensions);

    svtkVector2i pos(mouse.GetScreenPos());
    if (pos[0] >= dimensions[0] && pos[0] <= dimensions[0] + dimensions[2] &&
      pos[1] >= dimensions[1] && pos[1] <= dimensions[1] + dimensions[3])
    {
      int labelIndex = this->Private->Plot->GetNearestPoint(position, tolerance, &plotPos);
      if (labelIndex >= 0)
      {
        const char* label = this->Private->Plot->GetLabel(labelIndex);
        std::ostringstream ostr;
        ostr << label << ": " << plotPos.GetY();
        this->Tooltip->SetText(ostr.str().c_str());
        this->Tooltip->SetPosition(mouse.GetScreenPos()[0] + 2, mouse.GetScreenPos()[1] + 2);
        return true;
      }
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
void svtkChartPie::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Private->Plot)
  {
    os << indent << "Plot: " << endl;
    this->Private->Plot->PrintSelf(os, indent.GetNextIndent());
  }
}
