#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkExodusIIReader.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkTestUtilities.h"

int TestExodusAttributes(int argc, char* argv[])
{
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/edgeFaceElem.exii");
  if (!fname)
  {
    cout << "Could not obtain filename for test data.\n";
    return 1;
  }

  svtkNew<svtkExodusIIReader> rdr;
  if (!rdr->CanReadFile(fname))
  {
    cout << "Cannot read \"" << fname << "\"\n";
    return 1;
  }
  rdr->SetFileName(fname);
  delete[] fname;

  rdr->UpdateInformation();
  rdr->SetObjectAttributeStatus(svtkExodusIIReader::ELEM_BLOCK, 0, "SPAGHETTI", 1);
  rdr->SetObjectAttributeStatus(svtkExodusIIReader::ELEM_BLOCK, 0, "WESTERN", 1);
  rdr->Update();
  svtkCellData* cd = svtkDataSet::SafeDownCast(
    svtkMultiBlockDataSet::SafeDownCast(
      svtkMultiBlockDataSet::SafeDownCast(rdr->GetOutputDataObject(0))->GetBlock(0))
      ->GetBlock(0))
                      ->GetCellData();
  if (!cd)
  {
    cout << "Could not obtain cell data\n";
    return 1;
  }
  int na = cd->GetNumberOfArrays();
  for (int i = 0; i < na; ++i)
  {
    svtkDataArray* arr = cd->GetArray(i);
    cout << "Cell array " << i << " \"" << arr->GetName() << "\"\n";
    for (int j = 0; j <= arr->GetMaxId(); ++j)
    {
      cout << " " << arr->GetTuple1(j) << "\n";
    }
  }
  svtkDataArray* spaghetti = cd->GetArray("SPAGHETTI");
  svtkDataArray* western = cd->GetArray("WESTERN");
  if (!spaghetti || !western || spaghetti->GetNumberOfTuples() != 2 ||
    western->GetNumberOfTuples() != 2)
  {
    cout << "Attribute arrays not read or are wrong length.\n";
    return 1;
  }
  if (spaghetti->GetTuple1(0) != 127. || spaghetti->GetTuple1(1) != 137)
  {
    cout << "Bad spaghetti\n";
    return 1;
  }
  if (western->GetTuple1(0) != 101. || western->GetTuple1(1) != 139)
  {
    cout << "Wrong western\n";
    return 1;
  }
  return 0;
}
