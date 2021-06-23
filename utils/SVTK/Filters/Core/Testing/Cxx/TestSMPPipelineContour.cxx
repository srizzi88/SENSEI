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
#include "svtkCompositeDataIterator.h"
#include "svtkExtentTranslator.h"
#include "svtkImageData.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPolyData.h"
#include "svtkRTAnalyticSource.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkSmartPointer.h"
#include "svtkSynchronizedTemplates3D.h"
#include "svtkThreadedCompositeDataPipeline.h"
#include "svtkTimerLog.h"
#include "svtkXMLMultiBlockDataWriter.h"

const int EXTENT = 100;
static int WholeExtent[] = { -EXTENT, EXTENT, -EXTENT, EXTENT, -EXTENT, EXTENT };
const int NUMBER_OF_PIECES = 50;
static svtkImageData* Pieces[NUMBER_OF_PIECES];

class svtkCreateImageData
{
  svtkSMPThreadLocalObject<svtkRTAnalyticSource> ImageSources;
  svtkNew<svtkExtentTranslator> Translator;

public:
  void Initialize()
  {
    svtkRTAnalyticSource*& source = this->ImageSources.Local();
    source->SetWholeExtent(-EXTENT, EXTENT, -EXTENT, EXTENT, -EXTENT, EXTENT);
  }

  void operator()(svtkIdType begin, svtkIdType end)
  {
    svtkRTAnalyticSource*& source = this->ImageSources.Local();

    for (svtkIdType i = begin; i < end; i++)
    {
      int extent[6];
      this->Translator->PieceToExtentThreadSafe(
        i, NUMBER_OF_PIECES, 0, WholeExtent, extent, svtkExtentTranslator::BLOCK_MODE, 0);
      source->UpdateExtent(extent);
      svtkImageData* piece = svtkImageData::New();
      piece->ShallowCopy(source->GetOutput());
      Pieces[i] = piece;
    }
  }

  void Reduce() {}
};

int TestSMPPipelineContour(int, char*[])
{
  svtkSMPTools::Initialize(2);

  svtkCreateImageData cid;
  svtkNew<svtkTimerLog> tl;

  tl->StartTimer();
  svtkSMPTools::For(0, NUMBER_OF_PIECES, cid);
  tl->StopTimer();

  cout << "Creation time: " << tl->GetElapsedTime() << endl;

  svtkNew<svtkMultiBlockDataSet> mbds;
  for (int i = 0; i < NUMBER_OF_PIECES; i++)
  {
    mbds->SetBlock(i, Pieces[i]);
    Pieces[i]->Delete();
  }

  svtkNew<svtkThreadedCompositeDataPipeline> executive;

  svtkNew<svtkSynchronizedTemplates3D> cf;
  cf->SetExecutive(executive);
  cf->SetInputData(mbds);
  cf->SetInputArrayToProcess(0, 0, 0, 0, "RTData");
  cf->SetValue(0, 200);
  tl->StartTimer();
  cf->Update();
  tl->StopTimer();

  cout << "Execution time: " << tl->GetElapsedTime() << endl;

  svtkIdType numCells = 0;
  svtkSmartPointer<svtkCompositeDataIterator> iter;
  iter.TakeReference(static_cast<svtkCompositeDataSet*>(cf->GetOutputDataObject(0))->NewIterator());
  iter->InitTraversal();
  while (!iter->IsDoneWithTraversal())
  {
    svtkPolyData* piece = static_cast<svtkPolyData*>(iter->GetCurrentDataObject());
    numCells += piece->GetNumberOfCells();
    iter->GoToNextItem();
  }

  cout << "Total num. cells: " << numCells << endl;

  svtkNew<svtkRTAnalyticSource> rt;
  rt->SetWholeExtent(-EXTENT, EXTENT, -EXTENT, EXTENT, -EXTENT, EXTENT);
  rt->Update();

  svtkNew<svtkSynchronizedTemplates3D> st;
  st->SetInputData(rt->GetOutput());
  st->SetInputArrayToProcess(0, 0, 0, 0, "RTData");
  st->SetValue(0, 200);

  tl->StartTimer();
  st->Update();
  tl->StopTimer();

  cout << "Serial execution time: " << tl->GetElapsedTime() << endl;

  cout << "Serial num. cells: " << st->GetOutput()->GetNumberOfCells() << endl;

  if (st->GetOutput()->GetNumberOfCells() != numCells)
  {
    cout << "Number of cells did not match." << endl;
    return EXIT_FAILURE;
  }

#if 0
  svtkNew<svtkXMLMultiBlockDataWriter> writer;
  writer->SetInputData(cf->GetOutputDataObject(0));
  writer->SetFileName("contour.vtm");
  writer->SetDataModeToAscii();
  writer->Write();
#endif

  return EXIT_SUCCESS;
}
