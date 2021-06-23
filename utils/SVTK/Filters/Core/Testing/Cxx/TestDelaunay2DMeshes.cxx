/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDelaunay2DMeshes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Test meshes obtained with svtkDelaunay2D.

#include "svtkCellArray.h"
#include "svtkDelaunay2D.h"
#include "svtkNew.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataReader.h"
#include "svtkPolyDataWriter.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkTransform.h"
#include "svtkTriangle.h"
#include "svtkXMLPolyDataReader.h"

#define SVTK_FAILURE 1

bool CompareMeshes(svtkPolyData* p1, svtkPolyData* p2)
{
  svtkIdType nbPoints1 = p1->GetNumberOfPoints();
  svtkIdType nbPoints2 = p2->GetNumberOfPoints();
  svtkIdType nbCells1 = p1->GetNumberOfCells();
  svtkIdType nbCells2 = p2->GetNumberOfCells();
  if (nbPoints1 != nbPoints2 || nbCells1 != nbCells2)
  {
    return false;
  }

  svtkCellArray* polys1 = p1->GetPolys();
  svtkCellArray* polys2 = p2->GetPolys();
  polys1->InitTraversal();
  polys2->InitTraversal();
  svtkIdType npts1;
  svtkIdType npts2;
  const svtkIdType* pts1;
  const svtkIdType* pts2;
  while (polys1->GetNextCell(npts1, pts1) && polys2->GetNextCell(npts2, pts2))
  {
    if (npts1 != npts2)
    {
      return false;
    }
    for (svtkIdType i = 0; i < npts1; i++)
    {
      if (pts1[i] != pts2[i])
      {
        return false;
      }
    }
  }

  return true;
}

void DumpMesh(svtkPolyData* mesh)
{
  svtkNew<svtkPolyDataWriter> writer;
  writer->SetInputData(mesh);
  writer->WriteToOutputStringOn();
  writer->Write();
  std::cerr << writer->GetOutputString() << std::endl;
}

bool TriangulationTest(const std::string& filePath)
{
  svtkNew<svtkPolyDataReader> inputReader;
  inputReader->SetFileName((filePath + "-Input.svtk").c_str());
  inputReader->Update();

  svtkNew<svtkDelaunay2D> delaunay2D;
  delaunay2D->SetInputConnection(inputReader->GetOutputPort());
  delaunay2D->SetSourceConnection(inputReader->GetOutputPort());
  delaunay2D->Update();

  svtkPolyData* obtainedMesh = delaunay2D->GetOutput();

  svtkNew<svtkPolyDataReader> outputReader;
  outputReader->SetFileName((filePath + "-Output.svtk").c_str());
  outputReader->Update();

  svtkPolyData* validMesh = outputReader->GetOutput();

  if (!CompareMeshes(validMesh, obtainedMesh))
  {
    std::cerr << "Obtained mesh is different from expected! "
                 "Its SVTK file follows:"
              << std::endl;
    DumpMesh(obtainedMesh);
    return false;
  }

  return true;
}

void GetTransform(svtkTransform* transform, svtkPoints* points)
{
  double zaxis[3] = { 0., 0., 1. };
  double pt0[3], pt1[3], pt2[3], normal[3];
  points->GetPoint(0, pt0);
  points->GetPoint(1, pt1);
  points->GetPoint(2, pt2);
  svtkTriangle::ComputeNormal(pt0, pt1, pt2, normal);

  double rotationAxis[3], center[3], rotationAngle;
  double dotZAxis = svtkMath::Dot(normal, zaxis);
  if (fabs(1.0 - dotZAxis) < 1e-6)
  {
    // Aligned with z-axis
    rotationAxis[0] = 1.0;
    rotationAxis[1] = 0.0;
    rotationAxis[2] = 0.0;
    rotationAngle = 0.0;
  }
  else if (fabs(1.0 + dotZAxis) < 1e-6)
  {
    // Co-linear with z-axis, but reversed sense.
    // Aligned with z-axis
    rotationAxis[0] = 1.0;
    rotationAxis[1] = 0.0;
    rotationAxis[2] = 0.0;
    rotationAngle = 180.0;
  }
  else
  {
    // The general case
    svtkMath::Cross(normal, zaxis, rotationAxis);
    svtkMath::Normalize(rotationAxis);
    rotationAngle = svtkMath::DegreesFromRadians(acos(svtkMath::Dot(zaxis, normal)));
  }

  transform->PreMultiply();
  transform->Identity();
  transform->RotateWXYZ(rotationAngle, rotationAxis[0], rotationAxis[1], rotationAxis[2]);

  svtkTriangle::TriangleCenter(pt0, pt1, pt2, center);
  transform->Translate(-center[0], -center[1], -center[2]);
}

bool TessellationTestWithTransform(const std::string& dataPath)
{
  std::string transformFilePath = dataPath + "-Transform.vtp";
  std::string boundaryFilePath = dataPath + "-Input.vtp";

  svtkNew<svtkXMLPolyDataReader> reader;
  reader->SetFileName(transformFilePath.c_str());
  reader->Update();

  svtkNew<svtkTransform> transform;
  svtkPoints* points = reader->GetOutput()->GetPoints();
  GetTransform(transform, points);

  reader->SetFileName(boundaryFilePath.c_str());
  reader->Update();
  svtkPolyData* boundaryPoly = reader->GetOutput();

  svtkNew<svtkDelaunay2D> del2D;
  del2D->SetInputData(boundaryPoly);
  del2D->SetSourceData(boundaryPoly);
  del2D->SetTolerance(0.0);
  del2D->SetAlpha(0.0);
  del2D->SetOffset(0);
  del2D->SetProjectionPlaneMode(SVTK_SET_TRANSFORM_PLANE);
  del2D->SetTransform(transform);
  del2D->BoundingTriangulationOff();
  del2D->Update();

  svtkPolyData* outPoly = del2D->GetOutput();

  if (outPoly->GetNumberOfCells() != boundaryPoly->GetNumberOfPoints() - 2)
  {
    std::cerr << "Bad triangulation for " << dataPath << "!" << std::endl;
    std::cerr << "Output has " << outPoly->GetNumberOfCells() << " cells instead of "
              << boundaryPoly->GetNumberOfPoints() - 2 << std::endl;
    return false;
  }
  return true;
}

int TestDelaunay2DMeshes(int argc, char* argv[])
{

  char* data_dir = svtkTestUtilities::GetDataRoot(argc, argv);
  if (!data_dir)
  {
    cerr << "Could not determine data directory." << endl;
    return SVTK_FAILURE;
  }

  std::string dataPath = std::string(data_dir) + "/Data/Delaunay/";
  delete[] data_dir;

  bool result = true;
  result &= TriangulationTest(dataPath + "DomainWithHole");
  result &= TessellationTestWithTransform(dataPath + "Test1");
  result &= TessellationTestWithTransform(dataPath + "Test2");
  result &= TessellationTestWithTransform(dataPath + "Test3");
  result &= TessellationTestWithTransform(dataPath + "Test4");
  result &= TessellationTestWithTransform(dataPath + "Test5");

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
