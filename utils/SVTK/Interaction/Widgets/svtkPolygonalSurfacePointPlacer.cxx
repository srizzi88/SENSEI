/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolygonalSurfacePointPlacer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPolygonalSurfacePointPlacer.h"

#include "svtkAssemblyNode.h"
#include "svtkAssemblyPath.h"
#include "svtkCellData.h"
#include "svtkCellPicker.h"
#include "svtkDataArray.h"
#include "svtkIdList.h"
#include "svtkInteractorObserver.h"
#include "svtkMapper.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkPolyDataCollection.h"
#include "svtkProp.h"
#include "svtkPropCollection.h"
#include "svtkRenderer.h"

#include <vector>

class svtkPolygonalSurfacePointPlacerInternals
{

public:
  typedef std::vector<svtkPolygonalSurfacePointPlacerNode*> NodesContainerType;

  NodesContainerType Nodes;

  ~svtkPolygonalSurfacePointPlacerInternals()
  {
    for (unsigned int i = 0; i < this->Nodes.size(); i++)
    {
      delete this->Nodes[i];
    }
    this->Nodes.clear();
  }

  svtkPolygonalSurfacePointPlacerNode* GetNodeAtSurfaceWorldPosition(double worldPos[3])
  {
    const double tolerance = 0.0005;
    for (unsigned int i = 0; i < this->Nodes.size(); i++)
    {
      if (svtkMath::Distance2BetweenPoints(this->Nodes[i]->SurfaceWorldPosition, worldPos) <
        tolerance)
      {
        return this->Nodes[i];
      }
    }
    return nullptr;
  }

  svtkPolygonalSurfacePointPlacerNode* GetNodeAtWorldPosition(double worldPos[3])
  {
    const double tolerance = 0.0005;
    for (unsigned int i = 0; i < this->Nodes.size(); i++)
    {
      if (svtkMath::Distance2BetweenPoints(this->Nodes[i]->WorldPosition, worldPos) < tolerance)
      {
        return this->Nodes[i];
      }
    }
    return nullptr;
  }

  svtkPolygonalSurfacePointPlacerNode* InsertNodeAtCurrentPickPosition(
    svtkCellPicker* picker, const double distanceOffset, int snapToClosestPoint)
  {
    double worldPos[3];
    picker->GetPickPosition(worldPos);

    // Get a node at this position if one exists and overwrite it
    // with the current pick position. If one doesn't exist, add
    // a new node.
    svtkPolygonalSurfacePointPlacerNode* node = this->GetNodeAtSurfaceWorldPosition(worldPos);
    if (!node)
    {
      node = new svtkPolygonalSurfacePointPlacerNode;
      this->Nodes.push_back(node);
    }

    svtkMapper* mapper = svtkMapper::SafeDownCast(picker->GetMapper());
    if (!mapper)
    {
      return nullptr;
    }

    // Get the underlying dataset
    svtkPolyData* pd = svtkPolyData::SafeDownCast(mapper->GetInput());
    if (!pd)
    {
      return nullptr;
    }

    node->CellId = picker->GetCellId();
    picker->GetPCoords(node->ParametricCoords);

    // translate to the closest point on that cell, if requested

    if (snapToClosestPoint)
    {
      svtkIdList* ids = svtkIdList::New();
      pd->GetCellPoints(picker->GetCellId(), ids);
      double p[3], minDistance = SVTK_DOUBLE_MAX;
      for (svtkIdType i = 0; i < ids->GetNumberOfIds(); ++i)
      {
        pd->GetPoints()->GetPoint(ids->GetId(i), p);
        const double dist2 =
          svtkMath::Distance2BetweenPoints(worldPos, pd->GetPoints()->GetPoint(ids->GetId(i)));
        if (dist2 < minDistance)
        {
          minDistance = dist2;
          worldPos[0] = p[0];
          worldPos[1] = p[1];
          worldPos[2] = p[2];
        }
      }
      ids->Delete();
    }

    node->SurfaceWorldPosition[0] = worldPos[0];
    node->SurfaceWorldPosition[1] = worldPos[1];
    node->SurfaceWorldPosition[2] = worldPos[2];
    node->PolyData = pd;
    double cellNormal[3];

    if (distanceOffset != 0.0)
    {
      pd->GetCellData()->GetNormals()->GetTuple(node->CellId, cellNormal);

      // Polyline can be drawn on polydata at a height offset.
      for (unsigned int i = 0; i < 3; i++)
      {
        node->WorldPosition[i] = node->SurfaceWorldPosition[i] + cellNormal[i] * distanceOffset;
      }
    }
    else
    {
      for (unsigned int i = 0; i < 3; i++)
      {
        node->WorldPosition[i] = node->SurfaceWorldPosition[i];
      }
    }
    return node;
  }

