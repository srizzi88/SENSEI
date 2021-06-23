#include "svtkBiomTableReader.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"
#include "svtkTestUtilities.h"

int TestBiomTableReader(int argc, char* argv[])
{
  char* file = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/otu_table.biom");

  cerr << "file: " << file << endl;

  svtkSmartPointer<svtkBiomTableReader> reader = svtkSmartPointer<svtkBiomTableReader>::New();
  reader->SetFileName(file);
  delete[] file;
  reader->Update();
  svtkTable* table = reader->GetOutput();

  int error_count = 0;

  if (table->GetNumberOfRows() != 419)
  {
    ++error_count;
  }

  if (table->GetNumberOfColumns() != 10)
  {
    ++error_count;
  }

  cerr << error_count << " errors" << endl;
  return error_count;
}
