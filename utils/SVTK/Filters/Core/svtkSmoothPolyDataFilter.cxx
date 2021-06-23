/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSmoothPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSmoothPolyDataFilter.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellLocator.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolygon.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTriangleFilter.h"

#include <limits>

svtkStandardNewMacro(svtkSmoothPolyDataFilter);

// The following code defines a helper class for performing mesh smoothing
// across the surface of another mesh.
typedef struct _svtkSmoothPoint
{
  svtkIdType cellId; // cell
  int subId;        // cell sub id
  double p[3];      // parametric coords in cell
} svtkSmoothPoint;

class svtkSmoothPoints
{ //;prevent man page generation
public:
  svtkSmoothPoints();
  ~svtkSmoothPoints() { delete[] this->Array; }
  svtkIdType GetNumberOfPoints() { return this->MaxId + 1; }
  svtkSmoothPoint* GetSmoothPoint(svtkIdType i) { return this->Array + i; }
  svtkSmoothPoint* InsertSmoothPoint(svtkIdType ptId)
  {
    if (ptId >= this->Size)
    {
      this->Resize(ptId + 1);
    }
    if (ptId > this->MaxId)
    {
      this->MaxId = ptId;
    }
    return this->Array + ptId;
  }
  svtkSmoothPoint* Resize(svtkIdType sz); // reallocates data
  void Reset() { this->MaxId = -1; }

  svtkSmoothPoint* Array; // pointer to data
  svtkIdType MaxId;       // maximum index inserted thus far
  svtkIdType Size;        // allocated size of data
  svtkIdType Extend;      // grow array by this amount
};

svtkSmoothPoints::svtkSmoothPoints()
{
  this->MaxId = -1;
  this->Array = new svtkSmoothPoint[1000];
  this->Size = 1000;
  this->Extend = 5000;
}

svtkSmoothPoint* svtkSmoothPoints::Resize(svtkIdType sz)
{
  svtkSmoothPoint* newArray;
  svtkIdType newSize;

  if (sz >= this->Size)
  {
    newSize = this->Size + this->Extend * (((sz - this->Size) / this->Extend) + 1);
  }
  else
  {
    newSize = sz;
  }

  newArray = new svtkSmoothPoint[newSize];

  memcpy(newArray, this->Array, (sz < this->Size ? sz : this->Size) * sizeof(svtkSmoothPoint));

  this->Size = newSize;
  delete[] this->Array;
  this->Array = newArray;

  return this->Array;
}

// The following code defines methods for the svtkSmoothPolyDataFilter class
//

// Construct object with number of iterations 20; relaxation factor .01;
// feature edge smoothing turned off; feature
// angle 45 degrees; edge angle 15 degrees; and boundary smoothing turned
// on. Error scalars and vectors are not generated (by default). The
// convergence criterion is 0.0 of the bounding box diagonal.
svtkSmoothPolyDataFilter::svtkSmoothPolyDataFilter()
{
  this->Convergence = 0.0; // goes to number of specified iterations
  this->NumberOfIterations = 20;

  this->RelaxationFactor = .01;

  this->FeatureAngle = 45.0;
  this->EdgeAngle = 15.0;
  this->FeatureEdgeSmoothing = 0;
  this->BoundarySmoothing = 1;

  this->GenerateErrorScalars = 0;
  this->GenerateErrorVectors = 0;

  this->OutputPointsPrecision = svtkAlgorithm::DEFAULT_PRECISION;

  this->SmoothPoints = nullptr;

  // optional second input
  this->SetNumberOfInputPorts(2);
}

void svtkSmoothPolyDataFilter::SetSourceData(svtkPolyData* source)
{
  this->SetInputData(1, source);
}

svtkPolyData* svtkSmoothPolyDataFilter::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

#define SVTK_SIMPLE_VERTEX 0
#define SVTK_FIXED_VERTEX 1
#define SVTK_FEATURE_EDGE_VERTEX 2
#define SVTK_BOUNDARY_EDGE_VERTEX 3

