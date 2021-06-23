/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BoxClipTriangulate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// This test makes sure that svtkBoxClipDataSet correctly triangulates all cell
// types.

#include "svtkBoxClipDataSet.h"
#include "svtkCellArray.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkDoubleArray.h"
#include "svtkMath.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkTriangle.h"
#include "svtkUnstructuredGrid.h"

#include "svtkActor.h"
#include "svtkExtractEdges.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, var) svtkSmartPointer<type> var = svtkSmartPointer<type>::New()

#include <ctime>

#include <vector>

const int NumPoints = 13;
const double PointData[NumPoints * 3] = {
  0.0, 0.0, 0.0, //
  0.0, 1.0, 0.0, //
  1.0, 0.0, 0.0, //
  1.0, 1.0, 0.0, //
  2.0, 0.0, 0.0, //
  2.0, 1.0, 0.0, //

  0.0, 0.0, 1.0, //
  0.0, 1.0, 1.0, //
  1.0, 0.0, 1.0, //
  1.0, 1.0, 1.0, //
  2.0, 0.0, 1.0, //
  2.0, 1.0, 1.0, //
  2.0, 0.5, 1.0  //
};

const svtkIdType NumTriStripCells = 1;
const svtkIdType TriStripCells[] = {
  6, 1, 0, 3, 2, 5, 4 //
};

const svtkIdType NumQuadCells = 2;
const svtkIdType QuadCells[] = {
  4, 0, 2, 3, 1, //
  4, 2, 4, 5, 3  //
};

const svtkIdType NumPixelCells = 2;
const svtkIdType PixelCells[] = {
  4, 0, 2, 1, 3, //
  4, 2, 4, 3, 5  //
};

const svtkIdType NumPolyCells = 3;
const svtkIdType PolyCells[] = {
  4, 0, 2, 3, 1,    //
  3, 2, 4, 5,       //
  5, 6, 8, 12, 9, 7 //
};

const svtkIdType NumHexCells = 2;
const svtkIdType HexCells[] = {
  8, 6, 8, 2, 0, 7, 9, 3, 1,  //
  8, 4, 2, 8, 10, 5, 3, 9, 11 //
};
const svtkIdType NumExpectedHexSurfacePolys = 20;

const svtkIdType NumVoxelCells = 2;
const svtkIdType VoxelCells[] = {
  8, 0, 2, 1, 3, 6, 8, 7, 9,  //
  8, 10, 8, 11, 9, 4, 2, 5, 3 //
};
const svtkIdType NumExpectedVoxelSurfacePolys = 20;

const svtkIdType NumWedgeCells = 4;
const svtkIdType WedgeCells[] = {
  6, 0, 1, 2, 6, 7, 8,  //
  6, 7, 8, 9, 1, 2, 3,  //
  6, 8, 11, 9, 2, 5, 3, //
  6, 2, 5, 4, 8, 11, 10 //
};
const svtkIdType NumExpectedWedgeSurfacePolys = 20;

const svtkIdType NumPyramidCells = 2;
const svtkIdType PyramidCells[] = {
  5, 8, 9, 3, 2, 0, //
  5, 2, 3, 9, 8, 12 //
};
const svtkIdType NumExpectedPyramidSurfacePolys = 8;

class BoxClipTriangulateFailed
{
};

//-----------------------------------------------------------------------------