  svtkPolygonalSurfacePointPlacerNode* InsertNodeAtCurrentPickPosition(svtkPolyData* pd,
    double worldPos[3], svtkIdType cellId, svtkIdType pointId,
    const double svtkNotUsed(distanceOffset), int svtkNotUsed(snapToClosestPoint))
  {

    // Get a node at this position if one exists and overwrite it
    // with the current pick position. If one doesn't exist, add
    // a new node.
    svtkPolygonalSurfacePointPlacerNode* node = this->GetNodeAtSurfaceWorldPosition(worldPos);
    if (!node)
    {
      node = new svtkPolygonalSurfacePointPlacerNode;
      this->Nodes.push_back(node);
    }

    node->CellId = cellId;
    node->PointId = pointId;

    node->SurfaceWorldPosition[0] = worldPos[0];
    node->SurfaceWorldPosition[1] = worldPos[1];
    node->SurfaceWorldPosition[2] = worldPos[2];
    node->PolyData = pd;

    for (unsigned int i = 0; i < 3; i++)
    {
      node->WorldPosition[i] = node->SurfaceWorldPosition[i];
    }
    return node;
  }
  // ashish
};

svtkStandardNewMacro(svtkPolygonalSurfacePointPlacer);

//----------------------------------------------------------------------
svtkPolygonalSurfacePointPlacer::svtkPolygonalSurfacePointPlacer()
{
  this->Polys = svtkPolyDataCollection::New();
  this->CellPicker = svtkCellPicker::New();
  this->CellPicker->PickFromListOn();
  this->CellPicker->SetTolerance(0.005); // need some fluff

  this->Internals = new svtkPolygonalSurfacePointPlacerInternals;
  this->DistanceOffset = 0.0;
  this->SnapToClosestPoint = 0;
}

//----------------------------------------------------------------------
svtkPolygonalSurfacePointPlacer::~svtkPolygonalSurfacePointPlacer()
{
  this->CellPicker->Delete();
  this->Polys->Delete();
  delete this->Internals;
}

//----------------------------------------------------------------------
void svtkPolygonalSurfacePointPlacer::AddProp(svtkProp* prop)
{
  this->SurfaceProps->AddItem(prop);
  this->CellPicker->AddPickList(prop);
}

//----------------------------------------------------------------------
void svtkPolygonalSurfacePointPlacer::RemoveViewProp(svtkProp* prop)
{
  this->Superclass::RemoveViewProp(prop);
  this->CellPicker->DeletePickList(prop);
}

//----------------------------------------------------------------------
void svtkPolygonalSurfacePointPlacer::RemoveAllProps()
{
  this->Superclass::RemoveAllProps();
  this->CellPicker->InitializePickList();
}

//----------------------------------------------------------------------
int svtkPolygonalSurfacePointPlacer::ComputeWorldPosition(svtkRenderer* ren, double displayPos[2],
  double* svtkNotUsed(refWorldPos), double worldPos[3], double worldOrient[9])
{
  return this->ComputeWorldPosition(ren, displayPos, worldPos, worldOrient);
}

