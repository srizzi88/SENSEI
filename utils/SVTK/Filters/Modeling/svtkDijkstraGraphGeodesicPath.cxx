/*=========================================================================

  Program:   Visualization Toolkit
  Module:  svtkDijkstraGraphGeodesicPath.cxx
  Language:  C++

  Made by Rasmus Paulsen
  email:  rrp(at)imm.dtu.dk
  web:    www.imm.dtu.dk/~rrp/SVTK

  This class is not mature enough to enter the official SVTK release.
=========================================================================*/
#include "svtkDijkstraGraphGeodesicPath.h"

#include "svtkCellArray.h"
#include "svtkDijkstraGraphInternals.h"
#include "svtkDoubleArray.h"
#include "svtkExecutive.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkDijkstraGraphGeodesicPath);
svtkCxxSetObjectMacro(svtkDijkstraGraphGeodesicPath, RepelVertices, svtkPoints);

//----------------------------------------------------------------------------
svtkDijkstraGraphGeodesicPath::svtkDijkstraGraphGeodesicPath()
{
  this->IdList = svtkIdList::New();
  this->Internals = new svtkDijkstraGraphInternals;
  this->StopWhenEndReached = 0;
  this->UseScalarWeights = 0;
  this->NumberOfVertices = 0;
  this->RepelPathFromVertices = 0;
  this->RepelVertices = nullptr;
}

//----------------------------------------------------------------------------
svtkDijkstraGraphGeodesicPath::~svtkDijkstraGraphGeodesicPath()
{
  if (this->IdList)
  {
    this->IdList->Delete();
  }
  delete this->Internals;
  this->SetRepelVertices(nullptr);
}

//----------------------------------------------------------------------------
void svtkDijkstraGraphGeodesicPath::GetCumulativeWeights(svtkDoubleArray* weights)
{
  if (!weights)
  {
    return;
  }

  weights->Initialize();
  double* weightsArray = new double[this->Internals->CumulativeWeights.size()];
  std::copy(this->Internals->CumulativeWeights.begin(), this->Internals->CumulativeWeights.end(),
    weightsArray);
  weights->SetArray(
    weightsArray, static_cast<svtkIdType>(this->Internals->CumulativeWeights.size()), 0);
}

//----------------------------------------------------------------------------
int svtkDijkstraGraphGeodesicPath::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!input)
  {
    return 0;
  }

  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    return 0;
  }

  if (this->AdjacencyBuildTime.GetMTime() < input->GetMTime())
  {
    this->Initialize(input);
  }
  else
  {
    this->Reset();
  }

  if (this->NumberOfVertices == 0)
  {
    return 0;
  }

  this->ShortestPath(input, this->StartVertex, this->EndVertex);
  this->TraceShortestPath(input, output, this->StartVertex, this->EndVertex);
  return 1;
}

//----------------------------------------------------------------------------
void svtkDijkstraGraphGeodesicPath::Initialize(svtkDataSet* inData)
{
  this->NumberOfVertices = inData->GetNumberOfPoints();

  this->Internals->CumulativeWeights.resize(this->NumberOfVertices);
  this->Internals->Predecessors.resize(this->NumberOfVertices);
  this->Internals->OpenVertices.resize(this->NumberOfVertices);
  this->Internals->ClosedVertices.resize(this->NumberOfVertices);
  this->Internals->Adjacency.clear();
  this->Internals->Adjacency.resize(this->NumberOfVertices);
  this->Internals->BlockedVertices.resize(this->NumberOfVertices);

  // The heap has elements from 1 to n
  this->Internals->InitializeHeap(this->NumberOfVertices);

  this->Reset();
  this->BuildAdjacency(inData);
}

