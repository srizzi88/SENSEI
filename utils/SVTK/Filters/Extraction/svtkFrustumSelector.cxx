/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFrustumSelector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkFrustumSelector.h"

#include "svtkCell.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkGenericCell.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkNew.h"
#include "svtkPlane.h"
#include "svtkPlanes.h"
#include "svtkPoints.h"
#include "svtkSMPTools.h"
#include "svtkSelectionNode.h"
#include "svtkSignedCharArray.h"
#include "svtkVoxel.h"

#include <vector>

#define MAXPLANE 6

namespace
{
//--------------------------------------------------------------------------
void ComputePlane(
  int idx, double v0[3], double v1[3], double v2[3], svtkPoints* points, svtkDoubleArray* norms)
{
  points->SetPoint(idx, v0[0], v0[1], v0[2]);

  double e0[3];
  e0[0] = v1[0] - v0[0];
  e0[1] = v1[1] - v0[1];
  e0[2] = v1[2] - v0[2];

  double e1[3];
  e1[0] = v2[0] - v0[0];
  e1[1] = v2[1] - v0[1];
  e1[2] = v2[2] - v0[2];

  double n[3];
  svtkMath::Cross(e0, e1, n);
  svtkMath::Normalize(n);

  norms->SetTuple(idx, n);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class ComputeCellsInFrustumFunctor
{
public:
  ComputeCellsInFrustumFunctor(svtkPlanes* f, svtkDataSet* in, svtkSignedCharArray* array)
    : Frustum(f)
    , Input(in)
    , Array(array)
  {
    svtkIdType i;
    double x[3];

    // find the near and far vertices to each plane for quick in/out tests
    for (i = 0; i < MAXPLANE; i++)
    {
      this->Frustum->GetNormals()->GetTuple(i, x);
      int xside = (x[0] > 0) ? 1 : 0;
      int yside = (x[1] > 0) ? 1 : 0;
      int zside = (x[2] > 0) ? 1 : 0;
      this->np_vertids[i][0] = (1 - xside) * 4 + (1 - yside) * 2 + (1 - zside);
      this->np_vertids[i][1] = xside * 4 + yside * 2 + zside;
    }
  }

  //--------------------------------------------------------------------------
  void operator()(svtkIdType begin, svtkIdType end)
  {
    double bounds[6];
    svtkNew<svtkGenericCell> cell;

    for (svtkIdType cellId = begin; cellId < end; ++cellId)
    {
      Input->GetCellBounds(cellId, bounds);
      Input->GetCell(cellId, cell);
      int isect = this->ABoxFrustumIsect(bounds, cell);
      if (isect == 1)
      {
        Array->SetValue(cellId, 1);
      }
      else
      {
        Array->SetValue(cellId, 0);
      }
    }
  }

  //--------------------------------------------------------------------------
  // Intersect the cell (with its associated bounds) with the clipping frustum.
  // Return 1 if at least partially inside, 0 otherwise.
  // Also return a distance to the near plane.
  int ABoxFrustumIsect(double* bounds, svtkCell* cell)
  {
    if (bounds[0] > bounds[1] || bounds[2] > bounds[3] || bounds[4] > bounds[5])
    {
      return this->IsectDegenerateCell(cell);
    }

    // convert bounds to 8 vertices
    double verts[8][3];
    verts[0][0] = bounds[0];
    verts[0][1] = bounds[2];
    verts[0][2] = bounds[4];
    verts[1][0] = bounds[0];
    verts[1][1] = bounds[2];
    verts[1][2] = bounds[5];
    verts[2][0] = bounds[0];
    verts[2][1] = bounds[3];
    verts[2][2] = bounds[4];
    verts[3][0] = bounds[0];
    verts[3][1] = bounds[3];
    verts[3][2] = bounds[5];
    verts[4][0] = bounds[1];
    verts[4][1] = bounds[2];
    verts[4][2] = bounds[4];
    verts[5][0] = bounds[1];
    verts[5][1] = bounds[2];
    verts[5][2] = bounds[5];
    verts[6][0] = bounds[1];
    verts[6][1] = bounds[3];
    verts[6][2] = bounds[4];
    verts[7][0] = bounds[1];
    verts[7][1] = bounds[3];
    verts[7][2] = bounds[5];

    int intersect = 0;

    // reject if any plane rejects the entire bbox
    svtkNew<svtkPlane> plane;
    for (int pid = 0; pid < MAXPLANE; pid++)
    {
      this->Frustum->GetPlane(pid, plane);
      double dist;
      int nvid;
      int pvid;
      nvid = this->np_vertids[pid][0];
      dist = plane->EvaluateFunction(verts[nvid]);
      if (dist > 0.0)
      {
        /*
        this->NumRejects++;
        */
        return 0;
      }
      pvid = this->np_vertids[pid][1];
      dist = plane->EvaluateFunction(verts[pvid]);
      if (dist > 0.0)
      {
        intersect = 1;
        break;
      }
    }

    // accept if entire bbox is inside all planes
    if (!intersect)
    {
      /*
      this->NumAccepts++;
      */
      return 1;
    }

    // otherwise we have to do clipping tests to decide if actually insects
    /*
    this->NumIsects++;
    */
    svtkCell* face;
    svtkCell* edge;
    svtkPoints* pts = nullptr;
    std::vector<double> vertbuffer;
    int maxedges = 16;
    // be ready to resize if we hit a polygon with many vertices
    vertbuffer.resize(3 * maxedges * 3);
    double* vlist = &vertbuffer[0 * maxedges * 3];
    double* wvlist = &vertbuffer[1 * maxedges * 3];
    double* ovlist = &vertbuffer[2 * maxedges * 3];

    int nfaces = cell->GetNumberOfFaces();
    if (nfaces < 1)
    {
      // some 2D cells have no faces, only edges
      int nedges = cell->GetNumberOfEdges();
      if (nedges < 1)
      {
        // SVTK_LINE and SVTK_POLY_LINE have no "edges" -- the cells
        // themselves are edges.  We catch them here and assemble the
        // list of vertices by hand because the code below assumes that
        // GetNumberOfEdges()==0 means a degenerate cell containing only
        // points.
        if (cell->GetCellType() == SVTK_LINE)
        {
          nedges = 2;
          svtkPoints* points = cell->GetPoints();
          points->GetPoint(0, &vlist[0 * 3]);
          points->GetPoint(1, &vlist[1 * 3]);
        }
        else if (cell->GetCellType() == SVTK_POLY_LINE)
        {
          nedges = cell->GetPointIds()->GetNumberOfIds();
          svtkPoints* points = cell->GetPoints();
          if (nedges + 4 > maxedges)
          {
            maxedges = (nedges + 4) * 2;
            vertbuffer.resize(3 * maxedges * 3);
            vlist = &vertbuffer[0 * maxedges * 3];
            wvlist = &vertbuffer[1 * maxedges * 3];
            ovlist = &vertbuffer[2 * maxedges * 3];
          }
          for (svtkIdType i = 0; i < cell->GetNumberOfPoints(); ++i)
          {
            points->GetPoint(i, &vlist[i * 3]);
          }
        }
        else
        {
          return this->IsectDegenerateCell(cell);
        }
      }
      if (nedges + 4 > maxedges)
      {
        maxedges = (nedges + 4) * 2;
        vertbuffer.resize(3 * maxedges * 3);
        vlist = &vertbuffer[0 * maxedges * 3];
        wvlist = &vertbuffer[1 * maxedges * 3];
        ovlist = &vertbuffer[2 * maxedges * 3];
      }
      edge = cell->GetEdge(0);
      if (edge)
      {
        pts = edge->GetPoints();
        pts->GetPoint(0, &vlist[0 * 3]);
        pts->GetPoint(1, &vlist[1 * 3]);
      }
      switch (cell->GetCellType())
      {
        case SVTK_PIXEL:
        {
          edge = cell->GetEdge(2);
          pts = edge->GetPoints();
          pts->GetPoint(0, &vlist[3 * 3]);
          pts->GetPoint(1, &vlist[2 * 3]);
          break;
        }
        case SVTK_QUAD:
        {
          edge = cell->GetEdge(2);
          pts = edge->GetPoints();
          pts->GetPoint(0, &vlist[2 * 3]);
          pts->GetPoint(1, &vlist[3 * 3]);
          break;
        }
        case SVTK_TRIANGLE:
        {
          edge = cell->GetEdge(1);
          pts = edge->GetPoints();
          pts->GetPoint(1, &vlist[2 * 3]);
          break;
        }
        case SVTK_LINE:
        case SVTK_POLY_LINE:
        {
          break;
        }
        default:
        {
          for (int e = 1; e < nedges - 1; e++)
          {
            edge = cell->GetEdge(e);
            pts = edge->GetPoints();
            pts->GetPoint(1, &vlist[(e + 1) * 3]); // get second point of the edge
          }
          break;
        }
      }
      if (this->FrustumClipPolygon(nedges, vlist, wvlist, ovlist))
      {
        return 1;
      }
    }
    else
    {

      // go around edges of each face and clip to planes
      // if nothing remains at the end, then we do not intersect and reject
      for (int f = 0; f < nfaces; f++)
      {
        face = cell->GetFace(f);

        int nedges = face->GetNumberOfEdges();
        if (nedges < 1)
        {
          if (this->IsectDegenerateCell(face))
          {
            return 1;
          }
          continue;
        }
        if (nedges + 4 > maxedges)
        {
          maxedges = (nedges + 4) * 2;
          vertbuffer.resize(3 * maxedges * 3);
          vlist = &vertbuffer[0 * maxedges * 3];
          wvlist = &vertbuffer[1 * maxedges * 3];
          ovlist = &vertbuffer[2 * maxedges * 3];
        }
        edge = face->GetEdge(0);
        pts = edge->GetPoints();
        pts->GetPoint(0, &vlist[0 * 3]);
        pts->GetPoint(1, &vlist[1 * 3]);
        switch (face->GetCellType())
        {
          case SVTK_PIXEL:
            edge = face->GetEdge(2);
            pts = edge->GetPoints();
            pts->GetPoint(0, &vlist[3 * 3]);
            pts->GetPoint(1, &vlist[2 * 3]);
            break;
          case SVTK_QUAD:
          {
            edge = face->GetEdge(2);
            pts = edge->GetPoints();
            pts->GetPoint(0, &vlist[2 * 3]);
            pts->GetPoint(1, &vlist[3 * 3]);
            break;
          }
          case SVTK_TRIANGLE:
          {
            edge = face->GetEdge(1);
            pts = edge->GetPoints();
            pts->GetPoint(1, &vlist[2 * 3]);
            break;
          }
          case SVTK_LINE:
          {
            break;
          }
          default:
          {
            for (int e = 1; e < nedges - 1; e++)
            {
              edge = cell->GetEdge(e);
              pts = edge->GetPoints();
              pts->GetPoint(1, &vlist[(e + 1) * 3]); // get second point of the edge
            }
            break;
          }
        }
        if (this->FrustumClipPolygon(nedges, vlist, wvlist, ovlist))
        {
          return 1;
        }
      }
    }

    return 0;
  }

  //--------------------------------------------------------------------------
  // handle degenerate cells by testing each point, if any in, then in
  int IsectDegenerateCell(svtkCell* cell)
  {
    svtkIdType npts = cell->GetNumberOfPoints();
    svtkPoints* pts = cell->GetPoints();
    double x[3];
    for (svtkIdType i = 0; i < npts; i++)
    {
      pts->GetPoint(i, x);
      if (this->Frustum->EvaluateFunction(x) < 0.0)
      {
        return 1;
      }
    }
    return 0;
  }

  //--------------------------------------------------------------------------
  // clips the polygon against the frustum
  // if there is no intersection, returns 0
  // if there is an intersection, returns 1
  // update ovlist to contain the resulting clipped vertices
  int FrustumClipPolygon(int nverts, double* ivlist, double* wvlist, double* ovlist)
  {
    int nwverts = nverts;
    memcpy(wvlist, ivlist, nverts * sizeof(double) * 3);

    int noverts = 0;
    int pid;
    for (pid = 0; pid < MAXPLANE; pid++)
    {
      noverts = 0;
      this->PlaneClipPolygon(nwverts, wvlist, pid, noverts, ovlist);
      if (noverts == 0)
      {
        return 0;
      }
      memcpy(wvlist, ovlist, noverts * sizeof(double) * 3);
      nwverts = noverts;
    }

    return 1;
  }

  //--------------------------------------------------------------------------
  // clips a polygon against the numbered plane, resulting vertices are stored
  // in ovlist, noverts
  void PlaneClipPolygon(int nverts, double* ivlist, int pid, int& noverts, double* ovlist)
  {
    int vid;
    // run around the polygon and clip to this edge
    for (vid = 0; vid < nverts - 1; vid++)
    {
      this->PlaneClipEdge(&ivlist[vid * 3], &ivlist[(vid + 1) * 3], pid, noverts, ovlist);
    }
    this->PlaneClipEdge(&ivlist[(nverts - 1) * 3], &ivlist[0 * 3], pid, noverts, ovlist);
  }

  //--------------------------------------------------------------------------
  // clips a line segment against the numbered plane.
  // intersection point and the second vertex are added to overts if on or inside
  void PlaneClipEdge(double* V0, double* V1, int pid, int& noverts, double* overts)
  {
    double t = 0.0;
    double ISECT[3];
    double normal[3], point[3];
    this->Frustum->GetNormals()->GetTuple(pid, normal);
    this->Frustum->GetPoints()->GetPoint(pid, point);
    int rc = svtkPlane::IntersectWithLine(V0, V1, normal, point, t, ISECT);

    if (rc)
    {
      overts[noverts * 3 + 0] = ISECT[0];
      overts[noverts * 3 + 1] = ISECT[1];
      overts[noverts * 3 + 2] = ISECT[2];
      noverts++;
    }

    svtkNew<svtkPlane> plane;
    this->Frustum->GetPlane(pid, plane);

    if (plane->EvaluateFunction(V1) < 0.0)
    {
      overts[noverts * 3 + 0] = V1[0];
      overts[noverts * 3 + 1] = V1[1];
      overts[noverts * 3 + 2] = V1[2];
      noverts++;
    }
  }

  svtkPlanes* Frustum;
  svtkDataSet* Input;
  svtkSignedCharArray* Array;
  int np_vertids[6][2];
};
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkFrustumSelector);

//----------------------------------------------------------------------------
svtkFrustumSelector::svtkFrustumSelector(svtkPlanes* f)
{
  this->Frustum = f;
  if (!this->Frustum)
  {
    // an inside out unit cube - which selects nothing
    double verts[32] = {
      0.0, 0.0, 0.0, 0.0, //
      0.0, 0.0, 1.0, 0.0, //
      0.0, 1.0, 0.0, 0.0, //
      0.0, 1.0, 1.0, 0.0, //
      1.0, 0.0, 0.0, 0.0, //
      1.0, 0.0, 1.0, 0.0, //
      1.0, 1.0, 0.0, 0.0, //
      1.0, 1.0, 1.0, 0.0  //
    };
    this->Frustum = svtkSmartPointer<svtkPlanes>::New();
    this->CreateFrustum(verts);
  }
}

//----------------------------------------------------------------------------
svtkFrustumSelector::~svtkFrustumSelector() = default;

//----------------------------------------------------------------------------
svtkPlanes* svtkFrustumSelector::GetFrustum()
{
  return this->Frustum;
}

//----------------------------------------------------------------------------
void svtkFrustumSelector::SetFrustum(svtkPlanes* f)
{
  if (this->Frustum != f)
  {
    this->Frustum = f;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
svtkMTimeType svtkFrustumSelector::GetMTime()
{
  svtkMTimeType mTime = this->MTime.GetMTime();
  svtkMTimeType impFuncMTime;

  if (this->Frustum != nullptr)
  {
    impFuncMTime = this->Frustum->GetMTime();
    mTime = (impFuncMTime > mTime ? impFuncMTime : mTime);
  }

  return mTime;
}

//--------------------------------------------------------------------------
void svtkFrustumSelector::CreateFrustum(double verts[32])
{
  svtkNew<svtkPoints> points;
  points->SetNumberOfPoints(6);

  svtkNew<svtkDoubleArray> norms;
  norms->SetNumberOfComponents(3);
  norms->SetNumberOfTuples(6);

  // left
  ComputePlane(0, &verts[0 * 4], &verts[2 * 4], &verts[3 * 4], points, norms);
  // right
  ComputePlane(1, &verts[7 * 4], &verts[6 * 4], &verts[4 * 4], points, norms);
  // bottom
  ComputePlane(2, &verts[5 * 4], &verts[4 * 4], &verts[0 * 4], points, norms);
  // top
  ComputePlane(3, &verts[2 * 4], &verts[6 * 4], &verts[7 * 4], points, norms);
  // near
  ComputePlane(4, &verts[6 * 4], &verts[2 * 4], &verts[0 * 4], points, norms);
  // far
  ComputePlane(5, &verts[1 * 4], &verts[3 * 4], &verts[7 * 4], points, norms);

  this->Frustum->SetPoints(points);
  this->Frustum->SetNormals(norms);
}

//--------------------------------------------------------------------------
void svtkFrustumSelector::Initialize(svtkSelectionNode* node)
{
  this->Superclass::Initialize(node);

  // sanity checks
  if (node && node->GetContentType() == svtkSelectionNode::FRUSTUM)
  {
    svtkDoubleArray* corners = svtkArrayDownCast<svtkDoubleArray>(node->GetSelectionList());
    this->CreateFrustum(corners->GetPointer(0));
  }
  else
  {
    svtkErrorMacro("Wrong type of selection node used to initialize svtkFrustumSelector");
  }
}

//--------------------------------------------------------------------------
bool svtkFrustumSelector::ComputeSelectedElements(
  svtkDataObject* input, svtkSignedCharArray* insidednessArray)
{
  svtkDataSet* inputDS = svtkDataSet::SafeDownCast(input);
  // frustum selection only supports datasets
  // if we don't have a selection node, the frustum is uninitialized...
  if (!inputDS || !this->Node)
  {
    svtkErrorMacro("Frustum selection only supports inputs of type svtkDataSet");
    return false;
  }
  auto fieldType = this->Node->GetProperties()->Get(svtkSelectionNode::FIELD_TYPE());
  if (fieldType == svtkSelectionNode::POINT)
  {
    this->ComputeSelectedPoints(inputDS, insidednessArray);
  }
  else if (fieldType == svtkSelectionNode::CELL)
  {
    this->ComputeSelectedCells(inputDS, insidednessArray);
  }
  else
  {
    svtkErrorMacro("Frustum selection only supports POINT and CELL association types");
    return false;
  }
  return true;
}

//--------------------------------------------------------------------------
void svtkFrustumSelector::ComputeSelectedPoints(svtkDataSet* input, svtkSignedCharArray* pointSelected)
{
  svtkIdType numPts = input->GetNumberOfPoints();
  if (numPts == 0)
  {
    return;
  }

  // Hacky PrepareForMultithreadedAccess()
  // call everything we will call on the data object on the main thread first
  // so that it can build its caching structures
  double xx[3];
  input->GetPoint(0, xx);

  svtkSMPTools::For(0, numPts, [input, this, &pointSelected](svtkIdType begin, svtkIdType end) {
    double x[3];
    for (svtkIdType ptId = begin; ptId < end; ++ptId)
    {
      input->GetPoint(ptId, x);
      if ((this->Frustum->EvaluateFunction(x)) < 0.0)
      {
        pointSelected->SetValue(ptId, 1);
      }
      else
      {
        pointSelected->SetValue(ptId, 0);
      }
    }
  });
}
//--------------------------------------------------------------------------
void svtkFrustumSelector::ComputeSelectedCells(svtkDataSet* input, svtkSignedCharArray* cellSelected)
{
  svtkIdType numCells = input->GetNumberOfCells();

  // Hacky PrepareForMultithreadedAccess()
  // call everything we will call on the data object on the main thread first
  // so that it can build its caching structures
  if (numCells == 0)
  {
    return;
  }
  double bounds[6];
  svtkNew<svtkGenericCell> cell;
  input->GetCellBounds(0, bounds);
  input->GetCell(0, cell);

  ComputeCellsInFrustumFunctor functor(this->Frustum, input, cellSelected);
  svtkSMPTools::For(0, numCells, functor);
}

void svtkFrustumSelector::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Frustum: " << static_cast<void*>(this->Frustum) << "\n";
}

int svtkFrustumSelector::OverallBoundsTest(double bounds[6])
{
  ComputeCellsInFrustumFunctor functor(this->Frustum, nullptr, nullptr);

  svtkNew<svtkVoxel> vox;
  svtkPoints* p = vox->GetPoints();
  p->SetPoint(0, bounds[0], bounds[2], bounds[4]);
  p->SetPoint(1, bounds[1], bounds[2], bounds[4]);
  p->SetPoint(2, bounds[0], bounds[3], bounds[4]);
  p->SetPoint(3, bounds[1], bounds[3], bounds[4]);
  p->SetPoint(4, bounds[0], bounds[2], bounds[5]);
  p->SetPoint(5, bounds[1], bounds[2], bounds[5]);
  p->SetPoint(6, bounds[0], bounds[3], bounds[5]);
  p->SetPoint(7, bounds[1], bounds[3], bounds[5]);

  int rc;
  rc = functor.ABoxFrustumIsect(bounds, vox);
  return (rc > 0);
}