//----------------------------------------------------------------------
int svtkPolygonalSurfacePointPlacer::ComputeWorldPosition(
  svtkRenderer* ren, double displayPos[2], double worldPos[3], double svtkNotUsed(worldOrient)[9])
{
  if (this->CellPicker->Pick(displayPos[0], displayPos[1], 0.0, ren))
  {

    svtkMapper* mapper = svtkMapper::SafeDownCast(this->CellPicker->GetMapper());
    if (!mapper)
    {
      return 0;
    }

    // Get the underlying dataset
    svtkPolyData* pd = svtkPolyData::SafeDownCast(mapper->GetInput());
    if (!pd)
    {
      return 0;
    }

    if (svtkAssemblyPath* path = this->CellPicker->GetPath())
    {

      // We are checking if the prop present in the path is present
      // in the list supplied to us.. If it is, that prop will be picked.
      // If not, no prop will be picked.

      bool found = false;
      svtkAssemblyNode* node = nullptr;
      svtkCollectionSimpleIterator sit;
      this->SurfaceProps->InitTraversal(sit);

      while (svtkProp* p = this->SurfaceProps->GetNextProp(sit))
      {
        svtkCollectionSimpleIterator psit;
        path->InitTraversal(psit);

        for (int i = 0; i < path->GetNumberOfItems() && !found; ++i)
        {
          node = path->GetNextNode(psit);
          found = (node->GetViewProp() == p);
        }

        if (found)
        {
          svtkPolygonalSurfacePointPlacer::Node* contourNode =
            this->Internals->InsertNodeAtCurrentPickPosition(
              this->CellPicker, this->DistanceOffset, this->SnapToClosestPoint);
          if (contourNode)
          {
            worldPos[0] = contourNode->WorldPosition[0];
            worldPos[1] = contourNode->WorldPosition[1];
            worldPos[2] = contourNode->WorldPosition[2];
            return 1;
          }
        }
      }
    }
  }

  return 0;
}

//----------------------------------------------------------------------
int svtkPolygonalSurfacePointPlacer::ValidateWorldPosition(
  double worldPos[3], double* svtkNotUsed(worldOrient))
{
  return this->ValidateWorldPosition(worldPos);
}

//----------------------------------------------------------------------
int svtkPolygonalSurfacePointPlacer::ValidateWorldPosition(double svtkNotUsed(worldPos)[3])
{
  return 1;
}

//----------------------------------------------------------------------
int svtkPolygonalSurfacePointPlacer::ValidateDisplayPosition(
  svtkRenderer*, double svtkNotUsed(displayPos)[2])
{
  // We could check here to ensure that the display point picks one of the
  // terrain props, but the contour representation always calls
  // ComputeWorldPosition followed by
  // ValidateDisplayPosition/ValidateWorldPosition when it needs to
  // update a node...
  //
  // So that would be wasting CPU cycles to perform
  // the same check twice..  Just return 1 here.

  return 1;
}

//----------------------------------------------------------------------
svtkPolygonalSurfacePointPlacer::Node* svtkPolygonalSurfacePointPlacer ::GetNodeAtWorldPosition(
  double worldPos[3])
{
  return this->Internals->GetNodeAtWorldPosition(worldPos);
}

//----------------------------------------------------------------------
int svtkPolygonalSurfacePointPlacer::UpdateNodeWorldPosition(
  double worldPos[3], svtkIdType nodePointId)
{
  if (this->Polys->GetNumberOfItems() != 0)
  {
    svtkPolyData* pd = svtkPolyData::SafeDownCast(this->Polys->GetItemAsObject(0));
    this->Internals->InsertNodeAtCurrentPickPosition(
      pd, worldPos, -1, nodePointId, this->DistanceOffset, this->SnapToClosestPoint);
    return 1;
  }
  else
  {
    svtkErrorMacro("PolyDataCollection has no items.");
    return 0;
  }
}

//----------------------------------------------------------------------
void svtkPolygonalSurfacePointPlacer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Cell Picker: " << this->CellPicker << endl;
  if (this->CellPicker)
  {
    this->CellPicker->PrintSelf(os, indent.GetNextIndent());
  }

  os << indent << "Surface Props: " << this->SurfaceProps << endl;
  if (this->SurfaceProps)
  {
    this->SurfaceProps->PrintSelf(os, indent.GetNextIndent());
  }

  os << indent << "Surface polygons: " << this->Polys << endl;
  if (this->Polys)
  {
    this->Polys->PrintSelf(os, indent.GetNextIndent());
  }

  os << indent << "Distance Offset: " << this->DistanceOffset << "\n";
  os << indent << "SnapToClosestPoint: " << this->SnapToClosestPoint << endl;
}
