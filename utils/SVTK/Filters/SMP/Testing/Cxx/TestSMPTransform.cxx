/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSMPTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDataSet.h"
#include "svtkElevationFilter.h"
#include "svtkFloatArray.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkSMPThreadLocal.h"
#include "svtkSMPTools.h"
#include "svtkSMPTransform.h"
#include "svtkStructuredGrid.h"
#include "svtkTimerLog.h"
#include "svtkTransform.h"
#include "svtkTransformFilter.h"

const double spacing = 0.1;
const int resolution = 101;

class svtkSetFunctor2
{
public:
  float* pts;
  float* disp;

  void operator()(svtkIdType begin, svtkIdType end)
  {
    svtkIdType offset = 3 * begin * resolution * resolution;
    float* itr = pts + offset;
    float* ditr = disp + offset;

    for (int k = begin; k < end; k++)
      for (int j = 0; j < resolution; j++)
        for (int i = 0; i < resolution; i++)
        {
          *itr = i * spacing;
          itr++;
          *itr = j * spacing;
          itr++;
          *itr = k * spacing;
          itr++;

          *ditr = 10;
          ditr++;
          *ditr = 10;
          ditr++;
          *ditr = 10;
          ditr++;
        }
  }
};

int TestSMPTransform(int argc, char* argv[])
{
  int numThreads = 2;
  for (int argi = 1; argi < argc; argi++)
  {
    if (std::string(argv[argi]) == "--numThreads")
    {
      numThreads = atoi(argv[++argi]);
      break;
    }
  }
  cout << "Num. threads: " << numThreads << endl;
  svtkSMPTools::Initialize(numThreads);

  svtkNew<svtkTimerLog> tl;

  svtkNew<svtkStructuredGrid> sg;
  sg->SetDimensions(resolution, resolution, resolution);

  svtkNew<svtkPoints> pts;
  pts->SetNumberOfPoints(resolution * resolution * resolution);

  // svtkSetFunctor func;
  svtkSetFunctor2 func;
  func.pts = (float*)pts->GetVoidPointer(0);
  // func.pts = (svtkFloatArray*)pts->GetData();

  sg->SetPoints(pts);

  svtkNew<svtkFloatArray> disp;
  disp->SetNumberOfComponents(3);
  disp->SetNumberOfTuples(sg->GetNumberOfPoints());
  disp->SetName("Disp");
  sg->GetPointData()->AddArray(disp);
  func.disp = (float*)disp->GetVoidPointer(0);

  tl->StartTimer();
  svtkSMPTools::For(0, resolution, func);
  tl->StopTimer();
  cout << "Initialize: " << tl->GetElapsedTime() << endl;

  svtkNew<svtkTransformFilter> tr;
  tr->SetInputData(sg);

  svtkNew<svtkTransform> serialTr;
  serialTr->Identity();
  tr->SetTransform(serialTr);

  tl->StartTimer();
  tr->Update();
  tl->StopTimer();
  cout << "Serial transform: " << tl->GetElapsedTime() << endl;

  // Release memory so that we can do more.
  tr->GetOutput()->Initialize();

  svtkNew<svtkTransformFilter> tr2;
  tr2->SetInputData(sg);

  svtkNew<svtkSMPTransform> parallelTr;
  parallelTr->Identity();
  tr2->SetTransform(parallelTr);

  tl->StartTimer();
  tr2->Update();
  tl->StopTimer();
  cout << "Parallel transform: " << tl->GetElapsedTime() << endl;

  return EXIT_SUCCESS;
}
