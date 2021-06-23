#include "svtkAMRGaussianPulseSource.h"
#include "svtkNew.h"
#include "svtkNonOverlappingAMR.h"
#include "svtkOverlappingAMR.h"
#include "svtkStructuredData.h"
#include "svtkTestUtilities.h"
#include "svtkXMLGenericDataObjectReader.h"
#include "svtkXMLUniformGridAMRReader.h"
#include "svtkXMLUniformGridAMRWriter.h"

#include <string>

namespace
{
#define svtk_assert(x)                                                                              \
  if (!(x))                                                                                        \
  {                                                                                                \
    cerr << "ERROR: Condition FAILED!! : " << #x << endl;                                          \
    return false;                                                                                  \
  }

bool Validate(svtkOverlappingAMR* input, svtkOverlappingAMR* result)
{
  svtk_assert(input->GetNumberOfLevels() == result->GetNumberOfLevels());
  svtk_assert(input->GetOrigin()[0] == result->GetOrigin()[0]);
  svtk_assert(input->GetOrigin()[1] == result->GetOrigin()[1]);
  svtk_assert(input->GetOrigin()[2] == result->GetOrigin()[2]);

  for (unsigned int level = 0; level < input->GetNumberOfLevels(); level++)
  {
    svtk_assert(input->GetNumberOfDataSets(level) == result->GetNumberOfDataSets(level));
  }

  cout << "Audit Input" << endl;
  input->Audit();
  cout << "Audit Output" << endl;
  result->Audit();
  return true;
}

bool TestAMRXMLIO_OverlappingAMR2D(const std::string& output_dir)
{
  svtkNew<svtkAMRGaussianPulseSource> pulse;
  pulse->SetDimension(2);
  pulse->SetRootSpacing(5);

  std::string filename = output_dir + "/TestAMRXMLIO_OverlappingAMR2D.vth";

  svtkNew<svtkXMLUniformGridAMRWriter> writer;
  writer->SetInputConnection(pulse->GetOutputPort());
  writer->SetFileName(filename.c_str());
  writer->Write();

  svtkNew<svtkXMLGenericDataObjectReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  return Validate(svtkOverlappingAMR::SafeDownCast(pulse->GetOutputDataObject(0)),
    svtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0)));
}

bool TestAMRXMLIO_OverlappingAMR3D(const std::string& output_dir)
{
  svtkNew<svtkAMRGaussianPulseSource> pulse;
  pulse->SetDimension(3);
  pulse->SetRootSpacing(13);

  std::string filename = output_dir + "/TestAMRXMLIO_OverlappingAMR3D.vth";

  svtkNew<svtkXMLUniformGridAMRWriter> writer;
  writer->SetInputConnection(pulse->GetOutputPort());
  writer->SetFileName(filename.c_str());
  writer->Write();

  svtkNew<svtkXMLGenericDataObjectReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  return Validate(svtkOverlappingAMR::SafeDownCast(pulse->GetOutputDataObject(0)),
    svtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0)));
}

bool TestAMRXMLIO_HierarchicalBox(const std::string& input_dir, const std::string& output_dir)
{
  std::string filename = input_dir + "/AMR/HierarchicalBoxDataset.v1.1.vthb";
  // for svtkHierarchicalBoxDataSet, svtkXMLGenericDataObjectReader creates the
  // legacy reader by default. For version 1.1, we should use the
  // svtkXMLUniformGridAMRReader explicitly. svtkHierarchicalBoxDataSet itself is
  // obsolete.
  svtkNew<svtkXMLUniformGridAMRReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  svtkOverlappingAMR* output = svtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));
  svtk_assert(output->GetNumberOfLevels() == 4);
  svtk_assert(output->GetNumberOfDataSets(0) == 1);
  svtk_assert(output->GetNumberOfDataSets(1) == 8);
  svtk_assert(output->GetNumberOfDataSets(2) == 40);
  svtk_assert(output->GetNumberOfDataSets(3) == 32);
  svtk_assert(output->GetGridDescription() == SVTK_XYZ_GRID);
  output->Audit();

  filename = output_dir + "/TestAMRXMLIO_HierarchicalBox.vth";
  svtkNew<svtkXMLUniformGridAMRWriter> writer;
  writer->SetFileName(filename.c_str());
  writer->SetInputDataObject(output);
  writer->Write();

  svtkNew<svtkXMLUniformGridAMRReader> reader2;
  reader2->SetFileName(filename.c_str());
  reader2->Update();
  return Validate(output, svtkOverlappingAMR::SafeDownCast(reader2->GetOutputDataObject(0)));
}
}

#define SVTK_SUCCESS 0
#define SVTK_FAILURE 1
int TestAMRXMLIO(int argc, char* argv[])
{
  char* temp_dir =
    svtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "SVTK_TEMP_DIR", "Testing/Temporary");
  if (!temp_dir)
  {
    cerr << "Could not determine temporary directory." << endl;
    return SVTK_FAILURE;
  }

  std::string output_dir = temp_dir;
  delete[] temp_dir;

  cout << "Test Overlapping AMR (2D)" << endl;
  if (!TestAMRXMLIO_OverlappingAMR2D(output_dir))
  {
    return SVTK_FAILURE;
  }

  cout << "Test Overlapping AMR (3D)" << endl;
  if (!TestAMRXMLIO_OverlappingAMR3D(output_dir))
  {
    return SVTK_FAILURE;
  }

  char* data_dir = svtkTestUtilities::GetDataRoot(argc, argv);
  if (!data_dir)
  {
    cerr << "Could not determine data directory." << endl;
    return SVTK_FAILURE;
  }

  std::string input_dir = data_dir;
  input_dir += "/Data";
  delete[] data_dir;

  cout << "Test HierarchicalBox AMR (v1.1)" << endl;
  if (!TestAMRXMLIO_HierarchicalBox(input_dir, output_dir))
  {
    return SVTK_FAILURE;
  }

  return SVTK_SUCCESS;
}
