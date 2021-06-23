/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkChartParallelCoordinates.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkChartParallelCoordinates.h"

#include "svtkAnnotationLink.h"
#include "svtkAxis.h"
#include "svtkBrush.h"
#include "svtkCommand.h"
#include "svtkContext2D.h"
#include "svtkContextMapper2D.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScene.h"
#include "svtkDataArray.h"
#include "svtkIdTypeArray.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkPlotParallelCoordinates.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTextProperty.h"
#include "svtkTransform2D.h"

#include <algorithm>
#include <vector>

// Minimal storage class for STL containers etc.
class svtkChartParallelCoordinates::Private
{
public:
  Private()
  {
    this->Plot = svtkSmartPointer<svtkPlotParallelCoordinates>::New();
    this->Transform = svtkSmartPointer<svtkTransform2D>::New();
    this->CurrentAxis = -1;
    this->AxisResize = -1;
  }
  ~Private()
  {
    for (std::vector<svtkAxis*>::iterator it = this->Axes.begin(); it != this->Axes.end(); ++it)
    {
      (*it)->Delete();
    }
  }
  svtkSmartPointer<svtkPlotParallelCoordinates> Plot;
  svtkSmartPointer<svtkTransform2D> Transform;
  std::vector<svtkAxis*> Axes;
  std::vector<svtkVector<float, 2> > AxesSelections;
  int CurrentAxis;
  int AxisResize;
  bool InteractiveSelection;
};

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkChartParallelCoordinates);

//-----------------------------------------------------------------------------
svtkChartParallelCoordinates::svtkChartParallelCoordinates()
{
  this->Storage = new svtkChartParallelCoordinates::Private;
  this->Storage->Plot->SetParent(this);
  this->GeometryValid = false;
  this->Selection = svtkIdTypeArray::New();
  this->Storage->Plot->SetSelection(this->Selection);
  this->Storage->InteractiveSelection = false;

  // Set up default mouse button assignments for parallel coordinates.
  this->SetActionToButton(svtkChart::PAN, svtkContextMouseEvent::RIGHT_BUTTON);
  this->SetActionToButton(svtkChart::SELECT, svtkContextMouseEvent::LEFT_BUTTON);
}

//-----------------------------------------------------------------------------
svtkChartParallelCoordinates::~svtkChartParallelCoordinates()
{
  this->Storage->Plot->SetSelection(nullptr);
  delete this->Storage;
  this->Selection->Delete();
}