//----------------------------------------------------------------------------
void svtkDijkstraGraphGeodesicPath::Reset()
{
  std::fill(
    this->Internals->CumulativeWeights.begin(), this->Internals->CumulativeWeights.end(), -1.0);
  std::fill(this->Internals->Predecessors.begin(), this->Internals->Predecessors.end(), -1);
  std::fill(this->Internals->OpenVertices.begin(), this->Internals->OpenVertices.end(), false);
  std::fill(this->Internals->ClosedVertices.begin(), this->Internals->ClosedVertices.end(), false);
  if (this->RepelPathFromVertices)
  {
    std::fill(
      this->Internals->BlockedVertices.begin(), this->Internals->BlockedVertices.end(), false);
  }

  this->IdList->Reset();
  this->Internals->ResetHeap();
}

//----------------------------------------------------------------------------
double svtkDijkstraGraphGeodesicPath::CalculateStaticEdgeCost(
  svtkDataSet* inData, svtkIdType u, svtkIdType v)
{
  double p1[3];
  inData->GetPoint(u, p1);
  double p2[3];
  inData->GetPoint(v, p2);

  double w = sqrt(svtkMath::Distance2BetweenPoints(p1, p2));

  if (this->UseScalarWeights)
  {
    double s2 = 0.0;
    // Note this edge cost is not symmetric!
    if (inData->GetPointData())
    {
      svtkFloatArray* scalars = svtkFloatArray::SafeDownCast(inData->GetPointData()->GetScalars());
      if (scalars)
      {
        s2 = static_cast<double>(scalars->GetValue(v));
      }
    }

    double wt = s2 * s2;
    if (wt != 0.0)
    {
      w /= wt;
    }
  }
  return w;
}

//----------------------------------------------------------------------------
// This is probably a horribly inefficient way to do it.
void svtkDijkstraGraphGeodesicPath::BuildAdjacency(svtkDataSet* inData)
{
  svtkPolyData* pd = svtkPolyData::SafeDownCast(inData);
  svtkIdType ncells = pd->GetNumberOfCells();

  for (svtkIdType i = 0; i < ncells; i++)
  {
    // Possible types
    //    SVTK_VERTEX, SVTK_POLY_VERTEX, SVTK_LINE,
    //    SVTK_POLY_LINE,SVTK_TRIANGLE, SVTK_QUAD,
    //    SVTK_POLYGON, or SVTK_TRIANGLE_STRIP.

    svtkIdType ctype = pd->GetCellType(i);

    // Until now only handle polys and triangles
    // TODO: All types
    if (ctype == SVTK_POLYGON || ctype == SVTK_TRIANGLE || ctype == SVTK_LINE)
    {
      const svtkIdType* pts;
      svtkIdType npts;
      pd->GetCellPoints(i, npts, pts);
      double cost;

      for (int j = 0; j < npts; ++j)
      {
        svtkIdType u = pts[j];
        svtkIdType v = pts[((j + 1) % npts)];

        std::map<int, double>& mu = this->Internals->Adjacency[u];
        if (mu.find(v) == mu.end())
        {
          cost = this->CalculateStaticEdgeCost(inData, u, v);
          mu.insert(std::pair<int, double>(v, cost));
        }

        std::map<int, double>& mv = this->Internals->Adjacency[v];
        if (mv.find(u) == mv.end())
        {
          cost = this->CalculateStaticEdgeCost(inData, v, u);
          mv.insert(std::pair<int, double>(u, cost));
        }
      }
    }
  }

  this->AdjacencyBuildTime.Modified();
}

//----------------------------------------------------------------------------
void svtkDijkstraGraphGeodesicPath::TraceShortestPath(
  svtkDataSet* inData, svtkPolyData* outPoly, svtkIdType startv, svtkIdType endv)
{
  svtkPoints* points = svtkPoints::New();
  svtkCellArray* lines = svtkCellArray::New();

  // n is far to many. Adjusted later
  lines->InsertNextCell(this->NumberOfVertices);

  // trace backward
  svtkIdType v = endv;
  double pt[3];
  svtkIdType id;
  while (v != startv)
  {
    if (v < 0)
    {
      // Invalid vertex. Path does not exist.
      break;
    }

    this->IdList->InsertNextId(v);

    inData->GetPoint(v, pt);
    id = points->InsertNextPoint(pt);
    lines->InsertCellPoint(id);

    v = this->Internals->Predecessors[v];
  }

  if (v >= 0)
  {
    this->IdList->InsertNextId(v);
    inData->GetPoint(v, pt);
    id = points->InsertNextPoint(pt);
    lines->InsertCellPoint(id);
    lines->UpdateCellCount(points->GetNumberOfPoints());
  }
  else
  {
    points->Reset();
    lines->Reset();
  }

  outPoly->SetPoints(points);
  points->Delete();
  outPoly->SetLines(lines);
  lines->Delete();
}

