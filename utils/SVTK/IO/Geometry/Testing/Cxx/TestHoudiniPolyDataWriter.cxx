/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHoudiniPolyDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <iostream>
#include <string>

#include <svtkHoudiniPolyDataWriter.h>
#include <svtkNew.h>
#include <svtkPolyData.h>
#include <svtkSmartPointer.h>
#include <svtkTestUtilities.h>

#include <svtkCellData.h>
#include <svtkLine.h>
#include <svtkPointData.h>
#include <svtkPolyLine.h>
#include <svtkPolyVertex.h>
#include <svtkPolygon.h>
#include <svtkQuad.h>
#include <svtkTriangle.h>
#include <svtkTriangleStrip.h>
#include <svtkVertex.h>

#include <svtkCharArray.h>
#include <svtkDataArray.h>
#include <svtkDoubleArray.h>
#include <svtkFloatArray.h>
#include <svtkIdTypeArray.h>
#include <svtkIntArray.h>
#include <svtkLongArray.h>
#include <svtkLongLongArray.h>
#include <svtkShortArray.h>
#include <svtkSignedCharArray.h>
#include <svtkStringArray.h>
#include <svtkUnsignedCharArray.h>
#include <svtkUnsignedIntArray.h>
#include <svtkUnsignedLongArray.h>
#include <svtkUnsignedLongLongArray.h>
#include <svtkUnsignedShortArray.h>