namespace
{

// Special structure for marking vertices
typedef struct _svtkMeshVertex
{
  char type;
  svtkIdList* edges; // connected edges (list of connected point ids)
  _svtkMeshVertex()
  {
    type = SVTK_SIMPLE_VERTEX; // can smooth
    edges = nullptr;
  }
} svtkMeshVertex, *svtkMeshVertexPtr;

template <typename T>
struct svtkSPDF_InternalParams
{
  svtkSmoothPolyDataFilter* spdf;
  int numberOfIterations;
  svtkPoints* newPts;
  T factor;
  T conv;
  svtkIdType numPts;
  svtkMeshVertexPtr vertexPtr;
  svtkPolyData* source;
  svtkSmoothPoints* SmoothPoints;
  double* w;
  svtkCellLocator* cellLocator;
};

template <typename T>
void svtkSPDF_MovePoints(svtkSPDF_InternalParams<T>& params)
{
  int iterationNumber = 0;
  for (T maxDist = std::numeric_limits<T>::max();
       maxDist > params.conv && iterationNumber < params.numberOfIterations; ++iterationNumber)
  {
    if (iterationNumber && !(iterationNumber % 5))
    {
      params.spdf->UpdateProgress(0.5 + 0.5 * iterationNumber / params.numberOfIterations);
      if (params.spdf->GetAbortExecute())
      {
        break;
      }
    }

    maxDist = 0.0;
    T* newPtsCoords = static_cast<T*>(params.newPts->GetVoidPointer(0));
    T* start = newPtsCoords;
    svtkMeshVertexPtr vertsPtr = params.vertexPtr;
    svtkIdType npts, *edgeIdPtr;
    T dist, deltaX[3];
    double dist2, xNew[3], closestPt[3];

    // For each non-fixed vertex of the mesh, move the point toward the mean
    // position of its connected neighbors using the relaxation factor.
    for (svtkIdType i = 0; i < params.numPts; ++i)
    {
      if (vertsPtr->type != SVTK_FIXED_VERTEX && vertsPtr->edges != nullptr &&
        (npts = vertsPtr->edges->GetNumberOfIds()) > 0)
      {
        deltaX[0] = deltaX[1] = deltaX[2] = 0.0;
        edgeIdPtr = vertsPtr->edges->GetPointer(0);
        // Compute the mean (cumulated) direction vector
        for (svtkIdType j = 0; j < npts; ++j)
        {
          for (unsigned short k = 0; k < 3; ++k)
          {
            deltaX[k] += *(start + 3 * (*edgeIdPtr) + k);
          }
          ++edgeIdPtr;
        } // for all connected points

        // Move the point
        *newPtsCoords += params.factor * (deltaX[0] / npts - (*newPtsCoords));
        xNew[0] = *newPtsCoords;
        ++newPtsCoords;
        *newPtsCoords += params.factor * (deltaX[1] / npts - (*newPtsCoords));
        xNew[1] = *newPtsCoords;
        ++newPtsCoords;
        *newPtsCoords += params.factor * (deltaX[2] / npts - (*newPtsCoords));
        xNew[2] = *newPtsCoords;
        ++newPtsCoords;

        // Constrain point to surface
        if (params.source)
        {
          svtkSmoothPoint* sPtr = params.SmoothPoints->GetSmoothPoint(i);
          svtkCell* cell = nullptr;

          if (sPtr->cellId >= 0) // in cell
          {
            cell = params.source->GetCell(sPtr->cellId);
          }

          if (!cell ||
            cell->EvaluatePosition(xNew, closestPt, sPtr->subId, sPtr->p, dist2, params.w) == 0)
          { // not in cell anymore
            params.cellLocator->FindClosestPoint(xNew, closestPt, sPtr->cellId, sPtr->subId, dist2);
          }
          for (int k = 0; k < 3; ++k)
          {
            xNew[k] = closestPt[k];
          }
          params.newPts->SetPoint(i, xNew);
        }

        if ((dist = svtkMath::Norm(deltaX)) > maxDist)
        {
          maxDist = dist;
        }
      } // if can move point
      else
      {
        newPtsCoords += 3;
      }
      ++vertsPtr;
    } // for all points
  }   // for not converged or within iteration count

  svtkDebugWithObjectMacro(params.spdf, << "Performed " << iterationNumber << " smoothing passes");
}

} // namespace

int svtkSmoothPolyDataFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* source = nullptr;
  if (sourceInfo)
  {
    source = svtkPolyData::SafeDownCast(sourceInfo->Get(svtkDataObject::DATA_OBJECT()));
  }
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType numPts, numCells, i, numPolys, numStrips;
  int j, k;
  svtkIdType npts = 0;
  const svtkIdType* pts = nullptr;
  svtkIdType p1, p2;
  double conv;
  double x1[3], x2[3], x3[3], l1[3], l2[3];
  double CosFeatureAngle; // Cosine of angle between adjacent polys
  double CosEdgeAngle;    // Cosine of angle between adjacent edges
  double closestPt[3], dist2, *w = nullptr;
  svtkIdType numSimple = 0, numBEdges = 0, numFixed = 0, numFEdges = 0;
  svtkPolyData *inMesh, *Mesh;
  svtkPoints* inPts;
  svtkTriangleFilter* toTris = nullptr;
  svtkCellArray *inVerts, *inLines, *inPolys, *inStrips;
  svtkPoints* newPts;
  svtkMeshVertexPtr Verts;
  svtkCellLocator* cellLocator = nullptr;

  // Check input
  //
  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();
  if (numPts < 1 || numCells < 1)
  {
    svtkErrorMacro(<< "No data to smooth!");
    return 1;
  }

  CosFeatureAngle = cos(svtkMath::RadiansFromDegrees(this->FeatureAngle));
  CosEdgeAngle = cos(svtkMath::RadiansFromDegrees(this->EdgeAngle));

  svtkDebugMacro(<< "Smoothing " << numPts << " vertices, " << numCells << " cells with:\n"
                << "\tConvergence= " << this->Convergence << "\n"
                << "\tIterations= " << this->NumberOfIterations << "\n"
                << "\tRelaxation Factor= " << this->RelaxationFactor << "\n"
                << "\tEdge Angle= " << this->EdgeAngle << "\n"
                << "\tBoundary Smoothing " << (this->BoundarySmoothing ? "On\n" : "Off\n")
                << "\tFeature Edge Smoothing " << (this->FeatureEdgeSmoothing ? "On\n" : "Off\n")
                << "\tError Scalars " << (this->GenerateErrorScalars ? "On\n" : "Off\n")
                << "\tError Vectors " << (this->GenerateErrorVectors ? "On\n" : "Off\n"));

  if (this->NumberOfIterations <= 0 || this->RelaxationFactor == 0.0)
  { // don't do anything! pass data through
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    return 1;
  }

  // Perform topological analysis. What we're gonna do is build a connectivity
  // array of connected vertices. The outcome will be one of three
  // classifications for a vertex: SVTK_SIMPLE_VERTEX, SVTK_FIXED_VERTEX. or
  // SVTK_EDGE_VERTEX. Simple vertices are smoothed using all connected
  // vertices. FIXED vertices are never smoothed. Edge vertices are smoothed
  // using a subset of the attached vertices.
  //
  svtkDebugMacro(<< "Analyzing topology...");
  Verts = new svtkMeshVertex[numPts];

  inPts = input->GetPoints();
  conv = this->Convergence * input->GetLength();

  // check vertices first. Vertices are never smoothed_--------------
  for (inVerts = input->GetVerts(), inVerts->InitTraversal(); inVerts->GetNextCell(npts, pts);)
  {
    for (j = 0; j < npts; j++)
    {
      Verts[pts[j]].type = SVTK_FIXED_VERTEX;
    }
  }
  this->UpdateProgress(0.10);

  // now check lines. Only manifold lines can be smoothed------------
  for (inLines = input->GetLines(), inLines->InitTraversal(); inLines->GetNextCell(npts, pts);)
  {
    for (j = 0; j < npts; j++)
    {
      if (Verts[pts[j]].type == SVTK_SIMPLE_VERTEX)
      {
        if (j == (npts - 1)) // end-of-line marked FIXED
        {
          Verts[pts[j]].type = SVTK_FIXED_VERTEX;
        }
        else if (j == 0) // beginning-of-line marked FIXED
        {
          Verts[pts[0]].type = SVTK_FIXED_VERTEX;
          inPts->GetPoint(pts[0], x2);
          inPts->GetPoint(pts[1], x3);
        }
        else // is edge vertex (unless already edge vertex!)
        {
          Verts[pts[j]].type = SVTK_FEATURE_EDGE_VERTEX;
          Verts[pts[j]].edges = svtkIdList::New();
          Verts[pts[j]].edges->SetNumberOfIds(2);
          Verts[pts[j]].edges->SetId(0, pts[j - 1]);
          Verts[pts[j]].edges->SetId(1, pts[j + 1]);
        }
      } // if simple vertex

      else if (Verts[pts[j]].type == SVTK_FEATURE_EDGE_VERTEX)
      { // multiply connected, becomes fixed!
        Verts[pts[j]].type = SVTK_FIXED_VERTEX;
        Verts[pts[j]].edges->Delete();
        Verts[pts[j]].edges = nullptr;
      }

    } // for all points in this line
  }   // for all lines
  this->UpdateProgress(0.25);

  // now polygons and triangle strips-------------------------------
  inPolys = input->GetPolys();
  numPolys = inPolys->GetNumberOfCells();
  inStrips = input->GetStrips();
  numStrips = inStrips->GetNumberOfCells();

  if (numPolys > 0 || numStrips > 0)
  { // build cell structure
    svtkCellArray* polys;
    svtkIdType cellId;
    int numNei, nei, edge;
    svtkIdType numNeiPts;
    const svtkIdType* neiPts;
    double normal[3], neiNormal[3];
    svtkIdList* neighbors;

    neighbors = svtkIdList::New();
    neighbors->Allocate(SVTK_CELL_SIZE);

    inMesh = svtkPolyData::New();
    inMesh->SetPoints(inPts);
    inMesh->SetPolys(inPolys);
    Mesh = inMesh;

    if ((numStrips = inStrips->GetNumberOfCells()) > 0)
    { // convert data to triangles
      inMesh->SetStrips(inStrips);
      toTris = svtkTriangleFilter::New();
      toTris->SetInputData(inMesh);
      toTris->Update();
      Mesh = toTris->GetOutput();
    }

    Mesh->BuildLinks(); // to do neighborhood searching
    polys = Mesh->GetPolys();
    this->UpdateProgress(0.375);

    for (cellId = 0, polys->InitTraversal(); polys->GetNextCell(npts, pts); cellId++)
    {
      for (i = 0; i < npts; i++)
      {
        p1 = pts[i];
        p2 = pts[(i + 1) % npts];

        if (Verts[p1].edges == nullptr)
        {
          Verts[p1].edges = svtkIdList::New();
          Verts[p1].edges->Allocate(16, 6);
        }
        if (Verts[p2].edges == nullptr)
        {
          Verts[p2].edges = svtkIdList::New();
          Verts[p2].edges->Allocate(16, 6);
        }

        Mesh->GetCellEdgeNeighbors(cellId, p1, p2, neighbors);
        numNei = neighbors->GetNumberOfIds();

        edge = SVTK_SIMPLE_VERTEX;
        if (numNei == 0)
        {
          edge = SVTK_BOUNDARY_EDGE_VERTEX;
        }

        else if (numNei >= 2)
        {
          // check to make sure that this edge hasn't been marked already
          for (j = 0; j < numNei; j++)
          {
            if (neighbors->GetId(j) < cellId)
            {
              break;
            }
          }
          if (j >= numNei)
          {
            edge = SVTK_FEATURE_EDGE_VERTEX;
          }
        }

        else if (numNei == 1 && (nei = neighbors->GetId(0)) > cellId)
        {
          if (this->FeatureEdgeSmoothing)
          {
            svtkPolygon::ComputeNormal(inPts, npts, pts, normal);
            Mesh->GetCellPoints(nei, numNeiPts, neiPts);
            svtkPolygon::ComputeNormal(inPts, numNeiPts, neiPts, neiNormal);

            if (svtkMath::Dot(normal, neiNormal) <= CosFeatureAngle)
            {
              edge = SVTK_FEATURE_EDGE_VERTEX;
            }
          }
        }
        else // a visited edge; skip rest of analysis
        {
          continue;
        }

        if (edge && Verts[p1].type == SVTK_SIMPLE_VERTEX)
        {
          Verts[p1].edges->Reset();
          Verts[p1].edges->InsertNextId(p2);
          Verts[p1].type = edge;
        }
        else if ((edge && Verts[p1].type == SVTK_BOUNDARY_EDGE_VERTEX) ||
          (edge && Verts[p1].type == SVTK_FEATURE_EDGE_VERTEX) ||
          (!edge && Verts[p1].type == SVTK_SIMPLE_VERTEX))
        {
          Verts[p1].edges->InsertNextId(p2);
          if (Verts[p1].type && edge == SVTK_BOUNDARY_EDGE_VERTEX)
          {
            Verts[p1].type = SVTK_BOUNDARY_EDGE_VERTEX;
          }
        }

        if (edge && Verts[p2].type == SVTK_SIMPLE_VERTEX)
        {
          Verts[p2].edges->Reset();
          Verts[p2].edges->InsertNextId(p1);
          Verts[p2].type = edge;
        }
        else if ((edge && Verts[p2].type == SVTK_BOUNDARY_EDGE_VERTEX) ||
          (edge && Verts[p2].type == SVTK_FEATURE_EDGE_VERTEX) ||
          (!edge && Verts[p2].type == SVTK_SIMPLE_VERTEX))
        {
          Verts[p2].edges->InsertNextId(p1);
          if (Verts[p2].type && edge == SVTK_BOUNDARY_EDGE_VERTEX)
          {
            Verts[p2].type = SVTK_BOUNDARY_EDGE_VERTEX;
          }
        }
      }
    }

    inMesh->Delete();
    if (toTris)
    {
      toTris->Delete();
    }

    neighbors->Delete();
  } // if strips or polys

  this->UpdateProgress(0.50);

  // post-process edge vertices to make sure we can smooth them
  for (i = 0; i < numPts; i++)
  {
    if (Verts[i].type == SVTK_SIMPLE_VERTEX)
    {
      numSimple++;
    }

    else if (Verts[i].type == SVTK_FIXED_VERTEX)
    {
      numFixed++;
    }

    else if (Verts[i].type == SVTK_FEATURE_EDGE_VERTEX || Verts[i].type == SVTK_BOUNDARY_EDGE_VERTEX)
    { // see how many edges; if two, what the angle is

      if (!this->BoundarySmoothing && Verts[i].type == SVTK_BOUNDARY_EDGE_VERTEX)
      {
        Verts[i].type = SVTK_FIXED_VERTEX;
        numBEdges++;
      }

      else if ((npts = Verts[i].edges->GetNumberOfIds()) != 2)
      {
        Verts[i].type = SVTK_FIXED_VERTEX;
        numFixed++;
      }

      else // check angle between edges
      {
        inPts->GetPoint(Verts[i].edges->GetId(0), x1);
        inPts->GetPoint(i, x2);
        inPts->GetPoint(Verts[i].edges->GetId(1), x3);

        for (k = 0; k < 3; k++)
        {
          l1[k] = x2[k] - x1[k];
          l2[k] = x3[k] - x2[k];
        }
        if (svtkMath::Normalize(l1) >= 0.0 && svtkMath::Normalize(l2) >= 0.0 &&
          svtkMath::Dot(l1, l2) < CosEdgeAngle)
        {
          numFixed++;
          Verts[i].type = SVTK_FIXED_VERTEX;
        }
        else
        {
          if (Verts[i].type == SVTK_FEATURE_EDGE_VERTEX)
          {
            numFEdges++;
          }
          else
          {
            numBEdges++;
          }
        }
      } // if along edge
    }   // if edge vertex
  }     // for all points

  svtkDebugMacro(<< "Found\n\t" << numSimple << " simple vertices\n\t" << numFEdges
                << " feature edge vertices\n\t" << numBEdges << " boundary edge vertices\n\t"
                << numFixed << " fixed vertices\n\t");

  svtkDebugMacro(<< "Beginning smoothing iterations...");

  // We've setup the topology...now perform Laplacian smoothing
  //
  newPts = svtkPoints::New();

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    newPts->SetDataType(inPts->GetDataType());
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(SVTK_DOUBLE);
  }

  newPts->SetNumberOfPoints(numPts);

  // If Source defined, we do constrained smoothing (that is, points are
  // constrained to the surface of the mesh object).
  if (source)
  {
    this->SmoothPoints = new svtkSmoothPoints;
    svtkSmoothPoint* sPtr;
    cellLocator = svtkCellLocator::New();
    w = new double[source->GetMaxCellSize()];

    cellLocator->SetDataSet(source);
    cellLocator->BuildLocator();

    for (i = 0; i < numPts; i++)
    {
      sPtr = this->SmoothPoints->InsertSmoothPoint(i);
      cellLocator->FindClosestPoint(
        inPts->GetPoint(i), closestPt, sPtr->cellId, sPtr->subId, dist2);
      newPts->SetPoint(i, closestPt);
    }
  }
  else // smooth normally
  {
    for (i = 0; i < numPts; i++) // initialize to old coordinates
    {
      newPts->SetPoint(i, inPts->GetPoint(i));
    }
  }

  if (newPts->GetDataType() == SVTK_DOUBLE)
  {
    svtkSPDF_InternalParams<double> params = { this, this->NumberOfIterations, newPts,
      this->RelaxationFactor, conv, numPts, Verts, source, this->SmoothPoints, w, cellLocator };

    svtkSPDF_MovePoints(params);
  }
  else
  {
    svtkSPDF_InternalParams<float> params = { this, this->NumberOfIterations, newPts,
      static_cast<float>(this->RelaxationFactor), static_cast<float>(conv), numPts, Verts, source,
      this->SmoothPoints, w, cellLocator };

    svtkSPDF_MovePoints(params);
  }

  if (source)
  {
    cellLocator->Delete();
    delete this->SmoothPoints;
    delete[] w;
  }

  // Update output. Only point coordinates have changed.
  //
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  if (this->GenerateErrorScalars)
  {
    svtkFloatArray* newScalars = svtkFloatArray::New();
    newScalars->SetNumberOfTuples(numPts);
    for (i = 0; i < numPts; i++)
    {
      inPts->GetPoint(i, x1);
      newPts->GetPoint(i, x2);
      newScalars->SetComponent(i, 0, sqrt(svtkMath::Distance2BetweenPoints(x1, x2)));
    }
    int idx = output->GetPointData()->AddArray(newScalars);
    output->GetPointData()->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);
    newScalars->Delete();
  }

  if (this->GenerateErrorVectors)
  {
    svtkFloatArray* newVectors = svtkFloatArray::New();
    newVectors->SetNumberOfComponents(3);
    newVectors->SetNumberOfTuples(numPts);
    for (i = 0; i < numPts; i++)
    {
      inPts->GetPoint(i, x1);
      newPts->GetPoint(i, x2);
      for (j = 0; j < 3; j++)
      {
        x3[j] = x2[j] - x1[j];
      }
      newVectors->SetTuple(i, x3);
    }
    output->GetPointData()->SetVectors(newVectors);
    newVectors->Delete();
  }

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetVerts(input->GetVerts());
  output->SetLines(input->GetLines());
  output->SetPolys(input->GetPolys());
  output->SetStrips(input->GetStrips());

  // free up connectivity storage
  for (i = 0; i < numPts; i++)
  {
    if (Verts[i].edges != nullptr)
    {
      Verts[i].edges->Delete();
      Verts[i].edges = nullptr;
    }
  }
  delete[] Verts;

  return 1;
}

int svtkSmoothPolyDataFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }

  if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

void svtkSmoothPolyDataFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Convergence: " << this->Convergence << "\n";
  os << indent << "Number of Iterations: " << this->NumberOfIterations << "\n";
  os << indent << "Relaxation Factor: " << this->RelaxationFactor << "\n";
  os << indent << "Feature Edge Smoothing: " << (this->FeatureEdgeSmoothing ? "On\n" : "Off\n");
  os << indent << "Feature Angle: " << this->FeatureAngle << "\n";
  os << indent << "Edge Angle: " << this->EdgeAngle << "\n";
  os << indent << "Boundary Smoothing: " << (this->BoundarySmoothing ? "On\n" : "Off\n");
  os << indent << "Generate Error Scalars: " << (this->GenerateErrorScalars ? "On\n" : "Off\n");
  os << indent << "Generate Error Vectors: " << (this->GenerateErrorVectors ? "On\n" : "Off\n");
  if (this->GetSource())
  {
    os << indent << "Source: " << static_cast<void*>(this->GetSource()) << "\n";
  }
  else
  {
    os << indent << "Source (none)\n";
  }

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