//----------------------------------------------------------------------------
void svtkDijkstraGraphGeodesicPath::Relax(const int& u, const int& v, const double& w)
{
  double du = this->Internals->CumulativeWeights[u] + w;
  if (this->Internals->CumulativeWeights[v] > du)
  {
    this->Internals->CumulativeWeights[v] = du;
    this->Internals->Predecessors[v] = u;

    this->Internals->HeapDecreaseKey(v);
  }
}

//----------------------------------------------------------------------------
void svtkDijkstraGraphGeodesicPath::ShortestPath(svtkDataSet* inData, int startv, int endv)
{
  int u, v;

  if (this->RepelPathFromVertices && this->RepelVertices)
  {
    // loop over the pts and if they are in the image
    // get the associated index for that point and mark it as blocked
    for (int i = 0; i < this->RepelVertices->GetNumberOfPoints(); ++i)
    {
      double* pt = this->RepelVertices->GetPoint(i);
      u = inData->FindPoint(pt);
      if (u < 0 || u == startv || u == endv)
      {
        continue;
      }
      this->Internals->BlockedVertices[u] = true;
    }
  }

  this->Internals->CumulativeWeights[startv] = 0;

  this->Internals->HeapInsert(startv);
  this->Internals->OpenVertices[startv] = true;

  bool stop = false;
  while ((u = this->Internals->HeapExtractMin()) >= 0 && !stop)
  {
    // u is now in ClosedVertices since the shortest path to u is determined
    this->Internals->ClosedVertices[u] = true;
    // remove u from OpenVertices
    this->Internals->OpenVertices[u] = false;

    if (u == endv && this->StopWhenEndReached)
    {
      stop = true;
    }

    std::map<int, double>::iterator it = this->Internals->Adjacency[u].begin();

    // Update all vertices v adjacent to u
    for (; it != this->Internals->Adjacency[u].end(); ++it)
    {
      v = (*it).first;

      // ClosedVertices is the set of vertices with determined shortest path...
      // do not use them again
      if (!this->Internals->ClosedVertices[v])
      {
        // Only relax edges where the end is not in ClosedVertices
        // and edge is in OpenVertices
        double w;
        if (this->Internals->BlockedVertices[v])
        {
          w = SVTK_FLOAT_MAX;
        }
        else
        {
          w = (*it).second + this->CalculateDynamicEdgeCost(inData, u, v);
        }

        if (this->Internals->OpenVertices[v])
        {
          this->Relax(u, v, w);
        }
        // add edge v to OpenVertices
        else
        {
          this->Internals->OpenVertices[v] = true;
          this->Internals->CumulativeWeights[v] = this->Internals->CumulativeWeights[u] + w;

          // Set Predecessor of v to be u
          this->Internals->Predecessors[v] = u;
          this->Internals->HeapInsert(v);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkDijkstraGraphGeodesicPath::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "StopWhenEndReached: ";
  if (this->StopWhenEndReached)
  {
    os << "On\n";
  }
  else
  {
    os << "Off\n";
  }
  os << indent << "UseScalarWeights: ";
  if (this->UseScalarWeights)
  {
    os << "On\n";
  }
  else
  {
    os << "Off\n";
  }
  os << indent << "RepelPathFromVertices: ";
  if (this->RepelPathFromVertices)
  {
    os << "On\n";
  }
  else
  {
    os << "Off\n";
  }
  os << indent << "RepelVertices: " << this->RepelVertices << endl;
  os << indent << "IdList: " << this->IdList << endl;
  os << indent << "Number of vertices in input data: " << this->NumberOfVertices << endl;
}
