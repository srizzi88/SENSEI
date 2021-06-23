/*=========================================================================

  Program:   Visualization Toolkit
  Module:    finance.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkActor.h"
#include "svtkAxes.h"
#include "svtkContourFilter.h"
#include "svtkDataSet.h"
#include "svtkFloatArray.h"
#include "svtkGaussianSplatter.h"
#include "svtkImageData.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTubeFilter.h"
#include "svtkUnstructuredGrid.h"
#include <svtkRegressionTestImage.h>
#include <svtksys/SystemTools.hxx>

#include <cstring>

#if defined(_MSC_VER)           /* Visual C++ (and Intel C++) */
#pragma warning(disable : 4996) // 'function': was declared deprecated
#endif

static svtkSmartPointer<svtkDataSet> ReadFinancialData(
  const char* fname, const char* x, const char* y, const char* z, const char* s);
static int ParseFile(FILE* file, const char* tag, float* data);

int main(int argc, char* argv[])
{
  double bounds[6];

  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " financial_file" << endl;
    return EXIT_FAILURE;
  }
  char* fname = argv[1];

  // read data
  svtkSmartPointer<svtkDataSet> dataSet =
    ReadFinancialData(fname, "MONTHLY_PAYMENT", "INTEREST_RATE", "LOAN_AMOUNT", "TIME_LATE");
  // construct pipeline for original population
  svtkSmartPointer<svtkGaussianSplatter> popSplatter = svtkSmartPointer<svtkGaussianSplatter>::New();
  popSplatter->SetInputData(dataSet);
  popSplatter->SetSampleDimensions(50, 50, 50);
  popSplatter->SetRadius(0.05);
  popSplatter->ScalarWarpingOff();

  svtkSmartPointer<svtkContourFilter> popSurface = svtkSmartPointer<svtkContourFilter>::New();
  popSurface->SetInputConnection(popSplatter->GetOutputPort());
  popSurface->SetValue(0, 0.01);

  svtkSmartPointer<svtkPolyDataMapper> popMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  popMapper->SetInputConnection(popSurface->GetOutputPort());
  popMapper->ScalarVisibilityOff();

  svtkSmartPointer<svtkActor> popActor = svtkSmartPointer<svtkActor>::New();
  popActor->SetMapper(popMapper);
  popActor->GetProperty()->SetOpacity(0.3);
  popActor->GetProperty()->SetColor(.9, .9, .9);

  // construct pipeline for delinquent population
  svtkSmartPointer<svtkGaussianSplatter> lateSplatter = svtkSmartPointer<svtkGaussianSplatter>::New();
  lateSplatter->SetInputData(dataSet);
  lateSplatter->SetSampleDimensions(50, 50, 50);
  lateSplatter->SetRadius(0.05);
  lateSplatter->SetScaleFactor(0.005);

  svtkSmartPointer<svtkContourFilter> lateSurface = svtkSmartPointer<svtkContourFilter>::New();
  lateSurface->SetInputConnection(lateSplatter->GetOutputPort());
  lateSurface->SetValue(0, 0.01);

  svtkSmartPointer<svtkPolyDataMapper> lateMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  lateMapper->SetInputConnection(lateSurface->GetOutputPort());
  lateMapper->ScalarVisibilityOff();

  svtkSmartPointer<svtkActor> lateActor = svtkSmartPointer<svtkActor>::New();
  lateActor->SetMapper(lateMapper);
  lateActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  // create axes
  popSplatter->Update();
  popSplatter->GetOutput()->GetBounds(bounds);

  svtkSmartPointer<svtkAxes> axes = svtkSmartPointer<svtkAxes>::New();
  axes->SetOrigin(bounds[0], bounds[2], bounds[4]);
  axes->SetScaleFactor(popSplatter->GetOutput()->GetLength() / 5);

  svtkSmartPointer<svtkTubeFilter> axesTubes = svtkSmartPointer<svtkTubeFilter>::New();
  axesTubes->SetInputConnection(axes->GetOutputPort());
  axesTubes->SetRadius(axes->GetScaleFactor() / 25.0);
  axesTubes->SetNumberOfSides(6);

  svtkSmartPointer<svtkPolyDataMapper> axesMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  axesMapper->SetInputConnection(axesTubes->GetOutputPort());

  svtkSmartPointer<svtkActor> axesActor = svtkSmartPointer<svtkActor>::New();
  axesActor->SetMapper(axesMapper);

  // graphics stuff
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();

  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(renderer);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // read data  //set up renderer
  renderer->AddActor(lateActor);
  renderer->AddActor(axesActor);
  renderer->AddActor(popActor);
  renderer->SetBackground(1, 1, 1);
  renWin->SetSize(300, 300);

  // For testing, check if "-V" is used to provide a regression test image
  if (argc >= 4 && strcmp(argv[2], "-V") == 0)
  {
    renWin->Render();
    int retVal = svtkRegressionTestImage(renWin);

    if (retVal == svtkTesting::FAILED)
    {
      return EXIT_FAILURE;
    }
    else if (retVal != svtkTesting::DO_INTERACTOR)
    {
      return EXIT_SUCCESS;
    }
  }

  // interact with data
  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}

