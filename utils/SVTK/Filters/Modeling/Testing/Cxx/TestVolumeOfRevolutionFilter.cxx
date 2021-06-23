/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVolumeOfRevolutionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <iostream>
#include <string>

#include <svtkActor.h>
#include <svtkDataSetSurfaceFilter.h>
#include <svtkNew.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkTestUtilities.h>
#include <svtkVolumeOfRevolutionFilter.h>

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

#include <svtkCellIterator.h>
#include <svtkUnstructuredGrid.h>

#include <svtkCharArray.h>
#include <svtkDataArray.h>
#include <svtkDoubleArray.h>
#include <svtkFloatArray.h>
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
  vertex->GetPointIds()->SetId(0, points->InsertNextPoint(1., 1., 0.));

  svtkSmartPointer<svtkPolyVertex> polyVertex = svtkSmartPointer<svtkPolyVertex>::New();
  polyVertex->GetPointIds()->SetNumberOfIds(2);
  polyVertex->GetPointIds()->SetId(0, points->InsertNextPoint(.25, 0., 0.));
  polyVertex->GetPointIds()->SetId(1, points->InsertNextPoint(0., .35, 0.));

  svtkSmartPointer<svtkCellArray> verts = svtkSmartPointer<svtkCellArray>::New();
  verts->InsertNextCell(vertex);
  verts->InsertNextCell(polyVertex);

  svtkSmartPointer<svtkLine> line = svtkSmartPointer<svtkLine>::New();
  line->GetPointIds()->SetId(0, points->InsertNextPoint(.75, 0., 0.));
  line->GetPointIds()->SetId(1, points->InsertNextPoint(1., 0., 0.));

  svtkSmartPointer<svtkPolyLine> polyLine = svtkSmartPointer<svtkPolyLine>::New();
  polyLine->GetPointIds()->SetNumberOfIds(3);
  polyLine->GetPointIds()->SetId(0, points->InsertNextPoint(1.5, 2., 0.));
  polyLine->GetPointIds()->SetId(1, points->InsertNextPoint(1.3, 1.5, 0.));
  polyLine->GetPointIds()->SetId(2, points->InsertNextPoint(1.75, 2., 0.));

  svtkSmartPointer<svtkCellArray> lines = svtkSmartPointer<svtkCellArray>::New();
  lines->InsertNextCell(line);
  lines->InsertNextCell(polyLine);

  svtkSmartPointer<svtkTriangle> triangle = svtkSmartPointer<svtkTriangle>::New();
  triangle->GetPointIds()->SetId(0, points->InsertNextPoint(.5, -2., 0.));
  triangle->GetPointIds()->SetId(1, points->InsertNextPoint(1.5, -2., 0.));
  triangle->GetPointIds()->SetId(2, points->InsertNextPoint(1.5, -1., 0.));

  svtkSmartPointer<svtkQuad> quad = svtkSmartPointer<svtkQuad>::New();
  quad->GetPointIds()->SetId(0, points->InsertNextPoint(.5, -1., 0.));
  quad->GetPointIds()->SetId(1, points->InsertNextPoint(1., -1., 0.));
  quad->GetPointIds()->SetId(2, points->InsertNextPoint(1., .2, 0.));
  quad->GetPointIds()->SetId(3, points->InsertNextPoint(.5, 0., 0.));

  svtkSmartPointer<svtkPolygon> poly = svtkSmartPointer<svtkPolygon>::New();
  poly->GetPointIds()->SetNumberOfIds(5);
  poly->GetPointIds()->SetId(0, points->InsertNextPoint(2., 2., 0.));
  poly->GetPointIds()->SetId(1, points->InsertNextPoint(2., 3., 0.));
  poly->GetPointIds()->SetId(2, points->InsertNextPoint(3., 4., 0.));
  poly->GetPointIds()->SetId(3, points->InsertNextPoint(4., 6., 0.));
  poly->GetPointIds()->SetId(4, points->InsertNextPoint(6., 1., 0.));

  svtkSmartPointer<svtkCellArray> polys = svtkSmartPointer<svtkCellArray>::New();
  polys->InsertNextCell(triangle);
  polys->InsertNextCell(quad);
  polys->InsertNextCell(poly);

  svtkSmartPointer<svtkTriangleStrip> triangleStrip = svtkSmartPointer<svtkTriangleStrip>::New();
  triangleStrip->GetPointIds()->SetNumberOfIds(4);
  triangleStrip->GetPointIds()->SetId(0, points->InsertNextPoint(2., 0., 0.));
  triangleStrip->GetPointIds()->SetId(1, points->InsertNextPoint(2., 1., 0.));
  triangleStrip->GetPointIds()->SetId(2, points->InsertNextPoint(3., 0., 0.));
  triangleStrip->GetPointIds()->SetId(3, points->InsertNextPoint(3.5, 1., 0.));

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

//----------------------------------------------------------------------------
int TestVolumeOfRevolutionFilter(int argc, char* argv[])
{
  svtkSmartPointer<svtkPolyData> pd = GeneratePolyData();

  double position[3] = { -1., 0., 0. };
  double direction[3] = { 0., 1., 0. };

  svtkNew<svtkVolumeOfRevolutionFilter> revolve;
  revolve->SetSweepAngle(360.);
  revolve->SetAxisPosition(position);
  revolve->SetAxisDirection(direction);
  revolve->SetInputData(pd);
  revolve->Update();

  // test that the auxiliary arrays in svtkUnstructuredGrid output are correct
  {
    svtkUnstructuredGrid* ug = revolve->GetOutput();
    svtkCellIterator* it = ug->NewCellIterator();
    for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
    {
      svtkIdType npts;
      const svtkIdType* ptIds;
      ug->GetCellPoints(it->GetCellId(), npts, ptIds);
      svtkIdType numberOfPoints = it->GetNumberOfPoints();
      if (npts != numberOfPoints)
      {
        return 1;
      }
      svtkIdList* pointIds = it->GetPointIds();
      for (svtkIdType i = 0; i < numberOfPoints; i++)
      {
        if (ptIds[i] != pointIds->GetId(i))
        {
          return 1;
        }
      }
    }
    it->Delete();
  }

  svtkNew<svtkDataSetSurfaceFilter> surfaceFilter;
  surfaceFilter->SetInputConnection(revolve->GetOutputPort());

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(surfaceFilter->GetOutputPort());

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);

  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  renderWindow->SetSize(300, 300);

  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
