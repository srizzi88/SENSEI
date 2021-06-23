/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestValuePass2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the ability for the value pass to draw arrays as
// colors such that the visible values can be recovered from the pixels.

#include <svtkActor.h>
#include <svtkCamera.h>
#include <svtkCameraPass.h>
#include <svtkCellArray.h>
#include <svtkCellData.h>
#include <svtkDoubleArray.h>
#include <svtkMath.h>
#include <svtkOpenGLRenderer.h>
#include <svtkPointData.h>
#include <svtkPoints.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkRenderPassCollection.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSequencePass.h>
#include <svtkSmartPointer.h>
#include <svtkUnsignedCharArray.h>
#include <svtkValuePass.h>
#include <svtkWindowToImageFilter.h>

#include <set>

#define TESTVP_MAX 10

void PrepArray(bool byName, bool drawCell, int arrayIndex, int arrayComponent, svtkDataSet* dataset,
  svtkDataArray* values, svtkValuePass* valuePass, double*& minmax)
{
  if (drawCell)
  {
    if (arrayIndex > dataset->GetCellData()->GetNumberOfArrays())
    {
      arrayIndex = 0;
    }
    values = dataset->GetCellData()->GetArray(arrayIndex);
    if (arrayComponent > values->GetNumberOfComponents())
    {
      arrayComponent = 0;
    }
    cerr << "Drawing CELL " << values->GetName() << " [" << arrayComponent << "]" << endl;
    if (!byName)
    {
      valuePass->SetInputArrayToProcess(SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA, arrayIndex);
    }
    else
    {
      valuePass->SetInputArrayToProcess(SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA, values->GetName());
    }
    minmax = values->GetRange(arrayComponent);
  }
  else
  {
    if (arrayIndex > dataset->GetPointData()->GetNumberOfArrays())
    {
      arrayIndex = 0;
    }
    values = dataset->GetPointData()->GetArray(arrayIndex);
    if (arrayComponent > values->GetNumberOfComponents())
    {
      arrayComponent = 0;
    }
    cerr << "Drawing POINT " << values->GetName() << " [" << arrayComponent << "]" << endl;
    if (!byName)
    {
      valuePass->SetInputArrayToProcess(SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA, arrayIndex);
    }
    else
    {
      valuePass->SetInputArrayToProcess(SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA, values->GetName());
    }
    minmax = values->GetRange(arrayComponent);
  }
  valuePass->SetInputComponentToProcess(arrayComponent);
  valuePass->SetScalarRange(minmax[0], minmax[1]);
}

