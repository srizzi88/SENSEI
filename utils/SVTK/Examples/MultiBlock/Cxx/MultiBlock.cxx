/*=========================================================================

  Program:   Visualization Toolkit
  Module:    MultiBlock.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates how multi-block datasets can be processed
// using the new svtkMultiBlockDataSet class.
//
// The command line arguments are:
// -D <path> => path to the data (SVTKData); the data should be in <path>/Data/

#include "svtkActor.h"
#include "svtkCellDataToPointData.h"
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkContourFilter.h"
#include "svtkDebugLeaks.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkOutlineCornerFilter.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkShrinkPolyData.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredGridOutlineFilter.h"
#include "svtkTestUtilities.h"
#include "svtkXMLStructuredGridReader.h"
#include <sstream>

int main(int argc, char* argv[])
{
  svtkCompositeDataPipeline* exec = svtkCompositeDataPipeline::New();
  svtkAlgorithm::SetDefaultExecutivePrototype(exec);
  exec->Delete();

  // Standard rendering classes
  svtkRenderer* ren = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(ren);
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // We will read three files and collect them together in one
  // multi-block dataset. I broke the combustor dataset into
  // three pieces and wrote them out separately.
  int i;
  svtkXMLStructuredGridReader* reader = svtkXMLStructuredGridReader::New();

  // svtkMultiBlockDataSet respresents multi-block datasets. See
  // the class documentation for more information.
  svtkMultiBlockDataSet* mb = svtkMultiBlockDataSet::New();

  for (i = 0; i < 3; i++)
  {
    // Here we load the three separate files (each containing
    // a structured grid dataset)
    std::ostringstream fname;
    fname << "Data/multicomb_" << i << ".vts" << ends;
    char* cfname = svtkTestUtilities::ExpandDataFileName(argc, argv, fname.str().c_str());
    reader->SetFileName(cfname);
    // We have to update since we are working without a SVTK pipeline.
    // This will read the file and the output of the reader will be
    // a valid structured grid data.
    reader->Update();
    delete[] cfname;

    // We create a copy to avoid adding the same data three
    // times (the output object of the reader does not change
    // when the filename changes)
    svtkStructuredGrid* sg = svtkStructuredGrid::New();
    sg->ShallowCopy(reader->GetOutput());

    // Add the structured grid to the multi-block dataset
    mb->SetBlock(i, sg);
    sg->Delete();
  }
  reader->Delete();

  // Multi-block can be processed with regular SVTK filters in two ways:
  // 1. Pass through a multi-block aware consumer. Since a multi-block
  //    aware mapper is not yet available, svtkCompositeDataGeometryFilter
  //    can be used
  // 2. Assign the composite executive (svtkCompositeDataPipeline) to
  //    all "simple" (that work only on simple, non-composite datasets) filters

  // outline
  svtkStructuredGridOutlineFilter* of = svtkStructuredGridOutlineFilter::New();
  of->SetInputData(mb);

  // geometry filter
  // This filter is multi-block aware and will request blocks from the
  // input. These blocks will be processed by simple processes as if they
  // are the whole dataset
  svtkCompositeDataGeometryFilter* geom1 = svtkCompositeDataGeometryFilter::New();
  geom1->SetInputConnection(0, of->GetOutputPort(0));

  // Rendering objects
  svtkPolyDataMapper* geoMapper = svtkPolyDataMapper::New();
  geoMapper->SetInputConnection(0, geom1->GetOutputPort(0));

  svtkActor* geoActor = svtkActor::New();
  geoActor->SetMapper(geoMapper);
  geoActor->GetProperty()->SetColor(0, 0, 0);
  ren->AddActor(geoActor);

  // cell 2 point and contour
  svtkCellDataToPointData* c2p = svtkCellDataToPointData::New();
  c2p->SetInputData(mb);

  svtkContourFilter* contour = svtkContourFilter::New();
  contour->SetInputConnection(0, c2p->GetOutputPort(0));
  contour->SetValue(0, 0.45);

  // geometry filter
  svtkCompositeDataGeometryFilter* geom2 = svtkCompositeDataGeometryFilter::New();
  geom2->SetInputConnection(0, contour->GetOutputPort(0));

  // Rendering objects
  svtkPolyDataMapper* contMapper = svtkPolyDataMapper::New();
  contMapper->SetInputConnection(0, geom2->GetOutputPort(0));

  svtkActor* contActor = svtkActor::New();
  contActor->SetMapper(contMapper);
  contActor->GetProperty()->SetColor(1, 0, 0);
  ren->AddActor(contActor);

  ren->SetBackground(1, 1, 1);
  renWin->SetSize(300, 300);
  iren->Start();

  // Cleanup
  svtkAlgorithm::SetDefaultExecutivePrototype(nullptr);
  of->Delete();
  geom1->Delete();
  geoMapper->Delete();
  geoActor->Delete();
  c2p->Delete();
  contour->Delete();
  geom2->Delete();
  contMapper->Delete();
  contActor->Delete();
  ren->Delete();
  renWin->Delete();
  iren->Delete();
  mb->Delete();

  return 0;
}
