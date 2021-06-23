#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCompositeDataIterator.h"
#include "svtkDataSet.h"
#include "svtkDataSetMapper.h"
#include "svtkExodusIIReader.h"
#include "svtkExodusIIWriter.h"
#include "svtkFieldData.h"
#include "svtkModelMetadata.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkPNGWriter.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStdString.h"
#include "svtkTestUtilities.h"
#include "svtkTesting.h"
#include "svtkWindowToImageFilter.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New();

int TestMultiBlockExodusWrite(int argc, char* argv[])
{
  char* InputFile;

  InputFile = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/edgeFaceElem.exii");
  if (!InputFile)
  {
    return 1;
  }

  SVTK_CREATE(svtkExodusIIReader, reader);
  if (!reader->CanReadFile(InputFile))
  {
    return 1;
  }
  reader->SetFileName(InputFile);
  reader->SetGlobalResultArrayStatus("CALIBER", 1);
  reader->SetGlobalResultArrayStatus("GUNPOWDER", 1);
  reader->Update();

  svtkMultiBlockDataSet* mbds = reader->GetOutput();
  if (!mbds)
  {
    return 1;
  }
  svtkMultiBlockDataSet* elems = svtkMultiBlockDataSet::SafeDownCast(mbds->GetBlock(0));
  if (!elems)
  {
    return 1;
  }
  if (elems->GetNumberOfBlocks() != 2)
  {
    return 1;
  }

  svtkFieldData* ifieldData = elems->GetBlock(0)->GetFieldData();
  int index;
  if (ifieldData->GetArray("CALIBER", index) == nullptr)
  {
    cerr << "Expected to find array CALIBER in original data set" << endl;
    return 1;
  }

  if (ifieldData->GetArray("GUNPOWDER", index) == nullptr)
  {
    cerr << "Expected to find array GUNPOWDER in original data set" << endl;
    return 1;
  }

  SVTK_CREATE(svtkTesting, testing);
  for (int i = 0; i < argc; i++)
  {
    testing->AddArgument(argv[i]);
  }

  svtkStdString OutputFile;
  OutputFile = testing->GetTempDirectory();
  OutputFile += "/testExodus.exii";

  SVTK_CREATE(svtkExodusIIWriter, writer);
  writer->SetInputConnection(reader->GetOutputPort());
  writer->SetFileName(OutputFile);
  writer->WriteOutBlockIdArrayOn();
  writer->WriteOutGlobalNodeIdArrayOn();
  writer->WriteOutGlobalElementIdArrayOn();
  writer->WriteAllTimeStepsOn();
  writer->Update();

  writer->GetModelMetadata()->PrintLocalInformation();

  SVTK_CREATE(svtkExodusIIReader, outputReader);
  if (!outputReader->CanReadFile(OutputFile))
  {
    return 1;
  }
  outputReader->SetFileName(OutputFile);
  outputReader->SetGlobalResultArrayStatus("CALIBER", 1);
  outputReader->SetGlobalResultArrayStatus("GUNPOWDER", 1);
  outputReader->Update();

  mbds = outputReader->GetOutput();
  if (!mbds)
  {
    return 1;
  }
  elems = svtkMultiBlockDataSet::SafeDownCast(mbds->GetBlock(0));
  if (!elems)
  {
    return 1;
  }
  if (elems->GetNumberOfBlocks() != 2)
  {
    return 1;
  }

  ifieldData = elems->GetBlock(0)->GetFieldData();
  if (ifieldData->GetArray("CALIBER", index) == nullptr)
  {
    cerr << "Array CALIBER was not written to output" << endl;
    return 1;
  }

  if (ifieldData->GetArray("GUNPOWDER", index) == nullptr)
  {
    cerr << "Array GUNPOWDER was not written to output" << endl;
    return 1;
  }

  svtkCompositeDataIterator* iter = mbds->NewIterator();
  iter->InitTraversal();
  svtkDataSet* ds = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
  iter->Delete();
  if (!ds)
  {
    return 1;
  }

  SVTK_CREATE(svtkDataSetMapper, mapper);
  mapper->SetInputData(ds);

  SVTK_CREATE(svtkActor, actor);
  actor->SetMapper(mapper);

  SVTK_CREATE(svtkRenderer, renderer);
  renderer->AddActor(actor);
  renderer->SetBackground(0.0, 0.0, 0.0);
  svtkCamera* c = renderer->GetActiveCamera();
  c->SetPosition(0.0, 10.0, 14.5);
  c->SetFocalPoint(0, 0, 0);
  c->SetViewUp(0.8, 0.3, -0.5);
  c->SetViewAngle(30);

  SVTK_CREATE(svtkRenderWindow, renderWindow);
  renderWindow->AddRenderer(renderer);
  renderWindow->SetSize(256, 256);

  SVTK_CREATE(svtkRenderWindowInteractor, irenderWindow);
  irenderWindow->SetRenderWindow(renderWindow);

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindow->Render();
    irenderWindow->Start();
    retVal = svtkRegressionTester::PASSED;
  }

  /*
    SVTK_CREATE (svtkWindowToImageFilter, w2i);
    w2i->SetInput (renderWindow);

    SVTK_CREATE (svtkPNGWriter, img);
    img->SetInputConnection (w2i->GetOutputPort ());
    img->SetFileName ("TestMultiBlockExodusWrite.png");

    renderWindow->Render ();
    w2i->Modified ();
    img->Write ();
    return 1;
  */

  delete[] InputFile;

  return !retVal;
}
