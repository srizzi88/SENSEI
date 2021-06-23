/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDendrogramItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDendrogramItem.h"

#include "svtkBrush.h"
#include "svtkColorLegend.h"
#include "svtkContext2D.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScene.h"
#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkGraphLayout.h"
#include "svtkIdTypeArray.h"
#include "svtkIntArray.h"
#include "svtkLookupTable.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkPruneTreeFilter.h"
#include "svtkStringArray.h"
#include "svtkTextProperty.h"
#include "svtkTransform2D.h"
#include "svtkTree.h"
#include "svtkTreeLayoutStrategy.h"
#include "svtkUnsignedIntArray.h"

#include <algorithm>
#include <queue>
#include <sstream>

svtkStandardNewMacro(svtkDendrogramItem);

//-----------------------------------------------------------------------------
svtkDendrogramItem::svtkDendrogramItem()
  : PositionVector(0, 0)
{
  this->Position = this->PositionVector.GetData();
  this->DendrogramBuildTime = 0;
  this->Interactive = true;
  this->ColorTree = false;
  this->LegendPositionSet = false;
  this->Tree = svtkSmartPointer<svtkTree>::New();
  this->PrunedTree = svtkSmartPointer<svtkTree>::New();
  this->LayoutTree = svtkSmartPointer<svtkTree>::New();

  /* initialize bounds with impossible values */
  this->MinX = 1.0;
  this->MinY = 1.0;
  this->MaxX = 0.0;
  this->MaxY = 0.0;

  this->LabelWidth = 0.0;
  this->LineWidth = 1.0;

  this->NumberOfLeafNodes = 0;
  this->MultiplierX = 100.0;
  this->MultiplierY = 100.0;
  this->LeafSpacing = 18.0;

  this->PruneFilter->SetShouldPruneParentVertex(false);

  this->ExtendLeafNodes = false;
  this->DrawLabels = true;
  this->DisplayNumberOfCollapsedLeafNodes = true;

  this->DistanceArrayName = "node weight";
  this->VertexNameArrayName = "node name";

  this->ColorLegend->SetVisible(false);
  this->ColorLegend->DrawBorderOn();
  this->ColorLegend->CacheBoundsOff();
  this->AddItem(this->ColorLegend);
}

//-----------------------------------------------------------------------------
svtkDendrogramItem::~svtkDendrogramItem() = default;

//-----------------------------------------------------------------------------
void svtkDendrogramItem::SetPosition(const svtkVector2f& pos)
{
  this->PositionVector = pos;
  this->DendrogramBuildTime = 0;
}

//-----------------------------------------------------------------------------
svtkVector2f svtkDendrogramItem::GetPositionVector()
{
  return this->PositionVector;
}

//-----------------------------------------------------------------------------
void svtkDendrogramItem::SetTree(svtkTree* tree)
{
  if (tree == nullptr || tree->GetNumberOfVertices() == 0)
  {
    this->Tree = svtkSmartPointer<svtkTree>::New();
    this->PrunedTree = svtkSmartPointer<svtkTree>::New();
    this->LayoutTree = svtkSmartPointer<svtkTree>::New();
    return;
  }

  this->Tree = tree;

  // initialize some additional arrays for the tree's vertex data
  svtkNew<svtkUnsignedIntArray> vertexIsPruned;
  vertexIsPruned->SetNumberOfComponents(1);
  vertexIsPruned->SetName("VertexIsPruned");
  vertexIsPruned->SetNumberOfValues(this->Tree->GetNumberOfVertices());
  vertexIsPruned->FillComponent(0, 0.0);
  this->Tree->GetVertexData()->AddArray(vertexIsPruned);

  svtkNew<svtkIdTypeArray> originalId;
  originalId->SetNumberOfComponents(1);
  originalId->SetName("OriginalId");
  svtkIdType numVertices = this->Tree->GetNumberOfVertices();
  originalId->SetNumberOfValues(numVertices);
  for (svtkIdType i = 0; i < numVertices; ++i)
  {
    originalId->SetValue(i, i);
  }
  this->Tree->GetVertexData()->AddArray(originalId);

  // make a copy of the full tree for later pruning
  this->PrunedTree->DeepCopy(this->Tree);

  // setup the lookup table that's used to color the triangles representing
  // collapsed subtrees.  First we find maximum possible value.
  svtkIdType root = this->Tree->GetRoot();
  if (this->Tree->GetNumberOfChildren(root) == 1)
  {
    root = this->Tree->GetChild(root, 0);
  }
  int numLeavesInBiggestSubTree = 0;
  for (svtkIdType child = 0; child < this->Tree->GetNumberOfChildren(root); ++child)
  {
    svtkIdType childVertex = this->Tree->GetChild(root, child);
    int numLeaves = this->CountLeafNodes(childVertex);
    if (numLeaves > numLeavesInBiggestSubTree)
    {
      numLeavesInBiggestSubTree = numLeaves;
    }
  }

  double rangeMinimum = 2.0;
  if (numLeavesInBiggestSubTree < rangeMinimum)
  {
    rangeMinimum = numLeavesInBiggestSubTree;
  }

  this->TriangleLookupTable->SetNumberOfTableValues(256);
  this->TriangleLookupTable->SetHueRange(0.5, 0.045);
  this->TriangleLookupTable->SetRange(rangeMinimum, static_cast<double>(numLeavesInBiggestSubTree));
  this->TriangleLookupTable->Build();
}

//-----------------------------------------------------------------------------
svtkTree* svtkDendrogramItem::GetTree()
{
  return this->Tree;
}

//-----------------------------------------------------------------------------
svtkTree* svtkDendrogramItem::GetPrunedTree()
{
  return this->PrunedTree;
}

//-----------------------------------------------------------------------------
bool svtkDendrogramItem::Paint(svtkContext2D* painter)
{
  if (this->Tree->GetNumberOfVertices() == 0)
  {
    return true;
  }

  this->PrepareToPaint(painter);
  this->PaintBuffers(painter);
  this->PaintChildren(painter);
  return true;
}

//-----------------------------------------------------------------------------
void svtkDendrogramItem::PrepareToPaint(svtkContext2D* painter)
{
  if (this->IsDirty())
  {
    this->RebuildBuffers();
  }
  this->ComputeLabelWidth(painter);
}