//-----------------------------------------------------------------------------
void svtkChartParallelCoordinates::Update()
{
  svtkTable* table = this->Storage->Plot->GetData()->GetInput();
  if (!table)
  {
    return;
  }

  if (table->GetMTime() < this->BuildTime && this->MTime < this->BuildTime &&
    this->GetScene()->GetMTime() < this->BuildTime)
  {
    return;
  }

  // Now we have a table, set up the axes accordingly, clear and build.
  if (static_cast<int>(this->Storage->Axes.size()) != this->VisibleColumns->GetNumberOfTuples())
  {
    for (std::vector<svtkAxis*>::iterator it = this->Storage->Axes.begin();
         it != this->Storage->Axes.end(); ++it)
    {
      this->RemoveItem(*it);
      (*it)->Delete();
    }
    this->Storage->Axes.clear();
    this->Storage->AxesSelections.clear();

    for (int i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
    {
      svtkAxis* axis = svtkAxis::New();
      axis->SetPosition(svtkAxis::PARALLEL);
      this->AddItem(axis);
      this->Storage->Axes.push_back(axis);
    }
    this->Storage->AxesSelections.resize(this->Storage->Axes.size(), svtkVector2f(0, 0));
  }

  // Now set up their ranges and locations
  for (int i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
  {
    double range[2];
    svtkDataArray* array =
      svtkArrayDownCast<svtkDataArray>(table->GetColumnByName(this->VisibleColumns->GetValue(i)));
    if (array)
    {
      array->GetRange(range);
    }
    svtkAxis* axis = this->Storage->Axes[i];
    if (axis->GetBehavior() == 0)
    {
      axis->SetMinimum(range[0]);
      axis->SetMaximum(range[1]);
    }
    axis->SetTitle(this->VisibleColumns->GetValue(i));
  }

  this->GeometryValid = false;
  this->BuildTime.Modified();
}

//-----------------------------------------------------------------------------
bool svtkChartParallelCoordinates::Paint(svtkContext2D* painter)
{
  if (this->GetScene()->GetViewWidth() == 0 || this->GetScene()->GetViewHeight() == 0 ||
    !this->Visible || !this->Storage->Plot->GetVisible() ||
    this->VisibleColumns->GetNumberOfTuples() < 2)
  {
    // The geometry of the chart must be valid before anything can be drawn
    return false;
  }

  this->Update();
  this->UpdateGeometry();

  // Handle selections
  svtkIdTypeArray* idArray = nullptr;
  if (this->AnnotationLink)
  {
    svtkSelection* selection = this->AnnotationLink->GetCurrentSelection();
    if (this->AnnotationLink->GetMTime() > this->Storage->Plot->GetMTime())
    {
      svtkSelectionNode* node = selection->GetNumberOfNodes() > 0 ? selection->GetNode(0) : nullptr;
      idArray = node ? svtkArrayDownCast<svtkIdTypeArray>(node->GetSelectionList()) : nullptr;
      this->Storage->Plot->SetSelection(idArray);
      this->Storage->Plot->Modified();

      // InteractiveSelection is true only when the selection has been changed by the
      // user interactively (see MouseButtonReleaseEvent). Otherwise, it means that
      // the selection has been changed indirectly. In this case, we need to reset the
      // axes selection so they will not appear out of sync with the current selection.
      if (!this->Storage->InteractiveSelection)
      {
        this->ResetAxesSelection();
      }
    }
  }
  else
  {
    svtkDebugMacro("No annotation link set.");
  }

  painter->PushMatrix();
  painter->AppendTransform(this->Storage->Transform);
  this->Storage->Plot->Paint(painter);
  painter->PopMatrix();

  // Now we have a table, set up the axes accordingly, clear and build.
  for (std::vector<svtkAxis*>::iterator it = this->Storage->Axes.begin();
       it != this->Storage->Axes.end(); ++it)
  {
    (*it)->Paint(painter);
  }

  // If there is a selected axis, draw the highlight
  if (this->Storage->CurrentAxis >= 0)
  {
    painter->GetBrush()->SetColor(200, 200, 200, 200);
    svtkAxis* axis = this->Storage->Axes[this->Storage->CurrentAxis];
    painter->DrawRect(
      axis->GetPoint1()[0] - 10, this->Point1[1], 20, this->Point2[1] - this->Point1[1]);
  }

  // Now draw our active selections
  for (size_t i = 0; i < this->Storage->AxesSelections.size(); ++i)
  {
    svtkVector<float, 2>& range = this->Storage->AxesSelections[i];
    if (range[0] != range[1])
    {
      painter->GetBrush()->SetColor(200, 20, 20, 220);
      float x = this->Storage->Axes[i]->GetPoint1()[0] - 5;
      float y = range[0];
      y *= this->Storage->Transform->GetMatrix()->GetElement(1, 1);
      y += this->Storage->Transform->GetMatrix()->GetElement(1, 2);
      float height = range[1] - range[0];
      height *= this->Storage->Transform->GetMatrix()->GetElement(1, 1);

      painter->DrawRect(x, y, 10, height);
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
void svtkChartParallelCoordinates::SetColumnVisibility(const svtkStdString& name, bool visible)
{
  if (visible)
  {
    for (svtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
    {
      if (this->VisibleColumns->GetValue(i) == name)
      {
        // Already there, nothing more needs to be done
        return;
      }
    }
    // Add the column to the end of the list
    this->VisibleColumns->InsertNextValue(name);
    this->Modified();
    this->Update();
  }
  else
  {
    // Remove the value if present
    for (svtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
    {
      if (this->VisibleColumns->GetValue(i) == name)
      {
        // Move all the later elements down by one, and reduce the size
        while (i < this->VisibleColumns->GetNumberOfTuples() - 1)
        {
          this->VisibleColumns->SetValue(i, this->VisibleColumns->GetValue(i + 1));
          ++i;
        }
        this->VisibleColumns->SetNumberOfTuples(this->VisibleColumns->GetNumberOfTuples() - 1);
        if (this->Storage->CurrentAxis >= this->VisibleColumns->GetNumberOfTuples())
        {
          this->Storage->CurrentAxis = -1;
        }
        this->Modified();
        this->Update();
        return;
      }
    }
  }
}

//-----------------------------------------------------------------------------
void svtkChartParallelCoordinates::SetColumnVisibilityAll(bool visible)
{
  // We always need to clear the current visible columns.
  this->VisibleColumns->SetNumberOfTuples(0);
  this->Storage->CurrentAxis = -1;
  if (visible)
  {
    svtkTable* table = this->GetPlot(0)->GetInput();
    for (svtkIdType i = 0; i < table->GetNumberOfColumns(); ++i)
    {
      this->SetColumnVisibility(table->GetColumnName(i), visible);
    }
  }
}

//-----------------------------------------------------------------------------
bool svtkChartParallelCoordinates::GetColumnVisibility(const svtkStdString& name)
{
  for (svtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
  {
    if (this->VisibleColumns->GetValue(i) == name)
    {
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
svtkStringArray* svtkChartParallelCoordinates::GetVisibleColumns()
{
  return this->VisibleColumns;
}

//-----------------------------------------------------------------------------
void svtkChartParallelCoordinates::SetVisibleColumns(svtkStringArray* visColumns)
{
  if (!visColumns || visColumns->GetNumberOfTuples() == 0)
  {
    this->VisibleColumns->SetNumberOfTuples(0);
  }
  else
  {
    this->VisibleColumns->SetNumberOfTuples(visColumns->GetNumberOfTuples());
    this->VisibleColumns->DeepCopy(visColumns);
  }
  if (this->Storage->CurrentAxis >= this->VisibleColumns->GetNumberOfTuples())
  {
    this->Storage->CurrentAxis = -1;
  }
  this->Modified();
  this->Update();
}

//-----------------------------------------------------------------------------
void svtkChartParallelCoordinates::SetPlot(svtkPlotParallelCoordinates* plot)
{
  this->Storage->Plot = plot;
  this->Storage->Plot->SetParent(this);
}

//-----------------------------------------------------------------------------
svtkPlot* svtkChartParallelCoordinates::GetPlot(svtkIdType)
{
  return this->Storage->Plot;
}

//-----------------------------------------------------------------------------
svtkIdType svtkChartParallelCoordinates::GetNumberOfPlots()
{
  return 1;
}

//-----------------------------------------------------------------------------
svtkAxis* svtkChartParallelCoordinates::GetAxis(int index)
{
  if (index < this->GetNumberOfAxes())
  {
    return this->Storage->Axes[index];
  }
  else
  {
    return nullptr;
  }
}

//-----------------------------------------------------------------------------
svtkIdType svtkChartParallelCoordinates::GetNumberOfAxes()
{
  return static_cast<svtkIdType>(this->Storage->Axes.size());
}

//-----------------------------------------------------------------------------
void svtkChartParallelCoordinates::UpdateGeometry()
{
  svtkVector2i geometry(this->GetScene()->GetViewWidth(), this->GetScene()->GetViewHeight());

  if (geometry.GetX() != this->Geometry[0] || geometry.GetY() != this->Geometry[1] ||
    !this->GeometryValid)
  {
    // Take up the entire window right now, this could be made configurable
    this->SetGeometry(geometry.GetData());

    svtkVector2i tileScale = this->Scene->GetLogicalTileScale();
    this->SetBorders(
      60 * tileScale.GetX(), 50 * tileScale.GetY(), 60 * tileScale.GetX(), 20 * tileScale.GetY());

    // Iterate through the axes and set them up to span the chart area.
    int xStep =
      (this->Point2[0] - this->Point1[0]) / (static_cast<int>(this->Storage->Axes.size()) - 1);
    int x = this->Point1[0];

    for (size_t i = 0; i < this->Storage->Axes.size(); ++i)
    {
      svtkAxis* axis = this->Storage->Axes[i];
      axis->SetPoint1(x, this->Point1[1]);
      axis->SetPoint2(x, this->Point2[1]);
      if (axis->GetBehavior() == 0)
      {
        axis->AutoScale();
      }
      axis->Update();
      x += xStep;
    }

    this->GeometryValid = true;
    // Cause the plot transform to be recalculated if necessary
    this->CalculatePlotTransform();
    this->Storage->Plot->Update();
  }
}

//-----------------------------------------------------------------------------
void svtkChartParallelCoordinates::CalculatePlotTransform()
{
  // In the case of parallel coordinates everything is plotted in a normalized
  // system, where the range is from 0.0 to 1.0 in the y axis, and in screen
  // coordinates along the x axis.
  if (this->Storage->Axes.empty())
  {
    return;
  }

  svtkAxis* axis = this->Storage->Axes[0];
  float* min = axis->GetPoint1();
  float* max = axis->GetPoint2();
  float yScale = 1.0f / (max[1] - min[1]);

  this->Storage->Transform->Identity();
  this->Storage->Transform->Translate(0, axis->GetPoint1()[1]);
  // Get the scale for the plot area from the x and y axes
  this->Storage->Transform->Scale(1.0, 1.0 / yScale);
}

//-----------------------------------------------------------------------------
void svtkChartParallelCoordinates::RecalculateBounds() {}

//-----------------------------------------------------------------------------
bool svtkChartParallelCoordinates::Hit(const svtkContextMouseEvent& mouse)
{
  svtkVector2i pos(mouse.GetScreenPos());
  if (pos[0] > this->Point1[0] - 10 && pos[0] < this->Point2[0] + 10 && pos[1] > this->Point1[1] &&
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
bool svtkChartParallelCoordinates::MouseEnterEvent(const svtkContextMouseEvent&)
{
  return true;
}

//-----------------------------------------------------------------------------
bool svtkChartParallelCoordinates::MouseMoveEvent(const svtkContextMouseEvent& mouse)
{
  if (mouse.GetButton() == this->Actions.Select())
  {
    // If an axis is selected, then lets try to narrow down a selection...
    if (this->Storage->CurrentAxis >= 0)
    {
      svtkVector<float, 2>& range = this->Storage->AxesSelections[this->Storage->CurrentAxis];

      // Normalize the coordinates
      float current = mouse.GetScenePos().GetY();
      current -= this->Storage->Transform->GetMatrix()->GetElement(1, 2);
      current /= this->Storage->Transform->GetMatrix()->GetElement(1, 1);

      if (current > 1.0f)
      {
        range[1] = 1.0f;
      }
      else if (current < 0.0f)
      {
        range[1] = 0.0f;
      }
      else
      {
        range[1] = current;
      }
    }
    this->Scene->SetDirty(true);
  }
  else if (mouse.GetButton() == this->Actions.Pan())
  {
    svtkAxis* axis = this->Storage->Axes[this->Storage->CurrentAxis];
    if (this->Storage->AxisResize == 0)
    {
      // Move the axis in x
      float deltaX = mouse.GetScenePos().GetX() - mouse.GetLastScenePos().GetX();

      axis->SetPoint1(axis->GetPoint1()[0] + deltaX, axis->GetPoint1()[1]);
      axis->SetPoint2(axis->GetPoint2()[0] + deltaX, axis->GetPoint2()[1]);

      svtkAxis* leftAxis = this->Storage->CurrentAxis > 0
        ? this->Storage->Axes[this->Storage->CurrentAxis - 1]
        : nullptr;

      svtkAxis* rightAxis =
        this->Storage->CurrentAxis < static_cast<int>(this->Storage->Axes.size()) - 1
        ? this->Storage->Axes[this->Storage->CurrentAxis + 1]
        : nullptr;

      if (leftAxis && axis->GetPoint1()[0] < leftAxis->GetPoint1()[0])
      {
        this->SwapAxes(this->Storage->CurrentAxis, this->Storage->CurrentAxis - 1);
        this->Storage->CurrentAxis--;
      }
      else if (rightAxis && axis->GetPoint1()[0] > rightAxis->GetPoint1()[0])
      {
        this->SwapAxes(this->Storage->CurrentAxis, this->Storage->CurrentAxis + 1);
        this->Storage->CurrentAxis++;
      }
    }
    else if (this->Storage->AxisResize == 1)
    {
      // Modify the bottom axis range...
      float deltaY = mouse.GetScenePos().GetY() - mouse.GetLastScenePos().GetY();
      float scale =
        (axis->GetPoint2()[1] - axis->GetPoint1()[1]) / (axis->GetMaximum() - axis->GetMinimum());
      axis->SetMinimum(axis->GetMinimum() - deltaY / scale);
      // If there is an active selection on the axis, remove it
      if (this->ResetAxeSelection(this->Storage->CurrentAxis))
      {
        this->ResetSelection();
      }

      // Now update everything that needs to be
      axis->Update();
      axis->RecalculateTickSpacing();
      this->Storage->Plot->Update();
    }
    else if (this->Storage->AxisResize == 2)
    {
      // Modify the bottom axis range...
      float deltaY = mouse.GetScenePos().GetY() - mouse.GetLastScenePos().GetY();
      float scale =
        (axis->GetPoint2()[1] - axis->GetPoint1()[1]) / (axis->GetMaximum() - axis->GetMinimum());
      axis->SetMaximum(axis->GetMaximum() - deltaY / scale);
      // If there is an active selection on the axis, remove it
      if (this->ResetAxeSelection(this->Storage->CurrentAxis))
      {
        this->ResetSelection();
      }

      axis->Update();
      axis->RecalculateTickSpacing();
      this->Storage->Plot->Update();
    }
    this->Scene->SetDirty(true);
  }

  return true;
}

//-----------------------------------------------------------------------------
bool svtkChartParallelCoordinates::MouseLeaveEvent(const svtkContextMouseEvent&)
{
  return true;
}

//-----------------------------------------------------------------------------
bool svtkChartParallelCoordinates::MouseButtonPressEvent(const svtkContextMouseEvent& mouse)
{
  if (mouse.GetButton() == this->Actions.Select())
  {
    // Select an axis if we are within range
    if (mouse.GetScenePos()[1] > this->Point1[1] && mouse.GetScenePos()[1] < this->Point2[1])
    {
      // Iterate over the axes, see if we are within 10 pixels of an axis
      for (size_t i = 0; i < this->Storage->Axes.size(); ++i)
      {
        svtkAxis* axis = this->Storage->Axes[i];
        if (axis->GetPoint1()[0] - 10 < mouse.GetScenePos()[0] &&
          axis->GetPoint1()[0] + 10 > mouse.GetScenePos()[0])
        {
          this->Storage->CurrentAxis = static_cast<int>(i);
          this->ResetAxeSelection(this->Storage->CurrentAxis);
          this->ResetSelection();
          // This is a manual interactive selection
          this->Storage->InteractiveSelection = true;

          // Transform into normalized coordinates
          float low = mouse.GetScenePos()[1];
          low -= this->Storage->Transform->GetMatrix()->GetElement(1, 2);
          low /= this->Storage->Transform->GetMatrix()->GetElement(1, 1);
          svtkVector<float, 2>& range = this->Storage->AxesSelections[this->Storage->CurrentAxis];
          range[0] = range[1] = low;

          this->Scene->SetDirty(true);
          return true;
        }
      }
    }
    this->Storage->CurrentAxis = -1;
    this->Scene->SetDirty(true);
    return true;
  }
  else if (mouse.GetButton() == this->Actions.Pan())
  {
    // Middle mouse button - move and zoom the axes
    // Iterate over the axes, see if we are within 10 pixels of an axis
    for (size_t i = 0; i < this->Storage->Axes.size(); ++i)
    {
      svtkAxis* axis = this->Storage->Axes[i];
      if (axis->GetPoint1()[0] - 10 < mouse.GetScenePos()[0] &&
        axis->GetPoint1()[0] + 10 > mouse.GetScenePos()[0])
      {
        this->Storage->CurrentAxis = static_cast<int>(i);
        if (mouse.GetScenePos().GetY() > axis->GetPoint1()[1] &&
          mouse.GetScenePos().GetY() < axis->GetPoint1()[1] + 20)
        {
          // Resize the bottom of the axis
          this->Storage->AxisResize = 1;
        }
        else if (mouse.GetScenePos().GetY() < axis->GetPoint2()[1] &&
          mouse.GetScenePos().GetY() > axis->GetPoint2()[1] - 20)
        {
          // Resize the top of the axis
          this->Storage->AxisResize = 2;
        }
        else
        {
          // Move the axis
          this->Storage->AxisResize = 0;
        }
      }
    }
    return true;
  }
  else
  {
    return false;
  }
}

//-----------------------------------------------------------------------------
bool svtkChartParallelCoordinates::MouseButtonReleaseEvent(const svtkContextMouseEvent& mouse)
{
  if (mouse.GetButton() == this->Actions.Select())
  {
    if (this->Storage->CurrentAxis >= 0)
    {
      svtkVector<float, 2>& range = this->Storage->AxesSelections[this->Storage->CurrentAxis];

      float final = mouse.GetScenePos()[1];
      final -= this->Storage->Transform->GetMatrix()->GetElement(1, 2);
      final /= this->Storage->Transform->GetMatrix()->GetElement(1, 1);

      // Set the final mouse position
      if (final > 1.0)
      {
        range[1] = 1.0;
      }
      else if (final < 0.0)
      {
        range[1] = 0.0;
      }
      else
      {
        range[1] = final;
      }

      if (range[0] == range[1])
      {
        this->ResetSelection();
      }
      else
      {
        // Add a new selection
        if (range[0] < range[1])
        {
          this->Storage->Plot->SetSelectionRange(this->Storage->CurrentAxis, range[0], range[1]);
        }
        else
        {
          this->Storage->Plot->SetSelectionRange(this->Storage->CurrentAxis, range[1], range[0]);
        }
      }
      // This is a manual interactive selection
      this->Storage->InteractiveSelection = true;

      if (this->AnnotationLink)
      {
        svtkSelection* selection = svtkSelection::New();
        svtkSelectionNode* node = svtkSelectionNode::New();
        selection->AddNode(node);
        node->SetContentType(svtkSelectionNode::INDICES);
        node->SetFieldType(svtkSelectionNode::POINT);

        node->SetSelectionList(this->Storage->Plot->GetSelection());
        this->AnnotationLink->SetCurrentSelection(selection);
        selection->Delete();
        node->Delete();
      }
      this->InvokeEvent(svtkCommand::SelectionChangedEvent);
      this->Scene->SetDirty(true);
    }
    return true;
  }
  else if (mouse.GetButton() == this->Actions.Pan())
  {
    this->Storage->CurrentAxis = -1;
    this->Storage->AxisResize = -1;
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool svtkChartParallelCoordinates::MouseWheelEvent(const svtkContextMouseEvent&, int)
{
  return true;
}

//-----------------------------------------------------------------------------
void svtkChartParallelCoordinates::ResetSelection()
{
  // This function takes care of resetting the selection of the chart
  // Reset the axes.
  this->Storage->Plot->ResetSelectionRange();

  // Now set the remaining selections that were kept
  for (size_t i = 0; i < this->Storage->AxesSelections.size(); ++i)
  {
    svtkVector<float, 2>& range = this->Storage->AxesSelections[i];
    if (range[0] != range[1])
    {
      // Process the selected range and display this
      if (range[0] < range[1])
      {
        this->Storage->Plot->SetSelectionRange(static_cast<int>(i), range[0], range[1]);
      }
      else
      {
        this->Storage->Plot->SetSelectionRange(static_cast<int>(i), range[1], range[0]);
      }
    }
  }
}

//-----------------------------------------------------------------------------
bool svtkChartParallelCoordinates::ResetAxeSelection(int axe)
{
  svtkVector<float, 2>& range = this->Storage->AxesSelections[axe];
  if (range[0] != range[1])
  {
    range[0] = range[1] = 0.0f;
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void svtkChartParallelCoordinates::ResetAxesSelection()
{
  for (size_t i = 0; i < this->Storage->AxesSelections.size(); ++i)
  {
    this->ResetAxeSelection(static_cast<int>(i));
  }
}

//-----------------------------------------------------------------------------
void svtkChartParallelCoordinates::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void svtkChartParallelCoordinates::SwapAxes(int a1, int a2)
{
  // only neighboring axes
  if (abs(a1 - a2) != 1)
    return;

  svtkAxis* axisTmp = this->Storage->Axes[a1];
  this->Storage->Axes[a1] = this->Storage->Axes[a2];
  this->Storage->Axes[a2] = axisTmp;

  svtkVector<float, 2> selTmp = this->Storage->AxesSelections[a1];
  this->Storage->AxesSelections[a1] = this->Storage->AxesSelections[a2];
  this->Storage->AxesSelections[a2] = selTmp;

  svtkStdString colTmp = this->VisibleColumns->GetValue(a1);
  this->VisibleColumns->SetValue(a1, this->VisibleColumns->GetValue(a2));
  this->VisibleColumns->SetValue(a2, colTmp);

  this->Storage->Plot->Update();
}
