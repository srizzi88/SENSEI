#include "svtkNew.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"
#include "svtkXMLPolyDataWriter.h"

int TestXMLToString(int /*argc*/, char* /*argv*/[])
{
  svtkNew<svtkSphereSource> sphere;
  svtkNew<svtkXMLPolyDataWriter> writer;

  writer->WriteToOutputStringOn();

  writer->SetInputConnection(0, sphere->GetOutputPort(0));

  writer->Update();
  writer->Write();

  writer->GetOutputString();

  return 0;
}