svtkSmartPointer<svtkPolyData> GeneratePolyData()
{
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();

  svtkSmartPointer<svtkVertex> vertex = svtkSmartPointer<svtkVertex>::New();
  vertex->GetPointIds()->SetId(0, points->InsertNextPoint(0., 0., 0.5));

  svtkSmartPointer<svtkPolyVertex> polyVertex = svtkSmartPointer<svtkPolyVertex>::New();
  polyVertex->GetPointIds()->SetNumberOfIds(2);
  polyVertex->GetPointIds()->SetId(0, points->InsertNextPoint(1., 0., 0.5));
  polyVertex->GetPointIds()->SetId(1, points->InsertNextPoint(0., 1., 0.5));

  svtkSmartPointer<svtkCellArray> verts = svtkSmartPointer<svtkCellArray>::New();
  verts->InsertNextCell(vertex);
  verts->InsertNextCell(polyVertex);

  svtkSmartPointer<svtkLine> line = svtkSmartPointer<svtkLine>::New();
  line->GetPointIds()->SetId(0, points->InsertNextPoint(0., 0., 1.));
  line->GetPointIds()->SetId(1, points->InsertNextPoint(1., 0., 1.));

  svtkSmartPointer<svtkPolyLine> polyLine = svtkSmartPointer<svtkPolyLine>::New();
  polyLine->GetPointIds()->SetNumberOfIds(3);
  polyLine->GetPointIds()->SetId(0, points->InsertNextPoint(1., 1., 1.));
  polyLine->GetPointIds()->SetId(1, points->InsertNextPoint(0., 1., 1.));
  polyLine->GetPointIds()->SetId(2, points->InsertNextPoint(1.5, 1., 1.));

  svtkSmartPointer<svtkCellArray> lines = svtkSmartPointer<svtkCellArray>::New();
  lines->InsertNextCell(line);
  lines->InsertNextCell(polyLine);

  svtkSmartPointer<svtkTriangle> triangle = svtkSmartPointer<svtkTriangle>::New();
  triangle->GetPointIds()->SetId(0, points->InsertNextPoint(0., 0., 2.));
  triangle->GetPointIds()->SetId(1, points->InsertNextPoint(1., 0., 2.));
  triangle->GetPointIds()->SetId(2, points->InsertNextPoint(1., 1., 2.));

  svtkSmartPointer<svtkQuad> quad = svtkSmartPointer<svtkQuad>::New();
  quad->GetPointIds()->SetId(0, points->InsertNextPoint(-1., -1., 2.));
  quad->GetPointIds()->SetId(1, points->InsertNextPoint(0., -1., 2.));
  quad->GetPointIds()->SetId(2, points->InsertNextPoint(0., 0., 2.));
  quad->GetPointIds()->SetId(3, points->InsertNextPoint(-1., 0., 2.));

  svtkSmartPointer<svtkPolygon> poly = svtkSmartPointer<svtkPolygon>::New();
  poly->GetPointIds()->SetNumberOfIds(5);
  poly->GetPointIds()->SetId(0, points->InsertNextPoint(2., 2., 2.));
  poly->GetPointIds()->SetId(1, points->InsertNextPoint(2., 3., 2.));
  poly->GetPointIds()->SetId(2, points->InsertNextPoint(3., 4., 2.));
  poly->GetPointIds()->SetId(3, points->InsertNextPoint(4., 6., 2.));
  poly->GetPointIds()->SetId(4, points->InsertNextPoint(6., 1., 2.));

  svtkSmartPointer<svtkCellArray> polys = svtkSmartPointer<svtkCellArray>::New();
  polys->InsertNextCell(triangle);
  polys->InsertNextCell(quad);
  polys->InsertNextCell(poly);

  svtkSmartPointer<svtkTriangleStrip> triangleStrip = svtkSmartPointer<svtkTriangleStrip>::New();
  triangleStrip->GetPointIds()->SetNumberOfIds(4);
  triangleStrip->GetPointIds()->SetId(0, points->InsertNextPoint(0, 0., 3.));
  triangleStrip->GetPointIds()->SetId(1, points->InsertNextPoint(0, 1., 3.));
  triangleStrip->GetPointIds()->SetId(2, points->InsertNextPoint(1., 0., 3.));
  triangleStrip->GetPointIds()->SetId(3, points->InsertNextPoint(1.5, 1., 3.));

  svtkSmartPointer<svtkCellArray> strips = svtkSmartPointer<svtkCellArray>::New();
  strips->InsertNextCell(triangleStrip);

  svtkSmartPointer<svtkPolyData> pd = svtkSmartPointer<svtkPolyData>::New();
  pd->SetPoints(points);
  pd->SetVerts(verts);
  pd->SetLines(lines);
  pd->SetPolys(polys);
  pd->SetStrips(strips);

  svtkIdType nPoints = pd->GetNumberOfPoints();
  svtkIdType nCells = pd->GetNumberOfCells();

#define AddPointDataArray(dataType, svtkArrayType, nComponents, value)                              \
  svtkSmartPointer<svtkArrayType> myp_##svtkArrayType = svtkSmartPointer<svtkArrayType>::New();         \
  std::string name_myp_##svtkArrayType = "p_";                                                      \
  name_myp_##svtkArrayType.append(#svtkArrayType);                                                   \
  myp_##svtkArrayType->SetName(name_myp_##svtkArrayType.c_str());                                    \
  myp_##svtkArrayType->SetNumberOfComponents(nComponents);                                          \
  myp_##svtkArrayType->SetNumberOfTuples(nPoints);                                                  \
  {                                                                                                \
    dataType tuple[nComponents];                                                                   \
    for (svtkIdType j = 0; j < (nComponents); j++)                                                  \
    {                                                                                              \
      tuple[j] = value;                                                                            \
    }                                                                                              \
    for (svtkIdType i = 0; i < nPoints; i++)                                                        \
    {                                                                                              \
      for (svtkIdType j = 0; j < (nComponents); j++)                                                \
      {                                                                                            \
        tuple[j] += 1;                                                                             \
      }                                                                                            \
      myp_##svtkArrayType->SetTypedTuple(i, tuple);                                                 \
    }                                                                                              \
  }                                                                                                \
  pd->GetPointData()->AddArray(myp_##svtkArrayType)

#define AddCellDataArray(dataType, svtkArrayType, nComponents, value)                               \
  svtkSmartPointer<svtkArrayType> myc_##svtkArrayType = svtkSmartPointer<svtkArrayType>::New();         \
  std::string name_myc_##svtkArrayType = "c_";                                                      \
  name_myc_##svtkArrayType.append(#svtkArrayType);                                                   \
  myc_##svtkArrayType->SetName(name_myc_##svtkArrayType.c_str());                                    \
  myc_##svtkArrayType->SetNumberOfComponents(nComponents);                                          \
  myc_##svtkArrayType->SetNumberOfTuples(nCells);                                                   \
  {                                                                                                \
    dataType tuple[nComponents];                                                                   \
    for (svtkIdType j = 0; j < (nComponents); j++)                                                  \
    {                                                                                              \
      tuple[j] = value;                                                                            \
    }                                                                                              \
    for (svtkIdType i = 0; i < nCells; i++)                                                         \
    {                                                                                              \
      for (svtkIdType j = 0; j < (nComponents); j++)                                                \
      {                                                                                            \
        tuple[j] += 1;                                                                             \
      }                                                                                            \
      myc_##svtkArrayType->SetTypedTuple(i, tuple);                                                 \
    }                                                                                              \
  }                                                                                                \
  pd->GetCellData()->AddArray(myc_##svtkArrayType)

  AddPointDataArray(int, svtkIntArray, 1, 0);
  AddPointDataArray(long, svtkLongArray, 1, 0);
  AddPointDataArray(long long, svtkLongLongArray, 1, 0);
  AddPointDataArray(short, svtkShortArray, 1, 0);
  AddPointDataArray(unsigned int, svtkUnsignedIntArray, 1, 0);
  AddPointDataArray(unsigned long, svtkUnsignedLongArray, 1, 0);
  AddPointDataArray(unsigned long long, svtkUnsignedLongLongArray, 1, 0);
  AddPointDataArray(unsigned short, svtkUnsignedShortArray, 1, 0);
  AddPointDataArray(svtkIdType, svtkIdTypeArray, 1, 0);
  AddPointDataArray(char, svtkCharArray, 1, '0');
  AddPointDataArray(unsigned char, svtkUnsignedCharArray, 1, '0');
  AddPointDataArray(signed char, svtkSignedCharArray, 1, '0');
  AddPointDataArray(float, svtkFloatArray, 1, 0.0);
  AddPointDataArray(double, svtkDoubleArray, 1, 0.0);

  AddCellDataArray(int, svtkIntArray, 1, 0);
  AddCellDataArray(long, svtkLongArray, 1, 0);
  AddCellDataArray(long long, svtkLongLongArray, 1, 0);
  AddCellDataArray(short, svtkShortArray, 1, 0);
  AddCellDataArray(unsigned int, svtkUnsignedIntArray, 1, 0);
  AddCellDataArray(unsigned long, svtkUnsignedLongArray, 1, 0);
  AddCellDataArray(unsigned long long, svtkUnsignedLongLongArray, 1, 0);
  AddCellDataArray(unsigned short, svtkUnsignedShortArray, 1, 0);
  AddCellDataArray(svtkIdType, svtkIdTypeArray, 1, 0);
  AddCellDataArray(char, svtkCharArray, 1, '0');
  AddCellDataArray(unsigned char, svtkUnsignedCharArray, 1, '0');
  AddCellDataArray(signed char, svtkSignedCharArray, 1, '0');
  AddCellDataArray(float, svtkFloatArray, 1, 0.0);
  AddCellDataArray(double, svtkDoubleArray, 1, 0.0);

#if 0
  svtkSmartPointer<svtkStringArray> myc_svtkStringArray =
    svtkSmartPointer<svtkStringArray>::New();
  myc_svtkStringArray->SetName("string");
  myc_svtkStringArray->SetNumberOfComponents(1);
  myc_svtkStringArray->SetNumberOfTuples(nCells);
  {
    for (svtkIdType i=0;i<nCells;i++)
    {
      std::stringstream s; s << "test" << i;
      myc_svtkStringArray->SetValue(i,s.str().c_str());
    }
  }
  pd->GetCellData()->AddArray(myc_svtkStringArray);
#endif

  return pd;
}

int TestHoudiniPolyDataWriter(int argc, char* argv[])
{
  char* temp_dir_c =
    svtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "SVTK_TEMP_DIR", "Testing/Temporary");
  std::string temp_dir = std::string(temp_dir_c);
  delete[] temp_dir_c;

  if (temp_dir.empty())
  {
    std::cerr << "Could not determine temporary directory." << std::endl;
    return EXIT_FAILURE;
  }

  std::string filename = temp_dir + "/testHoudiniPolyDataWriter.geo";

  svtkNew<svtkHoudiniPolyDataWriter> writer;
  writer->SetFileName(filename.c_str());
  svtkSmartPointer<svtkPolyData> pd = GeneratePolyData();
  writer->SetInputData(pd);
  writer->Write();

  return EXIT_SUCCESS;
}
