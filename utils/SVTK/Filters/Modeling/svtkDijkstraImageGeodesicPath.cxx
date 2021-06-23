/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDijkstraImageGeodesicPath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDijkstraImageGeodesicPath.h"

#include "svtkCellArray.h"
#include "svtkExecutive.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"

#include "svtkDijkstraGraphInternals.h"

svtkStandardNewMacro(svtkDijkstraImageGeodesicPath);

//----------------------------------------------------------------------------
svtkDijkstraImageGeodesicPath::svtkDijkstraImageGeodesicPath()
{
  this->PixelSize = 1.0;
  this->ImageWeight = 1.0;
  this->EdgeLengthWeight = 0.0;
  this->CurvatureWeight = 0.0;
  this->RebuildStaticCosts = false;
}

//----------------------------------------------------------------------------
svtkDijkstraImageGeodesicPath::~svtkDijkstraImageGeodesicPath() = default;

//----------------------------------------------------------------------------
void svtkDijkstraImageGeodesicPath::SetImageWeight(double w)
{
  w = w < 0.0 ? 0.0 : (w > 1.0 ? 1.0 : w);
  if (w != this->ImageWeight)
  {
    this->ImageWeight = w;
    this->RebuildStaticCosts = true;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkDijkstraImageGeodesicPath::SetEdgeLengthWeight(double w)
{
  w = w < 0.0 ? 0.0 : (w > 1.0 ? 1.0 : w);
  if (w != this->EdgeLengthWeight)
  {
    this->EdgeLengthWeight = w;
    this->RebuildStaticCosts = true;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkDijkstraImageGeodesicPath::SetInputData(svtkDataObject* input)
{
  svtkImageData* image = svtkImageData::SafeDownCast(input);
  if (!image)
  {
    return;
  }

  int* dimensions = image->GetDimensions();
  int u[3];
  int n = 0;
  for (int i = 0; i < 3; ++i)
  {
    if (dimensions[i] != 1)
    {
      u[n++] = i;
    }
  }
  if (n != 2)
  {
    svtkErrorMacro(<< "Input cost image must be 2D: input dimensions " << dimensions[0] << ","
                  << dimensions[1] << "," << dimensions[2]);
    return;
  }

  double* spacing = image->GetSpacing();
  this->PixelSize = sqrt(spacing[u[0]] * spacing[u[0]] + spacing[u[1]] * spacing[u[1]]);
  this->Superclass::SetInputData(image);
}

//----------------------------------------------------------------------------
svtkImageData* svtkDijkstraImageGeodesicPath::GetInputAsImageData()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  return svtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

//----------------------------------------------------------------------------
int svtkDijkstraImageGeodesicPath::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
int svtkDijkstraImageGeodesicPath::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* costInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkImageData* image = svtkImageData::SafeDownCast(costInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (!image)
  {
    return 0;
  }

  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (!output)
  {
    return 0;
  }

  if (this->AdjacencyBuildTime.GetMTime() < image->GetMTime())
  {
    this->Initialize(image);
  }
  else
  {
    // if the filter's static cost weights change, then update them
    if (this->RebuildStaticCosts)
    {
      this->UpdateStaticCosts(image);
    }
    this->Reset();
  }

  this->ShortestPath(image, this->StartVertex, this->EndVertex);
  this->TraceShortestPath(image, output, this->StartVertex, this->EndVertex);

  return 1;
}

//----------------------------------------------------------------------------
double svtkDijkstraImageGeodesicPath::CalculateStaticEdgeCost(
  svtkDataSet* inData, svtkIdType u, svtkIdType v)
{
  svtkImageData* image = svtkImageData::SafeDownCast(inData);

  double p1[3];
  image->GetPoint(u, p1);
  double p2[3];
  image->GetPoint(v, p2);

  double pcoords[3];
  int ijk1[3];
  int ijk2[3];
  image->ComputeStructuredCoordinates(p1, ijk1, pcoords);
  image->ComputeStructuredCoordinates(p2, ijk2, pcoords);

  double cost = this->ImageWeight *
    (image->GetScalarComponentAsDouble(ijk1[0], ijk1[1], ijk1[2], 0) +
      image->GetScalarComponentAsDouble(ijk1[0], ijk1[1], ijk1[2], 0));

  if (this->EdgeLengthWeight != 0.0)
  {
    cost +=
      this->EdgeLengthWeight * (sqrt(svtkMath::Distance2BetweenPoints(p1, p2)) / this->PixelSize);
  }

  return cost;
}

//----------------------------------------------------------------------------
double svtkDijkstraImageGeodesicPath::CalculateDynamicEdgeCost(
  svtkDataSet* inData, svtkIdType u, svtkIdType v)
{
  double cost = 0.0;

  if (this->CurvatureWeight != 0.0)
  {
    int t = this->Internals->Predecessors[u];
    if (t != -1)
    {
      svtkImageData* image = svtkImageData::SafeDownCast(inData);

      double p0[3];
      image->GetPoint(t, p0);
      double p1[3];
      image->GetPoint(u, p1);
      double p2[3];
      image->GetPoint(v, p2);

      double p10[3] = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] };
      double p21[3] = { p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2] };

      svtkMath::Normalize(p10);
      svtkMath::Normalize(p21);

      // the range of dot product of two unit vectors is [-1, 1] so normalize
      // the maximum curvature from 2 to 1
      cost = this->CurvatureWeight * (0.5 * fabs(svtkMath::Dot(p10, p21) - 1.0));
    }
  }

  return cost;
}

//----------------------------------------------------------------------------
// This is probably a horribly inefficient way to do it.
void svtkDijkstraImageGeodesicPath::BuildAdjacency(svtkDataSet* inData)
{
  svtkImageData* image = svtkImageData::SafeDownCast(inData);

  svtkIdType ncells = image->GetNumberOfCells();

  // optimized for cell type SVTK_PIXEL
  //
  svtkIdList* ptIds = svtkIdList::New();
  svtkIdType uId[6] = { 0, 1, 2, 3, 0, 1 };
  svtkIdType vId[6] = { 1, 2, 3, 0, 2, 3 };
  double cost;

  for (svtkIdType i = 0; i < ncells; ++i)
  {
    image->GetCellPoints(i, ptIds);

    for (int j = 0; j < 6; ++j)
    {
      svtkIdType u = ptIds->GetId(vId[j]);
      svtkIdType v = ptIds->GetId(uId[j]);

      // before insert and calc, check if key map u has key v
      std::map<int, double>& mu = this->Internals->Adjacency[u];
      if (mu.find(v) == mu.end())
      {
        cost = this->CalculateStaticEdgeCost(image, u, v);
        mu.insert(std::pair<int, double>(v, cost));
      }

      std::map<int, double>& mv = this->Internals->Adjacency[v];
      if (mv.find(u) == mv.end())
      {
        cost = this->CalculateStaticEdgeCost(image, v, u);
        mv.insert(std::pair<int, double>(u, cost));
      }
    }
  }

  ptIds->Delete();

  this->RebuildStaticCosts = false;
  this->AdjacencyBuildTime.Modified();
}

//----------------------------------------------------------------------------
void svtkDijkstraImageGeodesicPath::UpdateStaticCosts(svtkImageData* image)
{
  for (int u = 0; u < static_cast<int>(this->Internals->Adjacency.size()); ++u)
  {
    std::map<int, double>& mu = this->Internals->Adjacency[u];
    std::map<int, double>::iterator it = mu.begin();
    for (; it != mu.end(); ++it)
    {
      int v = (*it).first;
      (*it).second = this->CalculateStaticEdgeCost(image, u, v);
    }
  }
  this->RebuildStaticCosts = false;
}

//----------------------------------------------------------------------------
void svtkDijkstraImageGeodesicPath::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ImageWeight: " << this->ImageWeight << endl;
  os << indent << "EdgeLengthWeight: " << this->EdgeLengthWeight << endl;
  os << indent << "CurvatureWeight: " << this->CurvatureWeight << endl;
}