static svtkSmartPointer<svtkDataSet> ReadFinancialData(
  const char* filename, const char* x, const char* y, const char* z, const char* s)
{
  float xyz[3];
  FILE* file;
  int i, npts;
  char tag[80];

  if ((file = svtksys::SystemTools::Fopen(filename, "r")) == nullptr)
  {
    std::cerr << "ERROR: Can't open file: " << filename << std::endl;
    return nullptr;
  }

  int n = fscanf(file, "%s %d", tag, &npts); // read number of points
  if (n != 2)
  {
    std::cerr << "ERROR: Can't read file: " << filename << std::endl;
    fclose(file);
    return nullptr;
  }
  // Check for a reasonable npts
  if (npts <= 0)
  {
    std::cerr << "ERROR: Number of points must be greater that 0" << std::endl;
    fclose(file);
    return nullptr;
  }
  // We arbitrarily pick a large upper limit on npts
  if (npts > SVTK_INT_MAX / 10)
  {
    std::cerr << "ERROR: npts (" << npts << ") is unreasonably large" << std::endl;
    fclose(file);
    return nullptr;
  }
  svtkSmartPointer<svtkUnstructuredGrid> dataSet = svtkSmartPointer<svtkUnstructuredGrid>::New();
  float* xV = new float[npts];
  float* yV = new float[npts];
  float* zV = new float[npts];
  float* sV = new float[npts];

  if (!ParseFile(file, x, xV) || !ParseFile(file, y, yV) || !ParseFile(file, z, zV) ||
    !ParseFile(file, s, sV))
  {
    std::cerr << "ERROR: Couldn't read data!" << std::endl;
    delete[] xV;
    delete[] yV;
    delete[] zV;
    delete[] sV;
    fclose(file);
    return nullptr;
  }

  svtkSmartPointer<svtkPoints> newPts = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkFloatArray> newScalars = svtkSmartPointer<svtkFloatArray>::New();

  for (i = 0; i < npts; i++)
  {
    xyz[0] = xV[i];
    xyz[1] = yV[i];
    xyz[2] = zV[i];
    newPts->InsertPoint(i, xyz);
    newScalars->InsertValue(i, sV[i]);
  }

  dataSet->SetPoints(newPts);
  dataSet->GetPointData()->SetScalars(newScalars);

  // cleanup
  delete[] xV;
  delete[] yV;
  delete[] zV;
  delete[] sV;
  fclose(file);

  return dataSet;
}

static int ParseFile(FILE* file, const char* label, float* data)
{
  char tag[80];
  int i, npts, readData = 0;
  float min = SVTK_FLOAT_MAX;
  float max = (-SVTK_FLOAT_MAX);

  if (file == nullptr || label == nullptr)
    return 0;

  rewind(file);

  if (fscanf(file, "%s %d", tag, &npts) != 2)
  {
    std::cerr << "ERROR: IO Error " << __FILE__ << ":" << __LINE__ << std::endl;
    return 0;
  }

  while (!readData && fscanf(file, "%s", tag) == 1)
  {
    if (!strcmp(tag, label))
    {
      readData = 1;
      for (i = 0; i < npts; i++)
      {
        if (fscanf(file, "%f", data + i) != 1)
        {
          std::cerr << "ERROR: IO Error " << __FILE__ << ":" << __LINE__ << std::endl;
          return 0;
        }
        if (data[i] < min)
          min = data[i];
        if (data[i] > min)
          max = data[i];
      }
      // normalize data
      for (i = 0; i < npts; i++)
        data[i] = min + data[i] / (max - min);
    }
    else
    {
      for (i = 0; i < npts; i++)
      {
        if (fscanf(file, "%*f") != 0)
        {
          std::cerr << "ERROR: IO Error " << __FILE__ << ":" << __LINE__ << std::endl;
          return 0;
        }
      }
    }
  }

  if (!readData)
    return 0;
  else
    return 1;
}
