// Author: Andrew J. P. Maclean
#include <svtkParticleReader.h>
#include <svtkSmartPointer.h>
#include <svtkXMLPolyDataWriter.h>

int main(int argc, char* argv[])
{
  if (argc != 3)
  {
    cerr << "Usage: " << argv[0] << " InputFile(csv) OutputFile(vtp)." << endl;
    return EXIT_FAILURE;
  }

  std::string inputFileName = argv[1];
  std::string outputFileName = argv[2];

  svtkSmartPointer<svtkParticleReader> reader = svtkSmartPointer<svtkParticleReader>::New();
  reader->SetFileName(inputFileName.c_str());
  reader->Update();

  svtkSmartPointer<svtkXMLPolyDataWriter> writer = svtkSmartPointer<svtkXMLPolyDataWriter>::New();
  writer->SetInputConnection(reader->GetOutputPort());
  writer->SetFileName(outputFileName.c_str());
  writer->Write();

  return EXIT_SUCCESS;
}
