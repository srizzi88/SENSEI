/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositePolyDataMapper2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCompositeDataDisplayAttributes.h"
#include "svtkCompositeDataSet.h"
#include "svtkCompositePolyDataMapper2.h"
#include "svtkCullerCollection.h"
#include "svtkInformation.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPointDataToCellData.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkRenderingOpenGLConfigure.h"
#include "svtkSmartPointer.h"
#include "svtkTimerLog.h"
#include "svtkTrivialProducer.h"

#include <svtkRegressionTestImage.h>
#include <svtkTestUtilities.h>

#include "svtkCylinderSource.h"
#include "svtkElevationFilter.h"

int TestCompositePolyDataMapper2CellScalars(int argc, char* argv[])
{
  bool timeit = false;
  if (argc > 1 && argv[1] && !strcmp(argv[1], "-timeit"))
  {
    timeit = true;
  }

  svtkSmartPointer<svtkRenderWindow> win = svtkSmartPointer<svtkRenderWindow>::New();
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkRenderer> ren = svtkSmartPointer<svtkRenderer>::New();
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  win->SetMultiSamples(0);

  svtkSmartPointer<svtkCompositePolyDataMapper2> mapper =
    svtkSmartPointer<svtkCompositePolyDataMapper2>::New();
  svtkNew<svtkCompositeDataDisplayAttributes> cdsa;
  mapper->SetCompositeDataDisplayAttributes(cdsa.GetPointer());

  int resolution = 18;
  svtkNew<svtkCylinderSource> cyl;
  cyl->CappingOn();
  cyl->SetRadius(0.2);
  cyl->SetResolution(resolution);

  svtkNew<svtkElevationFilter> elev;
  elev->SetInputConnection(cyl->GetOutputPort());

  svtkNew<svtkPointDataToCellData> p2c;
  p2c->SetInputConnection(elev->GetOutputPort());
  p2c->PassPointDataOff();

  // build a composite dataset
  svtkNew<svtkMultiBlockDataSet> data;
  int blocksPerLevel[3] = { 1, 32, 64 };
  if (timeit)
  {
    blocksPerLevel[1] = 64;
    blocksPerLevel[2] = 256;
  }
  std::vector<svtkSmartPointer<svtkMultiBlockDataSet> > blocks;
  blocks.push_back(data.GetPointer());
  unsigned levelStart = 0;
  unsigned levelEnd = 1;
  int numLevels = sizeof(blocksPerLevel) / sizeof(blocksPerLevel[0]);
  int numLeaves = 0;
  int numNodes = 0;
  svtkStdString blockName("Rolf");
  mapper->SetInputDataObject(data.GetPointer());
  for (int level = 1; level < numLevels; ++level)
  {
    int nblocks = blocksPerLevel[level];
    for (unsigned parent = levelStart; parent < levelEnd; ++parent)
    {
      blocks[parent]->SetNumberOfBlocks(nblocks);
      for (int block = 0; block < nblocks; ++block, ++numNodes)
      {
        if (level == numLevels - 1)
        {
          svtkNew<svtkPolyData> child;
          cyl->SetCenter(block * 0.25, 0.0, parent * 0.5);
          elev->SetLowPoint(block * 0.25 - 0.2 + 0.2 * block / nblocks, -0.02, 0.0);
          elev->SetHighPoint(block * 0.25 + 0.1 + 0.2 * block / nblocks, 0.02, 0.0);
          p2c->Update();
          child->DeepCopy(p2c->GetOutput(0));
          blocks[parent]->SetBlock(block, (block % 2) ? nullptr : child.GetPointer());
          blocks[parent]->GetMetaData(block)->Set(svtkCompositeDataSet::NAME(), blockName.c_str());
          // test not setting it on some
          if (block % 11)
          {
            mapper->SetBlockVisibility(parent + numLeaves, (block % 7) != 0);
          }
          ++numLeaves;
        }
        else
        {
          svtkNew<svtkMultiBlockDataSet> child;
          blocks[parent]->SetBlock(block, child.GetPointer());
          blocks.push_back(child.GetPointer());
        }
      }
    }
    levelStart = levelEnd;
    levelEnd = static_cast<unsigned>(blocks.size());
  }

  mapper->SetScalarModeToUseCellData();

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetEdgeColor(1, 0, 0);
  // actor->GetProperty()->EdgeVisibilityOn();
  ren->AddActor(actor);
  win->SetSize(400, 400);

  ren->RemoveCuller(ren->GetCullers()->GetLastItem());
  ren->ResetCamera();

  svtkSmartPointer<svtkTimerLog> timer = svtkSmartPointer<svtkTimerLog>::New();
  win->Render(); // get the window up

  // modify the data to force a rebuild of OpenGL structs
  // after rendering set one cylinder to white
  mapper->SetBlockColor(911, 1.0, 1.0, 1.0);
  mapper->SetBlockOpacity(911, 1.0);
  mapper->SetBlockVisibility(911, 1.0);

  timer->StartTimer();
  win->Render();
  timer->StopTimer();
  cout << "First frame time: " << timer->GetElapsedTime() << "\n";

  timer->StartTimer();

  int numFrames = (timeit ? 30 : 2);
  for (int i = 0; i <= numFrames; i++)
  {
    ren->GetActiveCamera()->Elevation(10.0 / numFrames);
    ren->GetActiveCamera()->Azimuth(-50.0 / numFrames);
    ren->GetActiveCamera()->Zoom(pow(2.5, 1.0 / numFrames));
    ren->GetActiveCamera()->Roll(20.0 / numFrames);
    win->Render();
  }

  timer->StopTimer();
  if (timeit)
  {
    double t = timer->GetElapsedTime();
    cout << "Avg Frame time: " << t / numFrames << " Frame Rate: " << numFrames / t << "\n";
  }

  int retVal = svtkRegressionTestImageThreshold(win.GetPointer(), 15);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