//-----------------------------------------------------------------------------
bool svtkDendrogramItem::IsDirty()
{
  if (this->Tree->GetNumberOfVertices() == 0)
  {
    return false;
  }
  if (this->MTime > this->DendrogramBuildTime)
  {
    return true;
  }
  if (this->PrunedTree->GetMTime() > this->DendrogramBuildTime)
  {
    return true;
  }
  if (this->Tree->GetMTime() > this->DendrogramBuildTime)
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void svtkDendrogramItem::RebuildBuffers()
{
  if (this->Tree->GetNumberOfVertices() == 0)
  {
    return;
  }

  // Special case where our input tree has been modified.  Refresh PrunedTree
  // to be an up-to-date full copy of it.
  if (this->Tree->GetMTime() > this->PrunedTree->GetMTime())
  {
    this->PrunedTree->DeepCopy(this->Tree);
  }

  int orientation = this->GetOrientation();

  svtkNew<svtkTreeLayoutStrategy> strategy;

  if (this->PrunedTree->GetVertexData()->GetAbstractArray(this->DistanceArrayName) != nullptr)
  {
    strategy->SetDistanceArrayName(this->DistanceArrayName);
  }

  strategy->SetLeafSpacing(1.0);

  strategy->SetRotation(this->GetAngleForOrientation(orientation));

  this->Layout->SetLayoutStrategy(strategy);
  this->Layout->SetInputData(this->PrunedTree);
  this->Layout->Update();
  this->LayoutTree = svtkTree::SafeDownCast(this->Layout->GetOutput());

  this->CountLeafNodes();
  this->ComputeMultipliers();
  this->ComputeBounds();

  if (this->ColorTree && !this->LegendPositionSet)
  {
    this->PositionColorLegend();
  }

  if (this->PrunedTree->GetMTime() > this->MTime)
  {
    this->DendrogramBuildTime = this->PrunedTree->GetMTime();
  }
  else
  {
    this->DendrogramBuildTime = this->MTime;
  }
}

//-----------------------------------------------------------------------------
void svtkDendrogramItem::ComputeMultipliers()
{
  double xMax = 1;
  double yMax = 1;
  double targetPoint[3];
  if (this->Tree->GetNumberOfVertices() > 0)
  {
    for (svtkIdType edge = 0; edge < this->LayoutTree->GetNumberOfEdges(); ++edge)
    {
      svtkIdType target = this->LayoutTree->GetTargetVertex(edge);
      this->LayoutTree->GetPoint(target, targetPoint);
      double x = fabs(targetPoint[0]);
      double y = fabs(targetPoint[1]);
      if (x > xMax)
      {
        xMax = x;
      }
      if (y > yMax)
      {
        yMax = y;
      }
    }
  }

  int orientation = this->GetOrientation();
  if (orientation == svtkDendrogramItem::LEFT_TO_RIGHT ||
    orientation == svtkDendrogramItem::RIGHT_TO_LEFT)
  {
    this->MultiplierX = (this->LeafSpacing * (this->NumberOfLeafNodes - 1)) / yMax;
    this->MultiplierY = this->MultiplierX;
  }
  else
  {
    this->MultiplierY = (this->LeafSpacing * (this->NumberOfLeafNodes - 1)) / xMax;
    this->MultiplierX = this->MultiplierY;
  }
}

//-----------------------------------------------------------------------------
void svtkDendrogramItem::ComputeBounds()
{
  this->MinX = SVTK_DOUBLE_MAX;
  this->MinY = SVTK_DOUBLE_MAX;
  this->MaxX = SVTK_DOUBLE_MIN;
  this->MaxY = SVTK_DOUBLE_MIN;

  double sourcePoint[3];
  double targetPoint[3];

  for (svtkIdType edge = 0; edge < this->LayoutTree->GetNumberOfEdges(); ++edge)
  {
    svtkIdType source = this->LayoutTree->GetSourceVertex(edge);
    this->LayoutTree->GetPoint(source, sourcePoint);
    double x0 = this->Position[0] + sourcePoint[0] * this->MultiplierX;
    double y0 = this->Position[1] + sourcePoint[1] * this->MultiplierY;

    svtkIdType target = this->LayoutTree->GetTargetVertex(edge);
    this->LayoutTree->GetPoint(target, targetPoint);
    double x1 = this->Position[0] + targetPoint[0] * this->MultiplierX;
    double y1 = this->Position[1] + targetPoint[1] * this->MultiplierY;

    if (x0 < this->MinX)
    {
      this->MinX = x0;
    }
    if (y0 < this->MinY)
    {
      this->MinY = y0;
    }
    if (x0 > this->MaxX)
    {
      this->MaxX = x0;
    }
    if (y0 > this->MaxY)
    {
      this->MaxY = y0;
    }
    if (x1 < this->MinX)
    {
      this->MinX = x1;
    }
    if (y1 < this->MinY)
    {
      this->MinY = y1;
    }
    if (x1 > this->MaxX)
    {
      this->MaxX = x1;
    }
    if (y1 > this->MaxY)
    {
      this->MaxY = y1;
    }
  }
}

//-----------------------------------------------------------------------------
void svtkDendrogramItem::CountLeafNodes()
{
  // figure out how many leaf nodes we have.
  this->NumberOfLeafNodes = 0;
  for (svtkIdType vertex = 0; vertex < this->LayoutTree->GetNumberOfVertices(); ++vertex)
  {
    if (!this->LayoutTree->IsLeaf(vertex))
    {
      continue;
    }
    ++this->NumberOfLeafNodes;
  }
}

//-----------------------------------------------------------------------------
int svtkDendrogramItem::CountLeafNodes(svtkIdType vertex)
{
  // figure out how many leaf nodes descend from vertex.
  int numLeaves = 0;
  for (svtkIdType child = 0; child < this->Tree->GetNumberOfChildren(vertex); ++child)
  {
    svtkIdType childVertex = this->Tree->GetChild(vertex, child);
    if (this->Tree->IsLeaf(childVertex))
    {
      ++numLeaves;
    }
    else
    {
      numLeaves += this->CountLeafNodes(childVertex);
    }
  }
  return numLeaves;
}

//-----------------------------------------------------------------------------
void svtkDendrogramItem::PaintBuffers(svtkContext2D* painter)
{
  // Calculate the extent of the data that is visible within the window.
  this->UpdateVisibleSceneExtent(painter);

  double xStart, yStart;
  double sourcePoint[3];
  double targetPoint[3];
  int numberOfCollapsedSubTrees = 0;

  svtkUnsignedIntArray* vertexIsPruned =
    svtkArrayDownCast<svtkUnsignedIntArray>(this->Tree->GetVertexData()->GetArray("VertexIsPruned"));

  int orientation = this->GetOrientation();

  float previousPenWidth = painter->GetPen()->GetWidth();
  painter->GetPen()->SetWidth(this->LineWidth);

  // draw the tree
  for (svtkIdType edge = 0; edge < this->LayoutTree->GetNumberOfEdges(); ++edge)
  {
    svtkIdType source = this->LayoutTree->GetSourceVertex(edge);
    svtkIdType target = this->LayoutTree->GetTargetVertex(edge);

    this->LayoutTree->GetPoint(source, sourcePoint);
    this->LayoutTree->GetPoint(target, targetPoint);

    double x0 = this->Position[0] + sourcePoint[0] * this->MultiplierX;
    double y0 = this->Position[1] + sourcePoint[1] * this->MultiplierY;
    double x1 = this->Position[0] + targetPoint[0] * this->MultiplierX;
    double y1 = this->Position[1] + targetPoint[1] * this->MultiplierY;

    // check if the target vertex is the root of a collapsed tree
    bool alreadyDrewCollapsedSubTree = false;
    svtkIdType originalId = this->GetOriginalId(target);

    double color[4];
    double colorKey;
    if (vertexIsPruned->GetValue(originalId) > 0)
    {
      ++numberOfCollapsedSubTrees;

      float trianglePoints[6], triangleLabelX, triangleLabelY;
      switch (orientation)
      {
        case svtkDendrogramItem::DOWN_TO_UP:
          trianglePoints[0] = x1;
          trianglePoints[1] = y0;
          trianglePoints[2] = x1 - this->LeafSpacing / 2;
          trianglePoints[3] = this->MaxY;
          trianglePoints[4] = x1 + this->LeafSpacing / 2;
          trianglePoints[5] = this->MaxY;
          triangleLabelX = trianglePoints[0];
          triangleLabelY = trianglePoints[3] - 1;
          painter->GetTextProp()->SetJustificationToRight();
          break;
        case svtkDendrogramItem::RIGHT_TO_LEFT:
          trianglePoints[0] = x0;
          trianglePoints[1] = y1;
          trianglePoints[2] = this->MinX;
          trianglePoints[3] = y1 - this->LeafSpacing / 2;
          trianglePoints[4] = this->MinX;
          trianglePoints[5] = y1 + this->LeafSpacing / 2;
          triangleLabelX = trianglePoints[2] + 1;
          triangleLabelY = trianglePoints[1];
          painter->GetTextProp()->SetJustificationToLeft();
          break;
        case svtkDendrogramItem::UP_TO_DOWN:
          trianglePoints[0] = x1;
          trianglePoints[1] = y0;
          trianglePoints[2] = x1 - this->LeafSpacing / 2;
          trianglePoints[3] = this->MinY;
          trianglePoints[4] = x1 + this->LeafSpacing / 2;
          trianglePoints[5] = this->MinY;
          triangleLabelX = trianglePoints[0];
          triangleLabelY = trianglePoints[3] + 1;
          painter->GetTextProp()->SetJustificationToRight();
          break;
        case svtkDendrogramItem::LEFT_TO_RIGHT:
        default:
          trianglePoints[0] = x0;
          trianglePoints[1] = y1;
          trianglePoints[2] = this->MaxX;
          trianglePoints[3] = y1 - this->LeafSpacing / 2;
          trianglePoints[4] = this->MaxX;
          trianglePoints[5] = y1 + this->LeafSpacing / 2;
          triangleLabelX = trianglePoints[2] - 1;
          triangleLabelY = trianglePoints[1];
          painter->GetTextProp()->SetJustificationToRight();
          break;
      }

      if (this->LineIsVisible(
            trianglePoints[0], trianglePoints[1], trianglePoints[2], trianglePoints[3]) ||
        this->LineIsVisible(
          trianglePoints[0], trianglePoints[1], trianglePoints[4], trianglePoints[5]) ||
        this->LineIsVisible(
          trianglePoints[2], trianglePoints[3], trianglePoints[4], trianglePoints[5]))
      {
        colorKey = static_cast<double>(vertexIsPruned->GetValue(originalId));
        this->TriangleLookupTable->GetColor(colorKey, color);
        painter->GetBrush()->SetColorF(color[0], color[1], color[2]);
        painter->DrawPolygon(trianglePoints, 3);

        if (this->DisplayNumberOfCollapsedLeafNodes)
        {
          unsigned int numCollapsedLeafNodes = vertexIsPruned->GetValue(originalId);
          std::stringstream ss;
          ss << numCollapsedLeafNodes;

          painter->GetTextProp()->SetVerticalJustificationToCentered();
          painter->GetTextProp()->SetOrientation(this->GetTextAngleForOrientation(orientation));
          painter->DrawString(triangleLabelX, triangleLabelY, ss.str());
        }
      }
      alreadyDrewCollapsedSubTree = true;
    }

    // color this portion of the tree based on the target node
    if (this->ColorTree)
    {
      colorKey = this->ColorArray->GetValue(target);
      this->TreeLookupTable->GetColor(colorKey, color);
      painter->GetPen()->SetColorF(color[0], color[1], color[2]);
    }

    // when drawing horizontal trees, we want to draw the vertical segment
    // before the horizontal segment.  The opposite is true when we are
    // drawing vertical trees.  We use the variables midpointX and midpointY
    // to handle this behavior.  extendedX and extendedY are used similarly
    // for extending leaf nodes below.
    double midpointX, midpointY, extendedX, extendedY;
    switch (orientation)
    {
      case svtkDendrogramItem::DOWN_TO_UP:
        midpointX = x1;
        midpointY = y0;
        extendedX = x1;
        extendedY = this->MaxY;
        break;
      case svtkDendrogramItem::RIGHT_TO_LEFT:
        midpointX = x0;
        midpointY = y1;
        extendedX = this->MinX;
        extendedY = y1;
        break;
      case svtkDendrogramItem::UP_TO_DOWN:
        midpointX = x1;
        midpointY = y0;
        extendedX = x1;
        extendedY = this->MinY;
        break;
      case svtkDendrogramItem::LEFT_TO_RIGHT:
      default:
        midpointX = x0;
        midpointY = y1;
        extendedX = this->MaxX;
        extendedY = y1;
        break;
    }

    if (this->LineIsVisible(x0, y0, midpointX, midpointY))
    {
      painter->DrawLine(x0, y0, midpointX, midpointY);
    }
    if (!alreadyDrewCollapsedSubTree)
    {
      if (this->LineIsVisible(midpointX, midpointY, x1, y1))
      {
        painter->DrawLine(midpointX, midpointY, x1, y1);
      }

      // extend leaf nodes so they line up
      if (this->ExtendLeafNodes && !(x1 == extendedX && y1 == extendedY) &&
        this->LayoutTree->IsLeaf(target) && this->LineIsVisible(x1, y1, extendedX, extendedY))
      {
        // we draw these extensions as grey lines to distinguish them
        // from the actual lengths of the leaf nodes.
        painter->GetPen()->SetColorF(0.75, 0.75, 0.75);

        painter->DrawLine(x1, y1, extendedX, extendedY);

        // revert to drawing black lines when we're done
        painter->GetPen()->SetColorF(0.0, 0.0, 0.0);
      }
    }

    if (this->ColorTree)
    {
      // revert to drawing thin black lines by default
      painter->GetPen()->SetColorF(0.0, 0.0, 0.0);
    }
  }

  painter->GetPen()->SetWidth(previousPenWidth);

  // the remainder of this function involves drawing the leaf node labels,
  // so we can return now if that feature has been disabled.
  if (!this->DrawLabels)
  {
    return;
  }

  // special case: all the true leaf nodes have been collapsed.
  // This means that there aren't any labels left to draw.
  if (this->NumberOfLeafNodes <= numberOfCollapsedSubTrees)
  {
    return;
  }

  //"Igq" selected for range of height
  int fontSize = painter->ComputeFontSizeForBoundedString("Igq", SVTK_FLOAT_MAX, this->LeafSpacing);
  // make sure our current zoom level allows for a legibly-sized font
  if (fontSize < 8)
  {
    return;
  }

  // leave a small amount of space between the tree and the vertex labels
  double spacing = this->LeafSpacing * 0.5;

  // set up our text property to draw leaf node labels
  painter->GetTextProp()->SetColor(0.0, 0.0, 0.0);
  painter->GetTextProp()->SetJustificationToLeft();
  painter->GetTextProp()->SetVerticalJustificationToCentered();
  painter->GetTextProp()->SetOrientation(this->GetTextAngleForOrientation(orientation));

  // make sure some of the labels would be visible on screen
  switch (orientation)
  {
    case svtkDendrogramItem::DOWN_TO_UP:
      if (this->SceneBottomLeft[1] > this->MaxY + spacing ||
        this->SceneTopRight[1] < this->MaxY + spacing)
      {
        return;
      }
      break;
    case svtkDendrogramItem::RIGHT_TO_LEFT:
      if (this->SceneBottomLeft[0] > this->MinX - spacing ||
        this->SceneTopRight[0] < this->MinX - spacing)
      {
        return;
      }
      painter->GetTextProp()->SetJustificationToRight();
      break;
    case svtkDendrogramItem::UP_TO_DOWN:
      if (this->SceneBottomLeft[1] > this->MinY - spacing ||
        this->SceneTopRight[1] < this->MinY - spacing)
      {
        return;
      }
      painter->GetTextProp()->SetJustificationToRight();
      break;
    case svtkDendrogramItem::LEFT_TO_RIGHT:
    default:
      if (this->SceneBottomLeft[0] > this->MaxX + spacing ||
        this->SceneTopRight[0] < this->MaxX + spacing)
      {
        return;
      }
      break;
  }

  // get array of node names from the tree
  svtkStringArray* vertexNames = svtkArrayDownCast<svtkStringArray>(
    this->LayoutTree->GetVertexData()->GetAbstractArray(this->VertexNameArrayName));

  // find our leaf nodes & draw their labels
  for (svtkIdType vertex = 0; vertex < this->LayoutTree->GetNumberOfVertices(); ++vertex)
  {
    if (!this->LayoutTree->IsLeaf(vertex))
    {
      continue;
    }

    double point[3];
    this->LayoutTree->GetPoint(vertex, point);
    switch (orientation)
    {
      case svtkDendrogramItem::DOWN_TO_UP:
        xStart = this->Position[0] + point[0] * this->MultiplierX;
        yStart = this->MaxY + spacing;
        break;
      case svtkDendrogramItem::RIGHT_TO_LEFT:
        xStart = this->MinX - spacing;
        yStart = this->Position[1] + point[1] * this->MultiplierY;
        break;
      case svtkDendrogramItem::UP_TO_DOWN:
        xStart = this->Position[0] + point[0] * this->MultiplierX;
        yStart = this->MinY - spacing;
        break;
      case svtkDendrogramItem::LEFT_TO_RIGHT:
      default:
        xStart = this->MaxX + spacing;
        yStart = this->Position[1] + point[1] * this->MultiplierY;
        break;
    }

    std::string vertexName = vertexNames->GetValue(vertex);
    if (this->SceneBottomLeft[0] < xStart && this->SceneTopRight[0] > xStart &&
      this->SceneBottomLeft[1] < yStart && this->SceneTopRight[1] > yStart)
    {
      painter->DrawString(xStart, yStart, vertexName);
    }
  }
}

//-----------------------------------------------------------------------------
void svtkDendrogramItem::UpdateVisibleSceneExtent(svtkContext2D* painter)
{
  float position[2];
  painter->GetTransform()->GetPosition(position);
  this->SceneBottomLeft[0] = -position[0];
  this->SceneBottomLeft[1] = -position[1];
  this->SceneBottomLeft[2] = 0.0;

  this->SceneTopRight[0] = static_cast<double>(this->GetScene()->GetSceneWidth() - position[0]);
  this->SceneTopRight[1] = static_cast<double>(this->GetScene()->GetSceneHeight() - position[1]);
  this->SceneTopRight[2] = 0.0;
  svtkNew<svtkMatrix3x3> inverse;
  painter->GetTransform()->GetInverse(inverse);
  inverse->MultiplyPoint(this->SceneBottomLeft, this->SceneBottomLeft);
  inverse->MultiplyPoint(this->SceneTopRight, this->SceneTopRight);
}

//-----------------------------------------------------------------------------
bool svtkDendrogramItem::LineIsVisible(double x0, double y0, double x1, double y1)
{
  // is the line degenerate, if so skip
  if (x0 == x1 && y0 == y1)
  {
    return false;
  }
  // use local variables to improve readability
  double xMinScene = this->SceneBottomLeft[0];
  double yMinScene = this->SceneBottomLeft[1];
  double xMaxScene = this->SceneTopRight[0];
  double yMaxScene = this->SceneTopRight[1];

  // if either end point of the line segment falls within the screen,
  // then the line segment is visible.
  if ((xMinScene <= x0 && xMaxScene >= x0 && yMinScene <= y0 && yMaxScene >= y0) ||
    (xMinScene <= x1 && xMaxScene >= x1 && yMinScene <= y1 && yMaxScene >= y1))
  {
    return true;
  }

  // figure out which end point is "greater" than the other in both dimensions
  double xMinLine, xMaxLine, yMinLine, yMaxLine;
  if (x0 < x1)
  {
    xMinLine = x0;
    xMaxLine = x1;
  }
  else
  {
    xMinLine = x1;
    xMaxLine = x0;
  }
  if (y0 < y1)
  {
    yMinLine = y0;
    yMaxLine = y1;
  }
  else
  {
    yMinLine = y1;
    yMaxLine = y0;
  }

  // case where the Y range of the line falls within the visible scene
  // and the X range of the line contains the entire visible scene
  if (yMinScene <= yMinLine && yMaxScene >= yMinLine && yMinScene <= yMaxLine &&
    yMaxScene >= yMaxLine && xMinLine <= xMinScene && xMaxLine >= xMaxScene)
  {
    return true;
  }

  // case where the X range of the line falls within the visible scene
  // and the Y range of the line contains the entire visible scene
  if (xMinScene <= xMinLine && xMaxScene >= xMinLine && xMinScene <= xMaxLine &&
    xMaxScene >= xMaxLine && yMinLine <= yMinScene && yMaxLine >= yMaxScene)
  {
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool svtkDendrogramItem::MouseDoubleClickEvent(const svtkContextMouseEvent& event)
{
  // get the position of the double click and convert it to scene coordinates
  double pos[3];
  svtkNew<svtkMatrix3x3> inverse;
  pos[0] = event.GetPos().GetX();
  pos[1] = event.GetPos().GetY();
  pos[2] = 0;
  this->GetScene()->GetTransform()->GetInverse(inverse);
  inverse->MultiplyPoint(pos, pos);

  bool rotatedTree = false;
  int orientation = this->GetOrientation();
  if (orientation == svtkDendrogramItem::UP_TO_DOWN || orientation == svtkDendrogramItem::DOWN_TO_UP)
  {
    rotatedTree = true;
  }

  // this event is only captured within the tree (not the vertex labels)
  if ((!rotatedTree && pos[0] <= this->MaxX && pos[0] >= this->MinX) ||
    (rotatedTree && pos[1] <= this->MaxY && pos[1] >= this->MinY))
  {
    svtkIdType collapsedSubTree = this->GetClickedCollapsedSubTree(pos[0], pos[1]);
    if (collapsedSubTree != -1)
    {
      // re-expand the subtree rooted at this vertex
      this->ExpandSubTree(collapsedSubTree);
    }
    else
    {
      // collapse the subtree rooted at this vertex
      svtkIdType closestVertex =
        this->GetClosestVertex((pos[0] - this->Position[0]) / this->MultiplierX,
          (pos[1] - this->Position[1]) / this->MultiplierY);
      this->CollapseSubTree(closestVertex);
    }

    this->Scene->SetDirty(true);
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
svtkIdType svtkDendrogramItem::GetClickedCollapsedSubTree(double x, double y)
{
  // iterate over all the collapsed subtrees to see if this click refers
  // to one of them.
  svtkUnsignedIntArray* vertexIsPruned =
    svtkArrayDownCast<svtkUnsignedIntArray>(this->Tree->GetVertexData()->GetArray("VertexIsPruned"));
  svtkIdTypeArray* originalIdArray =
    svtkArrayDownCast<svtkIdTypeArray>(this->PrunedTree->GetVertexData()->GetArray("OriginalId"));
  int orientation = this->GetOrientation();

  for (svtkIdType originalId = 0; originalId < vertexIsPruned->GetNumberOfTuples(); ++originalId)
  {
    if (vertexIsPruned->GetValue(originalId) > 0)
    {
      // Find PrunedTree's vertex that corresponds to this originalId.
      for (svtkIdType prunedId = 0; prunedId < originalIdArray->GetNumberOfTuples(); ++prunedId)
      {
        if (originalIdArray->GetValue(prunedId) == originalId)
        {
          // determined where this collapsed subtree is rooted.
          double point[3];
          this->LayoutTree->GetPoint(prunedId, point);
          point[0] = point[0] * this->MultiplierX + this->Position[0];
          point[1] = point[1] * this->MultiplierY + this->Position[1];

          // we also need the location of this node's parent
          double parentPoint[3];
          this->LayoutTree->GetPoint(this->LayoutTree->GetParent(prunedId), parentPoint);
          parentPoint[0] = parentPoint[0] * this->MultiplierX + this->Position[0];
          parentPoint[1] = parentPoint[1] * this->MultiplierY + this->Position[1];

          float xMin = 0.0;
          float xMax = 0.0;
          float yMin = 0.0;
          float yMax = 0.0;

          switch (orientation)
          {
            case svtkDendrogramItem::DOWN_TO_UP:
              // proper width (X) range: within +/- LeafSpacing of the vertex's
              // X value.
              xMin = point[0] - this->LeafSpacing / 2;
              xMax = point[0] + this->LeafSpacing / 2;

              // proper height (Y) range: >= parent's Y value
              yMin = parentPoint[1];
              yMax = this->MaxY;
              break;

            case svtkDendrogramItem::RIGHT_TO_LEFT:
              // proper width (X) range: <= parent's X value.
              xMin = this->MinX;
              xMax = parentPoint[0];

              // proper height (Y) range: within +/- LeafSpacing of the vertex's
              // Y value.
              yMin = point[1] - this->LeafSpacing / 2;
              yMax = point[1] + this->LeafSpacing / 2;
              break;

            case svtkDendrogramItem::UP_TO_DOWN:
              // proper width (X) range: within +/- LeafSpacing of the vertex's
              // X value.
              xMin = point[0] - this->LeafSpacing / 2;
              xMax = point[0] + this->LeafSpacing / 2;

              // proper height (Y) range: <= parent's Y value
              yMin = this->MinY;
              yMax = parentPoint[1];
              break;

            case svtkDendrogramItem::LEFT_TO_RIGHT:
            default:
              // proper width (X) range: >= parent's X value.
              xMin = parentPoint[0];
              xMax = this->MaxX;

              // proper height (Y) range: within +/- LeafSpacing of the vertex's
              // Y value.
              yMin = point[1] - this->LeafSpacing / 2;
              yMax = point[1] + this->LeafSpacing / 2;
              break;
          }

          if (x >= xMin && x <= xMax && y >= yMin && y <= yMax)
          {
            return prunedId;
          }

          break;
        }
      }
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
svtkIdType svtkDendrogramItem::GetClosestVertex(double x, double y)
{
  double minDistance = SVTK_DOUBLE_MAX;
  svtkIdType closestVertex = -1;
  for (svtkIdType vertex = 0; vertex < this->LayoutTree->GetNumberOfVertices(); ++vertex)
  {
    if (this->LayoutTree->IsLeaf(vertex))
    {
      continue;
    }
    double point[3];
    this->LayoutTree->GetPoint(vertex, point);
    double distance = sqrt((x - point[0]) * (x - point[0]) + (y - point[1]) * (y - point[1]));

    if (distance < minDistance)
    {
      minDistance = distance;
      closestVertex = vertex;
    }
  }
  return closestVertex;
}

//-----------------------------------------------------------------------------
void svtkDendrogramItem::CollapseSubTree(svtkIdType vertex)
{
  // no removing the root of the tree
  svtkIdType root = this->PrunedTree->GetRoot();
  if (vertex == root)
  {
    return;
  }

  // look up the original ID of the vertex that's being collapsed.
  svtkIdTypeArray* originalIdArray =
    svtkArrayDownCast<svtkIdTypeArray>(this->PrunedTree->GetVertexData()->GetArray("OriginalId"));
  svtkIdType originalId = originalIdArray->GetValue(vertex);

  // use this value as the index to the original (un-reindexed) tree's
  // "VertexIsPruned" array.  Mark that vertex as pruned by recording
  // how many collapsed leaf nodes exist beneath it.
  int numLeavesCollapsed = this->CountLeafNodes(originalId);

  // make sure we're not about to collapse away the whole tree
  int totalLeaves = this->CountLeafNodes(root);
  if (numLeavesCollapsed >= totalLeaves)
  {
    return;
  }

  // no collapsing of leaf nodes.  This should never happen, but it doesn't
  // hurt to be safe.
  if (numLeavesCollapsed == 0)
  {
    return;
  }

  svtkUnsignedIntArray* vertexIsPruned =
    svtkArrayDownCast<svtkUnsignedIntArray>(this->Tree->GetVertexData()->GetArray("VertexIsPruned"));
  vertexIsPruned->SetValue(originalId, numLeavesCollapsed);

  svtkNew<svtkTree> prunedTreeCopy;
  prunedTreeCopy->ShallowCopy(this->PrunedTree);

  this->PruneFilter->SetInputData(prunedTreeCopy);
  this->PruneFilter->SetParentVertex(vertex);
  this->PruneFilter->Update();
  this->PrunedTree = this->PruneFilter->GetOutput();
}

//-----------------------------------------------------------------------------
void svtkDendrogramItem::ExpandSubTree(svtkIdType vertex)
{
  // mark this vertex as "not pruned"
  svtkUnsignedIntArray* vertexIsPruned =
    svtkArrayDownCast<svtkUnsignedIntArray>(this->Tree->GetVertexData()->GetArray("VertexIsPruned"));
  svtkIdType vertexOriginalId = this->GetOriginalId(vertex);
  vertexIsPruned->SetValue(vertexOriginalId, 0);

  // momentarily revert PrunedTree to the full (unpruned) Tree.
  this->PrunedTree->DeepCopy(this->Tree);

  // re-prune as necessary.  this->Tree has the list of originalIds that
  // need to be re-pruned.
  for (svtkIdType originalId = 0; originalId < vertexIsPruned->GetNumberOfTuples(); ++originalId)
  {
    if (vertexIsPruned->GetValue(originalId) > 0)
    {
      // Find PrunedTree's vertex that corresponds to this originalId.
      // Use this to re-collapse the subtrees that were not just expanded.
      svtkIdTypeArray* originalIdArray =
        svtkArrayDownCast<svtkIdTypeArray>(this->PrunedTree->GetVertexData()->GetArray("OriginalId"));
      for (svtkIdType prunedId = 0; prunedId < originalIdArray->GetNumberOfTuples(); ++prunedId)
      {
        if (originalIdArray->GetValue(prunedId) == originalId)
        {
          this->CollapseSubTree(prunedId);
          break;
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
svtkIdType svtkDendrogramItem::GetOriginalId(svtkIdType vertex)
{
  svtkIdTypeArray* originalIdArray =
    svtkArrayDownCast<svtkIdTypeArray>(this->PrunedTree->GetVertexData()->GetArray("OriginalId"));
  return originalIdArray->GetValue(vertex);
}

//-----------------------------------------------------------------------------
svtkIdType svtkDendrogramItem::GetPrunedIdForOriginalId(svtkIdType originalId)
{
  svtkIdTypeArray* originalIdArray =
    svtkArrayDownCast<svtkIdTypeArray>(this->PrunedTree->GetVertexData()->GetArray("OriginalId"));
  for (svtkIdType i = 0; i < originalIdArray->GetNumberOfTuples(); ++i)
  {
    if (originalIdArray->GetValue(i) == originalId)
    {
      return i;
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
void svtkDendrogramItem::CollapseToNumberOfLeafNodes(unsigned int n)
{
  // check that the number requested is actually smaller than the number of
  // leaf nodes in the tree.
  unsigned int numLeaves = this->CountLeafNodes(this->Tree->GetRoot());
  if (n >= numLeaves)
  {
    svtkWarningMacro(<< "n >= total leaf nodes");
    return;
  }

  // reset pruned tree to contain the entire input tree
  this->PrunedTree->DeepCopy(this->Tree);

  // Initialize a priority queue of vertices based on their weight.
  // Vertices with lower weight (closer to the root) have a higher priority.
  std::priority_queue<svtkDendrogramItem::WeightedVertex,
    std::vector<svtkDendrogramItem::WeightedVertex>, svtkDendrogramItem::CompareWeightedVertices>
    queue;
  std::vector<svtkIdType> verticesToCollapse;
  svtkDoubleArray* nodeWeights = svtkArrayDownCast<svtkDoubleArray>(
    this->Tree->GetVertexData()->GetAbstractArray(this->DistanceArrayName));

  // initially, the priority queue contains the children of the root node.
  svtkIdType root = this->Tree->GetRoot();
  for (svtkIdType child = 0; child < this->Tree->GetNumberOfChildren(root); ++child)
  {
    svtkIdType childVertex = this->Tree->GetChild(root, child);

    double weight = 0.0;
    if (nodeWeights != nullptr)
    {
      weight = nodeWeights->GetValue(childVertex);
    }
    else
    {
      weight = static_cast<double>(this->Tree->GetLevel(childVertex));
    }

    svtkDendrogramItem::WeightedVertex v = { childVertex, weight };
    queue.push(v);
  }

  // use the priority queue to find the vertices that we should collapse.
  unsigned int numberOfLeafNodesFound = 0;
  while (queue.size() + numberOfLeafNodesFound < n)
  {
    svtkDendrogramItem::WeightedVertex v = queue.top();
    queue.pop();
    if (this->Tree->GetNumberOfChildren(v.ID) == 0)
    {
      verticesToCollapse.push_back(v.ID);
      ++numberOfLeafNodesFound;
      continue;
    }

    for (svtkIdType child = 0; child < this->Tree->GetNumberOfChildren(v.ID); ++child)
    {
      svtkIdType childVertex = this->Tree->GetChild(v.ID, child);

      double weight = 0.0;
      if (nodeWeights != nullptr)
      {
        weight = nodeWeights->GetValue(childVertex);
      }
      else
      {
        weight = static_cast<double>(this->Tree->GetLevel(childVertex));
      }

      svtkDendrogramItem::WeightedVertex v2 = { childVertex, weight };
      queue.push(v2);
    }
  }

  // collapse the vertices that we found.
  for (unsigned int i = 0; i < verticesToCollapse.size(); ++i)
  {
    svtkIdType prunedId = this->GetPrunedIdForOriginalId(verticesToCollapse[i]);
    if (prunedId == -1)
    {
      svtkErrorMacro("prunedId is -1");
      continue;
    }
    this->CollapseSubTree(prunedId);
  }
  while (!queue.empty())
  {
    svtkDendrogramItem::WeightedVertex v = queue.top();
    queue.pop();
    svtkIdType prunedId = this->GetPrunedIdForOriginalId(v.ID);
    if (prunedId == -1)
    {
      svtkErrorMacro("prunedId is -1");
      continue;
    }
    this->CollapseSubTree(prunedId);
  }
}

//-----------------------------------------------------------------------------
void svtkDendrogramItem::SetColorArray(const char* arrayName)
{
  this->ColorArray =
    svtkArrayDownCast<svtkDoubleArray>(this->Tree->GetVertexData()->GetArray(arrayName));
  if (!this->ColorArray)
  {
    svtkErrorMacro("Could not downcast " << arrayName << " to a svtkDoubleArray");
    this->ColorTree = false;
    return;
  }

  this->ColorTree = true;

  double minValue = SVTK_DOUBLE_MAX;
  double maxValue = SVTK_DOUBLE_MIN;

  for (svtkIdType id = 0; id < this->ColorArray->GetNumberOfTuples(); ++id)
  {
    double d = this->ColorArray->GetValue(id);
    if (d > maxValue)
    {
      maxValue = d;
    }
    if (d < minValue)
    {
      minValue = d;
    }
  }

  // special case: when there is no range of values to display, all edges should
  // be drawn in grey.  Without this, all the edges would be drawn in either red
  // or blue.
  if (minValue == maxValue)
  {
    this->TreeLookupTable->SetNumberOfTableValues(1);
    this->TreeLookupTable->SetTableValue(0, 0.60, 0.60, 0.60);
    // this is done to prevent the legend from being drawn
    this->LegendPositionSet = true;
    return;
  }

  // how much we vary the colors from step to step
  double inc = 0.06;

  // setup the color lookup table.  It will contain 10 shades of red,
  // 10 shades of blue, and a grey neutral value.

  this->TreeLookupTable->SetNumberOfTableValues(21);
  if (fabs(maxValue) > fabs(minValue))
  {
    this->TreeLookupTable->SetRange(-maxValue, maxValue);
  }
  else
  {
    this->TreeLookupTable->SetRange(minValue, -minValue);
  }
  for (svtkIdType i = 0; i < 10; ++i)
  {
    this->TreeLookupTable->SetTableValue(i, 1.0, 0.25 + inc * i, 0.25 + inc * i);
  }
  this->TreeLookupTable->SetTableValue(10, 0.60, 0.60, 0.60);
  for (svtkIdType i = 11; i < 21; ++i)
  {
    this->TreeLookupTable->SetTableValue(i, 0.85 - inc * (i - 10), 0.85 - inc * (i - 10), 1.0);
  }

  // initialize color legend
  this->ColorLegend->SetTransferFunction(this->TreeLookupTable);
  this->ColorLegend->SetTitle(arrayName);
  this->PositionColorLegend();
}

//-----------------------------------------------------------------------------
void svtkDendrogramItem::PositionColorLegend()
{
  // bail out early if we don't have meaningful bounds yet.
  if (this->MinX > this->MaxX || this->MinY > this->MaxY)
  {
    return;
  }

  int orientation = this->GetOrientation();
  switch (orientation)
  {
    case svtkDendrogramItem::DOWN_TO_UP:
    case svtkDendrogramItem::UP_TO_DOWN:
      this->ColorLegend->SetHorizontalAlignment(svtkChartLegend::RIGHT);
      this->ColorLegend->SetVerticalAlignment(svtkChartLegend::CENTER);
      this->ColorLegend->SetOrientation(svtkColorLegend::VERTICAL);
      this->ColorLegend->SetPoint(
        this->MinX - this->LeafSpacing, this->MinY + (this->MaxY - this->MinY) / 2.0);
      this->ColorLegend->SetTextureSize(
        this->ColorLegend->GetSymbolWidth(), this->MaxY - this->MinY);
      break;

    case svtkDendrogramItem::RIGHT_TO_LEFT:
    case svtkDendrogramItem::LEFT_TO_RIGHT:
    default:
      this->ColorLegend->SetHorizontalAlignment(svtkChartLegend::CENTER);
      this->ColorLegend->SetVerticalAlignment(svtkChartLegend::TOP);
      this->ColorLegend->SetOrientation(svtkColorLegend::HORIZONTAL);
      this->ColorLegend->SetPoint(
        this->MinX + (this->MaxX - this->MinX) / 2.0, this->MinY - this->LeafSpacing);
      this->ColorLegend->SetTextureSize(
        this->MaxX - this->MinX, this->ColorLegend->GetSymbolWidth());
      break;
  }
  this->ColorLegend->Update();
  this->ColorLegend->SetVisible(true);
  this->Scene->SetDirty(true);
  this->LegendPositionSet = true;
}

//-----------------------------------------------------------------------------
void svtkDendrogramItem::SetOrientation(int orientation)
{
  this->SetOrientation(this->Tree, orientation);
}

//-----------------------------------------------------------------------------
void svtkDendrogramItem::SetOrientation(svtkTree* tree, int orientation)
{
  svtkIntArray* existingArray =
    svtkArrayDownCast<svtkIntArray>(tree->GetFieldData()->GetArray("orientation"));
  if (existingArray)
  {
    existingArray->SetValue(0, orientation);
  }
  else
  {
    svtkSmartPointer<svtkIntArray> orientationArray = svtkSmartPointer<svtkIntArray>::New();
    orientationArray->SetNumberOfComponents(1);
    orientationArray->SetName("orientation");
    orientationArray->InsertNextValue(orientation);
    tree->GetFieldData()->AddArray(orientationArray);
  }

  if (tree == this->Tree)
  {
    this->SetOrientation(this->PrunedTree, orientation);
    this->SetOrientation(this->LayoutTree, orientation);
  }
}

//-----------------------------------------------------------------------------
int svtkDendrogramItem::GetOrientation()
{
  svtkIntArray* orientationArray =
    svtkArrayDownCast<svtkIntArray>(this->Tree->GetFieldData()->GetArray("orientation"));
  if (orientationArray)
  {
    return orientationArray->GetValue(0);
  }
  return svtkDendrogramItem::LEFT_TO_RIGHT;
}

//-----------------------------------------------------------------------------
double svtkDendrogramItem::GetAngleForOrientation(int orientation)
{
  switch (orientation)
  {
    case svtkDendrogramItem::DOWN_TO_UP:
      return 180.0;

    case svtkDendrogramItem::RIGHT_TO_LEFT:
      return 270.0;

    case svtkDendrogramItem::UP_TO_DOWN:
      return 0.0;

    case svtkDendrogramItem::LEFT_TO_RIGHT:
    default:
      return 90.0;
  }
}

//-----------------------------------------------------------------------------
double svtkDendrogramItem::GetTextAngleForOrientation(int orientation)
{
  switch (orientation)
  {
    case svtkDendrogramItem::DOWN_TO_UP:
      return 90.0;

    case svtkDendrogramItem::RIGHT_TO_LEFT:
      return 0.0;

    case svtkDendrogramItem::UP_TO_DOWN:
      return 270.0;

    case svtkDendrogramItem::LEFT_TO_RIGHT:
    default:
      return 0.0;
  }
}

//-----------------------------------------------------------------------------
void svtkDendrogramItem::GetBounds(double bounds[4])
{
  bounds[0] = this->MinX;
  bounds[1] = this->MaxX;
  bounds[2] = this->MinY;
  bounds[3] = this->MaxY;

  if (this->LabelWidth == 0.0)
  {
    return;
  }

  double spacing = this->LeafSpacing * 0.5;

  switch (this->GetOrientation())
  {
    case svtkDendrogramItem::LEFT_TO_RIGHT:
    default:
      bounds[1] += spacing + this->LabelWidth;
      break;

    case svtkDendrogramItem::UP_TO_DOWN:
      bounds[2] -= spacing + this->LabelWidth;
      break;

    case svtkDendrogramItem::RIGHT_TO_LEFT:
      bounds[0] -= spacing + this->LabelWidth;
      break;

    case svtkDendrogramItem::DOWN_TO_UP:
      bounds[3] += spacing + this->LabelWidth;
      break;
  }
}

//-----------------------------------------------------------------------------
float svtkDendrogramItem::GetLabelWidth()
{
  return this->LabelWidth;
}

//-----------------------------------------------------------------------------
void svtkDendrogramItem::ComputeLabelWidth(svtkContext2D* painter)
{
  this->LabelWidth = 0.0;
  if (!this->DrawLabels)
  {
    return;
  }
  int fontSize = painter->ComputeFontSizeForBoundedString("Igq", SVTK_FLOAT_MAX, this->LeafSpacing);
  if (fontSize < 8)
  {
    return;
  }

  // temporarily set text to default orientation
  double orientation = painter->GetTextProp()->GetOrientation();
  painter->GetTextProp()->SetOrientation(0.0);

  // get array of node names from the tree
  svtkStringArray* vertexNames = svtkArrayDownCast<svtkStringArray>(
    this->LayoutTree->GetVertexData()->GetAbstractArray(this->VertexNameArrayName));

  float bounds[4];
  for (svtkIdType i = 0; i < vertexNames->GetNumberOfTuples(); ++i)
  {
    painter->ComputeStringBounds(vertexNames->GetValue(i), bounds);
    if (bounds[2] > this->LabelWidth)
    {
      this->LabelWidth = bounds[2];
    }
  }

  // restore orientation
  painter->GetTextProp()->SetOrientation(orientation);
}

//-----------------------------------------------------------------------------
bool svtkDendrogramItem::GetPositionOfVertex(const std::string& vertexName, double position[2])
{
  svtkStringArray* vertexNames = svtkArrayDownCast<svtkStringArray>(
    this->LayoutTree->GetVertexData()->GetAbstractArray(this->VertexNameArrayName));

  svtkIdType vertex = vertexNames->LookupValue(vertexName);
  if (vertex == -1)
  {
    return false;
  }

  double point[3];
  this->LayoutTree->GetPoint(vertex, point);

  position[0] = this->Position[0] + point[0] * this->MultiplierX;
  position[1] = this->Position[1] + point[1] * this->MultiplierY;

  return true;
}

//-----------------------------------------------------------------------------
bool svtkDendrogramItem::Hit(const svtkContextMouseEvent& svtkNotUsed(mouse))
{
  // If we are interactive, we want to catch anything that propagates to the
  // background, otherwise we do not want any mouse events.
  return this->Interactive;
}

//-----------------------------------------------------------------------------
void svtkDendrogramItem::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Tree: " << (this->Tree ? "" : "(null)") << std::endl;
  if (this->Tree->GetNumberOfVertices() > 0)
  {
    this->Tree->PrintSelf(os, indent.GetNextIndent());
  }
}
