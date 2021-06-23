/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDijkstraImageContourLineInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDijkstraImageContourLineInterpolator.h"

#include "svtkCellArray.h"
#include "svtkContourRepresentation.h"
#include "svtkDijkstraImageGeodesicPath.h"
#include "svtkImageActor.h"
#include "svtkImageActorPointPlacer.h"
#include "svtkImageData.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkDijkstraImageContourLineInterpolator);

//----------------------------------------------------------------------
svtkDijkstraImageContourLineInterpolator ::svtkDijkstraImageContourLineInterpolator()
{
  this->DijkstraImageGeodesicPath = svtkDijkstraImageGeodesicPath::New();
  this->CostImage = nullptr;
}

//----------------------------------------------------------------------
svtkDijkstraImageContourLineInterpolator ::~svtkDijkstraImageContourLineInterpolator()
{
  this->DijkstraImageGeodesicPath->Delete();
  this->CostImage = nullptr;
}

//----------------------------------------------------------------------------
void svtkDijkstraImageContourLineInterpolator::SetCostImage(svtkImageData* arg)
{
  if (this->CostImage == arg)
  {
    return;
  }

  this->CostImage = arg;
  if (this->CostImage)
  {
    this->DijkstraImageGeodesicPath->SetInputData(this->CostImage);
  }
}

//----------------------------------------------------------------------
int svtkDijkstraImageContourLineInterpolator::InterpolateLine(
  svtkRenderer* svtkNotUsed(ren), svtkContourRepresentation* rep, int idx1, int idx2)
{
  // if the user didn't set the image, try to get it from the actor
  if (!this->CostImage)
  {

    svtkImageActorPointPlacer* placer =
      svtkImageActorPointPlacer::SafeDownCast(rep->GetPointPlacer());

    if (!placer)
    {
      return 1;
    }

    svtkImageActor* actor = placer->GetImageActor();
    if (!actor || !(this->CostImage = actor->GetInput()))
    {
      return 1;
    }
    this->DijkstraImageGeodesicPath->SetInputData(this->CostImage);
  }

  double p1[3], p2[3];
  rep->GetNthNodeWorldPosition(idx1, p1);
  rep->GetNthNodeWorldPosition(idx2, p2);

  svtkIdType beginVertId = this->CostImage->FindPoint(p1);
  svtkIdType endVertId = this->CostImage->FindPoint(p2);

  // Could not find the starting and ending cells. We can't interpolate.
  if (beginVertId == -1 || endVertId == -1)
  {
    return 0;
  }

  int nnodes = rep->GetNumberOfNodes();

  if (this->DijkstraImageGeodesicPath->GetRepelPathFromVertices() && nnodes > 2)
  {
    svtkPoints* verts = svtkPoints::New();
    double pt[3];
    for (int i = 0; i < nnodes; ++i)
    {
      if (i == idx1)
        continue;

      for (int j = 0; j < rep->GetNumberOfIntermediatePoints(i); ++j)
      {
        rep->GetIntermediatePointWorldPosition(i, j, pt);
        verts->InsertNextPoint(pt);
      }
    }
    this->DijkstraImageGeodesicPath->SetRepelVertices(verts);
    verts->Delete();
  }
  else
  {
    this->DijkstraImageGeodesicPath->SetRepelVertices(nullptr);
  }

  this->DijkstraImageGeodesicPath->SetStartVertex(endVertId);
  this->DijkstraImageGeodesicPath->SetEndVertex(beginVertId);
  this->DijkstraImageGeodesicPath->Update();

  svtkPolyData* pd = this->DijkstraImageGeodesicPath->GetOutput();

  svtkIdType npts = 0;
  const svtkIdType* pts = nullptr;
  pd->GetLines()->InitTraversal();
  pd->GetLines()->GetNextCell(npts, pts);

  for (int i = 0; i < npts; ++i)
  {
    rep->AddIntermediatePointWorldPosition(idx1, pd->GetPoint(pts[i]));
  }

  return 1;
}

//----------------------------------------------------------------------
void svtkDijkstraImageContourLineInterpolator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DijkstraImageGeodesicPath: " << this->DijkstraImageGeodesicPath << endl;
  os << indent << "CostImage: " << this->GetCostImage() << endl;
}
