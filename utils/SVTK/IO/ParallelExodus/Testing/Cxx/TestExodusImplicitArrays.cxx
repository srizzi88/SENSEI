#include "svtkAbstractArray.h"
#include "svtkDataObject.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkExodusIIReader.h"
#include "svtkIdTypeArray.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkPExodusIIReader.h"
#include "svtkSmartPointer.h"

#include <svtkTestUtilities.h>
#include <svtkTesting.h>

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New();

int TestExodusImplicitArrays(int argc, char* argv[])
{
  const char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/disk_out_ref.ex2");
  SVTK_CREATE(svtkExodusIIReader, reader);
  reader->SetFileName(fname);
  reader->GenerateImplicitElementIdArrayOn();
  reader->GenerateImplicitNodeIdArrayOn();
  reader->Update();

  delete[] fname;

  svtkMultiBlockDataSet* mb = reader->GetOutput();

  if (!mb)
  {
    return 1;
  }
  svtkMultiBlockDataSet* elems = svtkMultiBlockDataSet::SafeDownCast(mb->GetBlock(0));

  if (!elems)
  {
    return 1;
  }
  svtkDataObject* obj = elems->GetBlock(0);

  if (!obj)
  {
    return 1;
  }
  svtkIdTypeArray* ie = svtkArrayDownCast<svtkIdTypeArray>(
    obj->GetAttributes(svtkDataSet::CELL)->GetAbstractArray("ImplicitElementId"));
  svtkIdTypeArray* in = svtkArrayDownCast<svtkIdTypeArray>(
    obj->GetAttributes(svtkDataSet::POINT)->GetAbstractArray("ImplicitNodeId"));
  if (!ie || !in)
  {
    return 1;
  }

  for (int id = 0; id < ie->GetNumberOfTuples(); id++)
  {
    if (ie->GetValue(id) != (id + 1))
    {
      return 1;
    }
  }

  if (in->GetValue(0) != 143 || in->GetValue(1) != 706 || in->GetValue(2) != 3173)
  {
    return 1;
  }

  return 0;
}