static void CheckWinding(svtkBoxClipDataSet* alg)
{
  alg->Update();
  svtkUnstructuredGrid* data = alg->GetOutput();

  svtkPoints* points = data->GetPoints();

  svtkCellArray* cells = data->GetCells();
  cells->InitTraversal();

  svtkIdType npts;
  const svtkIdType* pts;
  while (cells->GetNextCell(npts, pts))
  {
    if (npts != 4)
    {
      std::cout << "Weird.  I got something that is not a tetrahedra." << std::endl;
      continue;
    }

    double p0[3], p1[3], p2[3], p3[3];
    points->GetPoint(pts[0], p0);
    points->GetPoint(pts[1], p1);
    points->GetPoint(pts[2], p2);
    points->GetPoint(pts[3], p3);

    // If the winding is correct, the normal to triangle p0,p1,p2 should point
    // towards p3.
    double v0[3], v1[3];
    v0[0] = p1[0] - p0[0];
    v0[1] = p1[1] - p0[1];
    v0[2] = p1[2] - p0[2];
    v1[0] = p2[0] - p0[0];
    v1[1] = p2[1] - p0[1];
    v1[2] = p2[2] - p0[2];

    double n[3];
    svtkMath::Cross(v0, v1, n);

    double d[3];
    d[0] = p3[0] - p0[0];
    d[1] = p3[1] - p0[1];
    d[2] = p3[2] - p0[2];

    if (svtkMath::Dot(n, d) < 0)
    {
      std::cout << "Found a tetrahedra with bad winding." << std::endl;
      throw BoxClipTriangulateFailed();
    }
  }
}

//-----------------------------------------------------------------------------

static svtkSmartPointer<svtkUnstructuredGrid> BuildInput(
  int type, svtkIdType numcells, const svtkIdType* cells)
{
  svtkIdType i;

  SVTK_CREATE(svtkUnstructuredGrid, input);

  // Randomly shuffle the points to possibly test various tessellations.
  // Make a map from original point orderings to new point orderings.
  std::vector<svtkIdType> idMap;
  std::vector<svtkIdType> idsLeft;

  for (i = 0; i < NumPoints; i++)
  {
    idsLeft.push_back(i);
  }

  while (!idsLeft.empty())
  {
    svtkIdType next = std::lround(svtkMath::Random(-0.49, idsLeft.size() - 0.51));
    std::vector<svtkIdType>::iterator nextp = idsLeft.begin() + next;
    idMap.push_back(*nextp);
    idsLeft.erase(nextp, nextp + 1);
  }

  // Build shuffled points.
  SVTK_CREATE(svtkPoints, points);
  points->SetNumberOfPoints(NumPoints);
  for (i = 0; i < NumPoints; i++)
  {
    points->SetPoint(idMap[i], PointData + 3 * i);
  }
  input->SetPoints(points);

  // Add the cells with indices properly mapped.
  SVTK_CREATE(svtkIdList, ptIds);
  const svtkIdType* c = cells;
  for (i = 0; i < numcells; i++)
  {
    svtkIdType npts = *c;
    c++;
    ptIds->Initialize();
    for (svtkIdType j = 0; j < npts; j++)
    {
      ptIds->InsertNextId(idMap[*c]);
      c++;
    }
    input->InsertNextCell(type, ptIds);
  }

  return input;
}

//-----------------------------------------------------------------------------

static void Check2DPrimitive(int type, svtkIdType numcells, const svtkIdType* cells)
{
  svtkSmartPointer<svtkUnstructuredGrid> input = BuildInput(type, numcells, cells);

  SVTK_CREATE(svtkBoxClipDataSet, clipper);
  clipper->SetInputData(input);
  // Clip nothing.
  clipper->SetBoxClip(0.0, 2.0, 0.0, 1.0, 0.0, 1.0);
  clipper->Update();

  svtkUnstructuredGrid* output = clipper->GetOutput();

  if (output->GetNumberOfCells() < 1)
  {
    std::cout << "Output has no cells!" << std::endl;
    throw BoxClipTriangulateFailed();
  }

  // Check to make sure all the normals point in the z direction.
  svtkCellArray* outCells = output->GetCells();
  outCells->InitTraversal();
  svtkIdType npts;
  const svtkIdType* pts;
  while (outCells->GetNextCell(npts, pts))
  {
    if (npts != 3)
    {
      std::cout << "Got a primitive that is not a triangle!" << std::endl;
      throw BoxClipTriangulateFailed();
    }

    double n[3];
    svtkTriangle::ComputeNormal(output->GetPoints(), npts, pts, n);
    if ((n[0] > 0.1) || (n[1] > 0.1) || (n[2] < 0.9))
    {
      std::cout << "Primitive is facing the wrong way!" << std::endl;
      throw BoxClipTriangulateFailed();
    }
  }
}