int TestValuePass2(int argc, char* argv[])
{
  bool byName = true;
  bool drawCell = true;
  unsigned int arrayIndex = 0;
  unsigned int arrayComponent = 0;
  bool interactive = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "index"))
    {
      byName = false;
    }
    if (!strcmp(argv[i], "point"))
    {
      drawCell = false;
    }
    if (!strcmp(argv[i], "N"))
    {
      arrayIndex = atoi(argv[i + 1]);
    }
    if (!strcmp(argv[i], "C"))
    {
      arrayComponent = atoi(argv[i + 1]);
    }
    if (!strcmp(argv[i], "-I"))
    {
      interactive = true;
    }
  }

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renderer->SetBackground(0.0, 0.0, 0.0);
  renderer->GradientBackgroundOff();

  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);

  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Create the data set
  svtkSmartPointer<svtkPolyData> dataset = svtkSmartPointer<svtkPolyData>::New();

  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  dataset->SetPoints(points);

  svtkSmartPointer<svtkDoubleArray> scalars = svtkSmartPointer<svtkDoubleArray>::New();
  scalars->SetNumberOfComponents(1);
  scalars->SetName("Point Scalar Array 1");
  dataset->GetPointData()->AddArray(scalars);

  svtkSmartPointer<svtkDoubleArray> vectors = svtkSmartPointer<svtkDoubleArray>::New();
  vectors->SetNumberOfComponents(3);
  vectors->SetName("Point Vector Array 1");
  dataset->GetPointData()->AddArray(vectors);

  double vector[3];
  for (unsigned int i = 0; i < TESTVP_MAX; i++)
  {
    for (unsigned int j = 0; j < TESTVP_MAX; j++)
    {
      points->InsertNextPoint(i, j, 0.0);
      scalars->InsertNextValue((double)i / TESTVP_MAX + 10);
      vector[0] = sin((double)j / TESTVP_MAX * 6.1418);
      vector[1] = 1.0;
      vector[2] = 1.0;
      svtkMath::Normalize(vector);
      vectors->InsertNextTuple3(vector[0], vector[1], vector[2]);
    }
  }

  svtkSmartPointer<svtkCellArray> cells = svtkSmartPointer<svtkCellArray>::New();
  dataset->SetPolys(cells);

  scalars = svtkSmartPointer<svtkDoubleArray>::New();
  scalars->SetNumberOfComponents(1);
  scalars->SetName("Cell Scalar Array 1");
  dataset->GetCellData()->AddArray(scalars);

  vectors = svtkSmartPointer<svtkDoubleArray>::New();
  vectors->SetNumberOfComponents(3);
  vectors->SetName("Cell Vector Array 1");
  dataset->GetCellData()->AddArray(vectors);
  for (unsigned int i = 0; i < (TESTVP_MAX - 1); i++)
  {
    double s = (double)i / (TESTVP_MAX - 1) - 10;
    for (unsigned int j = 0; j < (TESTVP_MAX - 1); j++)
    {
      cells->InsertNextCell(4);
      cells->InsertCellPoint(i * TESTVP_MAX + j);
      cells->InsertCellPoint(i * TESTVP_MAX + j + 1);
      cells->InsertCellPoint((i + 1) * TESTVP_MAX + j + 1);
      cells->InsertCellPoint((i + 1) * TESTVP_MAX + j);

      scalars->InsertNextValue(s);
      vector[0] = sin((double)j / (TESTVP_MAX - 1) * 6.1418);
      vector[1] = 1.0;
      vector[2] = 1.0;
      svtkMath::Normalize(vector);
      vectors->InsertNextTuple3(vector[0], vector[1], vector[2]);
    }
  }

  // Set up rendering pass
  svtkSmartPointer<svtkValuePass> valuePass = svtkSmartPointer<svtkValuePass>::New();

  svtkSmartPointer<svtkRenderPassCollection> passes = svtkSmartPointer<svtkRenderPassCollection>::New();
  passes->AddItem(valuePass);

  svtkSmartPointer<svtkSequencePass> sequence = svtkSmartPointer<svtkSequencePass>::New();
  sequence->SetPasses(passes);

  svtkSmartPointer<svtkCameraPass> cameraPass = svtkSmartPointer<svtkCameraPass>::New();
  cameraPass->SetDelegatePass(sequence);

  svtkOpenGLRenderer* glRenderer = svtkOpenGLRenderer::SafeDownCast(renderer);
  glRenderer->SetPass(cameraPass);

  svtkDataArray* values = nullptr;
  double* minmax;
  PrepArray(byName, drawCell, arrayIndex, arrayComponent, dataset, values, valuePass, minmax);

  double scale = minmax[1] - minmax[0];
  valuePass->SetInputComponentToProcess(arrayComponent);

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputData(dataset);

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  renderer->AddActor(actor);

  renderWindow->Render();

  // iterate to look for leaks and such
  for (int i = 0; i < 8; i++)
  {
    bool _byName = true;
    bool _drawCell = true;
    svtkFieldData* fd = dataset->GetCellData();
    if (i < 4)
    {
      _byName = false;
    }
    if (i % 2)
    {
      _drawCell = false;
      fd = dataset->GetPointData();
    }
    for (int j = 0; j < fd->GetNumberOfArrays(); j++)
    {
      for (int k = 0; k < fd->GetArray(j)->GetNumberOfComponents(); k++)
      {
        PrepArray(_byName, _drawCell, j, k, dataset, values, valuePass, minmax);
        renderWindow->Render();
      }
    }
  }

  PrepArray(byName, drawCell, arrayIndex, arrayComponent, dataset, values, valuePass, minmax);
  renderWindow->Render();

  svtkSmartPointer<svtkWindowToImageFilter> grabber = svtkSmartPointer<svtkWindowToImageFilter>::New();
  grabber->SetInput(renderWindow);
  grabber->Update();
  svtkImageData* id = grabber->GetOutput();
  // id->PrintSelf(cerr, svtkIndent(0));

  svtkUnsignedCharArray* ar =
    svtkArrayDownCast<svtkUnsignedCharArray>(id->GetPointData()->GetArray("ImageScalars"));
  unsigned char* ptr = static_cast<unsigned char*>(ar->GetVoidPointer(0));
  std::set<double> found;
  double value;
  for (int i = 0; i < id->GetNumberOfPoints(); i++)
  {
    valuePass->ColorToValue(ptr, minmax[0], scale, value);
    if (found.find(value) == found.end())
    {
      found.insert(value);
      cerr << "READ " << std::hex << (int)ptr[0] << (int)ptr[1] << (int)ptr[2] << "\t" << std::dec
           << value << endl;
    }
    ptr += 3;
  }

  std::set<double>::iterator it;
  double min = SVTK_DOUBLE_MAX;
  double max = SVTK_DOUBLE_MIN;
  for (it = found.begin(); it != found.end(); ++it)
  {
    if (*it < min)
    {
      min = *it;
    }
    if (*it > max)
    {
      max = *it;
    }
  }
  bool fail = false;
  if (fabs(min - -10.0) > 0.0001)
  {
    cerr << "ERROR min value not correct" << endl;
    fail = true;
  }
  if (fabs(max - -9.0) > 0.12)
  {
    cerr << "ERROR max value not correct" << endl;
    fail = true;
  }

  if (interactive)
  {
    renderWindowInteractor->Start();
  }

  return fail;
}
