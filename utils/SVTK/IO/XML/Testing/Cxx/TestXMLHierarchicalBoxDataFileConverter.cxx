#include "svtkNew.h"
#include "svtkOverlappingAMR.h"
#include "svtkTestUtilities.h"
#include "svtkXMLGenericDataObjectReader.h"
#include "svtkXMLHierarchicalBoxDataFileConverter.h"

#include <string>
#include <svtksys/SystemTools.hxx>

#define SVTK_SUCCESS 0
#define SVTK_FAILURE 1
int TestXMLHierarchicalBoxDataFileConverter(int argc, char* argv[])
{
  char* temp_dir =
    svtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "SVTK_TEMP_DIR", "Testing/Temporary");
  if (!temp_dir)
  {
    cerr << "Could not determine temporary directory." << endl;
    return SVTK_FAILURE;
  }

  char* data_dir = svtkTestUtilities::GetDataRoot(argc, argv);
  if (!data_dir)
  {
    delete[] temp_dir;
    cerr << "Could not determine data directory." << endl;
    return SVTK_FAILURE;
  }

  std::string input = data_dir;
  input += "/Data/AMR/HierarchicalBoxDataset.v1.0.vthb";

  std::string output = temp_dir;
  output += "/HierarchicalBoxDataset.Converted.v1.1.vthb";

  svtkNew<svtkXMLHierarchicalBoxDataFileConverter> converter;
  converter->SetInputFileName(input.c_str());
  converter->SetOutputFileName(output.c_str());

  if (!converter->Convert())
  {
    delete[] temp_dir;
    delete[] data_dir;
    return SVTK_FAILURE;
  }

  // Copy the subfiles over to the temporary directory so that we can test
  // loading the written file.
  std::string input_dir = data_dir;
  input_dir += "/Data/AMR/HierarchicalBoxDataset.v1.0";

  std::string output_dir = temp_dir;
  output_dir += "/HierarchicalBoxDataset.Converted.v1.1";

  svtksys::SystemTools::RemoveADirectory(output_dir);
  if (!svtksys::SystemTools::CopyADirectory(input_dir, output_dir))
  {
    delete[] temp_dir;
    delete[] data_dir;
    cerr << "Failed to copy image data files over for testing." << endl;
    return SVTK_FAILURE;
  }

  svtkNew<svtkXMLGenericDataObjectReader> reader;
  reader->SetFileName(output.c_str());
  reader->Update();
  svtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0))->Audit();

  delete[] temp_dir;
  delete[] data_dir;

  return SVTK_SUCCESS;
}
