/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTulipReaderClusters.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkForceDirectedLayoutStrategy.h"
#include "svtkGraphAnnotationLayersFilter.h"
#include "svtkGraphLayout.h"
#include "svtkGraphMapper.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkTulipReader.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestTulipReaderClusters(int argc, char* argv[])
{
  char* file = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/clustered-graph.tlp");
  SVTK_CREATE(svtkTulipReader, reader);
  reader->SetFileName(file);
  delete[] file;

  SVTK_CREATE(svtkForceDirectedLayoutStrategy, strategy);
  SVTK_CREATE(svtkGraphLayout, layout);
  layout->SetInputConnection(reader->GetOutputPort());
  layout->SetLayoutStrategy(strategy);

  SVTK_CREATE(svtkGraphMapper, graphMapper);
  graphMapper->SetInputConnection(layout->GetOutputPort());
  SVTK_CREATE(svtkActor, graphActor);
  graphActor->SetMapper(graphMapper);

  SVTK_CREATE(svtkGraphAnnotationLayersFilter, clusters);
  clusters->SetInputConnection(0, layout->GetOutputPort(0));
  clusters->SetInputConnection(1, reader->GetOutputPort(1));
  clusters->SetScaleFactor(1.2);
  clusters->SetMinHullSizeInWorld(0.02);
  clusters->SetMinHullSizeInDisplay(32);
  clusters->OutlineOn();

  SVTK_CREATE(svtkPolyDataMapper, clustersMapper);
  clustersMapper->SetInputConnection(clusters->GetOutputPort());
  clustersMapper->SelectColorArray("Hull color");
  clustersMapper->SetScalarModeToUseCellFieldData();
  clustersMapper->SetScalarVisibility(true);
  SVTK_CREATE(svtkActor, clustersActor);
  clustersActor->SetMapper(clustersMapper);

  SVTK_CREATE(svtkPolyDataMapper, outlineMapper);
  outlineMapper->SetInputConnection(clusters->GetOutputPort(1));
  SVTK_CREATE(svtkActor, outlineActor);
  outlineActor->SetMapper(outlineMapper);
  outlineActor->GetProperty()->SetColor(0.5, 0.7, 0.0);

  SVTK_CREATE(svtkRenderer, ren);
  clusters->SetRenderer(ren);
  ren->AddActor(graphActor);
  ren->AddActor(clustersActor);
  ren->AddActor(outlineActor);
  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  SVTK_CREATE(svtkRenderWindow, win);
  win->SetMultiSamples(0);
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  int retVal = svtkRegressionTestImage(win);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Initialize();
    iren->Start();

    retVal = svtkRegressionTester::PASSED;
  }

  return !retVal;
}