//-----------------------------------------------------------------------------

static void Check3DPrimitive(
  int type, svtkIdType numcells, const svtkIdType* cells, svtkIdType numSurfacePolys)
{
  svtkSmartPointer<svtkUnstructuredGrid> input = BuildInput(type, numcells, cells);

  SVTK_CREATE(svtkBoxClipDataSet, clipper);
  clipper->SetInputData(input);
  // Clip nothing.
  clipper->SetBoxClip(0.0, 2.0, 0.0, 1.0, 0.0, 1.0);
  clipper->Update();

  svtkUnstructuredGrid* output = clipper->GetOutput();

  if (output->GetNumberOfCells() < 1)
  {
    std::cout << "Output has no cells!" << std::endl;
    throw BoxClipTriangulateFailed();
  }

#if 0
  SVTK_CREATE(svtkExtractEdges, edges);
  edges->SetInput(output);
  SVTK_CREATE(svtkPolyDataMapper, mapper);
  mapper->SetInputConnection(0, edges->GetOutputPort(0));
  SVTK_CREATE(svtkActor, actor);
  actor->SetMapper(mapper);
  SVTK_CREATE(svtkRenderer, renderer);
  renderer->AddActor(actor);
  SVTK_CREATE(svtkRenderWindow, renwin);
  renwin->AddRenderer(renderer);
  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);
  renwin->Render();
  iren->Start();
#endif

  CheckWinding(clipper);

  SVTK_CREATE(svtkDataSetSurfaceFilter, surface);
  surface->SetInputConnection(clipper->GetOutputPort());
  surface->Update();

  if (surface->GetOutput()->GetNumberOfCells() != numSurfacePolys)
  {
    std::cout << "Expected " << numSurfacePolys << " triangles on the surface, got "
              << surface->GetOutput()->GetNumberOfCells() << std::endl;
    throw BoxClipTriangulateFailed();
  }
}

//-----------------------------------------------------------------------------

int BoxClipTriangulate(int, char*[])
{
  long seed = time(nullptr);
  std::cout << "Random seed = " << seed << std::endl;
  svtkMath::RandomSeed(seed);
  svtkMath::Random();
  svtkMath::Random();
  svtkMath::Random();

  try
  {
    std::cout << "Checking triangle strip." << std::endl;
    Check2DPrimitive(SVTK_TRIANGLE_STRIP, NumTriStripCells, TriStripCells);

    std::cout << "Checking quadrilaterals." << std::endl;
    Check2DPrimitive(SVTK_QUAD, NumQuadCells, QuadCells);

    std::cout << "Checking pixels." << std::endl;
    Check2DPrimitive(SVTK_PIXEL, NumPixelCells, PixelCells);

    std::cout << "Checking polygons." << std::endl;
    Check2DPrimitive(SVTK_POLYGON, NumPolyCells, PolyCells);

    std::cout << "Checking hexahedrons." << std::endl;
    Check3DPrimitive(SVTK_HEXAHEDRON, NumHexCells, HexCells, NumExpectedHexSurfacePolys);

    std::cout << "Checking voxels." << std::endl;
    Check3DPrimitive(SVTK_VOXEL, NumVoxelCells, VoxelCells, NumExpectedVoxelSurfacePolys);

    std::cout << "Checking wedges." << std::endl;
    Check3DPrimitive(SVTK_WEDGE, NumWedgeCells, WedgeCells, NumExpectedWedgeSurfacePolys);

    std::cout << "Checking pyramids." << std::endl;
    Check3DPrimitive(SVTK_PYRAMID, NumPyramidCells, PyramidCells, NumExpectedPyramidSurfacePolys);
  }
  catch (BoxClipTriangulateFailed)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
