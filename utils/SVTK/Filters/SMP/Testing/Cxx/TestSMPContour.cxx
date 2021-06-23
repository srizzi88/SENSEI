/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDataSetTriangleFilter.h"
#include "svtkNew.h"
#include "svtkPolyData.h"
#include "svtkRTAnalyticSource.h"
#include "svtkSMPContourGrid.h"
#if !defined(SVTK_LEGACY_REMOVE)
#include "svtkSMPContourGridManyPieces.h"
#endif
#include "svtkCellData.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkContourFilter.h"
#include "svtkContourGrid.h"
#include "svtkElevationFilter.h"
#include "svtkNonMergingPointLocator.h"
#include "svtkPointData.h"
#include "svtkPointDataToCellData.h"
#include "svtkSMPTools.h"
#include "svtkTimerLog.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXMLMultiBlockDataWriter.h"
#include "svtkXMLPolyDataWriter.h"

#define WRITE_DEBUG 0

const int EXTENT = 30;
int TestSMPContour(int, char*[])
{
  svtkSMPTools::Initialize(2);

  svtkNew<svtkTimerLog> tl;

  svtkNew<svtkRTAnalyticSource> imageSource;
#if 1
  imageSource->SetWholeExtent(-EXTENT, EXTENT, -EXTENT, EXTENT, -EXTENT, EXTENT);
#else
  imageSource->SetWholeExtent(-EXTENT, EXTENT, -EXTENT, EXTENT, 0, 0);
#endif

  svtkNew<svtkElevationFilter> ev;
  ev->SetInputConnection(imageSource->GetOutputPort());
  ev->SetLowPoint(-EXTENT, -EXTENT, -EXTENT);
  ev->SetHighPoint(EXTENT, EXTENT, EXTENT);

  svtkNew<svtkDataSetTriangleFilter> tetraFilter;
  tetraFilter->SetInputConnection(ev->GetOutputPort());

  tl->StartTimer();

  svtkNew<svtkPointDataToCellData> p2c;
  p2c->SetInputConnection(tetraFilter->GetOutputPort());
  p2c->Update();

  tetraFilter->GetOutput()->GetCellData()->ShallowCopy(p2c->GetOutput()->GetCellData());

  tl->StopTimer();
  cout << "Data generation time: " << tl->GetElapsedTime() << endl;

  cout << "Contour grid: " << endl;
  svtkNew<svtkContourGrid> cg;
  cg->SetInputData(tetraFilter->GetOutput());
  cg->SetInputArrayToProcess(0, 0, 0, 0, "RTData");
  cg->SetValue(0, 200);
  cg->SetValue(1, 220);
  tl->StartTimer();
  cg->Update();
  tl->StopTimer();

  svtkIdType baseNumCells = cg->GetOutput()->GetNumberOfCells();

  cout << "Number of cells: " << cg->GetOutput()->GetNumberOfCells() << endl;
  cout << "NUmber of points: " << cg->GetOutput()->GetNumberOfPoints() << endl;
  cout << "Time: " << tl->GetElapsedTime() << endl;

  cout << "Contour filter: " << endl;
  svtkNew<svtkContourFilter> cf;
  cf->SetInputData(tetraFilter->GetOutput());
  cf->SetInputArrayToProcess(0, 0, 0, 0, "RTData");
  cf->SetValue(0, 200);
  cf->SetValue(1, 220);
  tl->StartTimer();
  cf->Update();
  tl->StopTimer();

  cout << "Number of cells: " << cf->GetOutput()->GetNumberOfCells() << endl;
  cout << "Time: " << tl->GetElapsedTime() << endl;

  cout << "SMP Contour grid: " << endl;
  svtkNew<svtkSMPContourGrid> cg2;
  cg2->SetInputData(tetraFilter->GetOutput());
  cg2->SetInputArrayToProcess(0, 0, 0, 0, "RTData");
  cg2->SetValue(0, 200);
  cg2->SetValue(1, 220);
  tl->StartTimer();
  cg2->Update();
  tl->StopTimer();

  cout << "Time: " << tl->GetElapsedTime() << endl;

#if WRITE_DEBUG
  svtkNew<svtkXMLPolyDataWriter> pdwriter;
  pdwriter->SetInputData(cg2->GetOutput());
  pdwriter->SetFileName("contour.vtp");
  // pwriter->SetDataModeToAscii();
  pdwriter->Write();
#endif

  if (cg2->GetOutput()->GetNumberOfCells() != baseNumCells)
  {
    cout << "Error in svtkSMPContourGrid (MergePieces = true) output." << endl;
    cout << "Number of cells does not match expected, " << cg2->GetOutput()->GetNumberOfCells()
         << " vs. " << baseNumCells << endl;
    return EXIT_FAILURE;
  }

  cout << "SMP Contour grid: " << endl;
  cg2->MergePiecesOff();
  tl->StartTimer();
  cg2->Update();
  tl->StopTimer();

  cout << "Time: " << tl->GetElapsedTime() << endl;

  svtkIdType numCells = 0;

  svtkCompositeDataSet* cds = svtkCompositeDataSet::SafeDownCast(cg2->GetOutputDataObject(0));
  if (cds)
  {
    svtkCompositeDataIterator* iter = cds->NewIterator();
    iter->InitTraversal();
    while (!iter->IsDoneWithTraversal())
    {
      svtkPolyData* pd = svtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
      if (pd)
      {
        numCells += pd->GetNumberOfCells();
      }
      iter->GoToNextItem();
    }
    iter->Delete();
  }

  if (numCells != baseNumCells)
  {
    cout << "Error in svtkSMPContourGrid (MergePieces = false) output." << endl;
    cout << "Number of cells does not match expected, " << numCells << " vs. " << baseNumCells
         << endl;
    return EXIT_FAILURE;
  }

#if !defined(SVTK_LEGACY_REMOVE)
  svtkNew<svtkSMPContourGridManyPieces> cg3;
  cg3->SetInputData(tetraFilter->GetOutput());
  cg3->SetInputArrayToProcess(0, 0, 0, 0, "RTData");
  cg3->SetValue(0, 200);
  cg3->SetValue(1, 220);
  cout << "SMP Contour grid: " << endl;
  tl->StartTimer();
  cg3->Update();
  tl->StopTimer();
  cout << "Time: " << tl->GetElapsedTime() << endl;

  numCells = 0;

  cds = svtkCompositeDataSet::SafeDownCast(cg2->GetOutputDataObject(0));
  if (cds)
  {
    svtkCompositeDataIterator* iter = cds->NewIterator();
    iter->InitTraversal();
    while (!iter->IsDoneWithTraversal())
    {
      svtkPolyData* pd = svtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
      if (pd)
      {
        numCells += pd->GetNumberOfCells();
      }
      iter->GoToNextItem();
    }
    iter->Delete();
  }

  if (numCells != baseNumCells)
  {
    cout << "Error in svtkSMPContourGridManyPieces output." << endl;
    cout << "Number of cells does not match expected, " << numCells << " vs. " << baseNumCells
         << endl;
    return EXIT_FAILURE;
  }

#if WRITE_DEBUG
  svtkNew<svtkXMLMultiBlockDataWriter> writer;
  writer->SetInputData(cg2->GetOutputDataObject(0));
  writer->SetFileName("contour1.vtm");
  writer->SetDataModeToAscii();
  writer->Write();

  svtkNew<svtkXMLMultiBlockDataWriter> writer2;
  writer2->SetInputData(cg3->GetOutputDataObject(0));
  writer2->SetFileName("contour2.vtm");
  writer2->SetDataModeToAscii();
  writer2->Write();
#endif

#endif

  return EXIT_SUCCESS;
}
