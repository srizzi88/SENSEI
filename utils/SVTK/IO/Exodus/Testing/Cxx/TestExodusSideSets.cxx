#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkExodusIIReader.h"
#include "svtkIdTypeArray.h"
#include "svtkIntArray.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkTestUtilities.h"

int TestExodusSideSets(int argc, char* argv[])
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

  rdr->GenerateGlobalNodeIdArrayOn();
  rdr->GenerateGlobalElementIdArrayOn();
  rdr->UpdateInformation();

  for (int i = 0; i < rdr->GetNumberOfObjects(svtkExodusIIReader::ELEM_BLOCK); i++)
  {
    rdr->SetObjectStatus(svtkExodusIIReader::ELEM_BLOCK, i, 0);
  }

  for (int i = 0; i < rdr->GetNumberOfObjects(svtkExodusIIReader::SIDE_SET); i++)
  {
    rdr->SetObjectStatus(svtkExodusIIReader::SIDE_SET, i, 1);
  }

  rdr->Update();

  svtkMultiBlockDataSet* mb = svtkMultiBlockDataSet::SafeDownCast(rdr->GetOutput());
  svtkCellData* cd =
    svtkDataSet::SafeDownCast(svtkMultiBlockDataSet::SafeDownCast(mb->GetBlock(4))->GetBlock(0))
      ->GetCellData();

  if (cd == nullptr)
  {
    cerr << "Can't find proper data set\n";
    return 1;
  }

  svtkIdTypeArray* sourceelementids = svtkArrayDownCast<svtkIdTypeArray>(
    cd->GetArray(svtkExodusIIReader::GetSideSetSourceElementIdArrayName()));

  svtkIntArray* sourceelementsides = svtkArrayDownCast<svtkIntArray>(
    cd->GetArray(svtkExodusIIReader::GetSideSetSourceElementSideArrayName()));

  if (!sourceelementsides || !sourceelementids)
  {
    cerr << "Can't find proper cell data arrays\n";
    return 1;
  }
  else
  {
    if (sourceelementids->GetNumberOfTuples() != 5)
    {
      cerr << "Wrong number of cell array tuples\n";
      return 1;
    }
    // correct values
    svtkIdType ids[] = { 0, 0, 0, 1, 1 };
    int sides[] = { 2, 3, 4, 1, 0 };

    for (svtkIdType i = 0; i < sourceelementids->GetNumberOfTuples(); i++)
    {
      if (sourceelementids->GetValue(i) != ids[i])
      {
        cerr << "Source element id is wrong\n";
        return 1;
      }
      if (sourceelementsides->GetValue(i) != sides[i])
      {
        cerr << "Source element side is wrong\n";
        return 1;
      }
    }
  }

  return 0;
}
