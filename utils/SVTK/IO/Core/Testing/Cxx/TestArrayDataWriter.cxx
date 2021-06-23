
#include "svtkArrayData.h"
#include "svtkArrayDataReader.h"
#include "svtkArrayDataWriter.h"
#include "svtkDenseArray.h"
#include "svtkNew.h"
#include "svtkSparseArray.h"
#include "svtkStdString.h"

#include <iostream>

int TestArrayDataWriter(int, char*[])
{
  std::cerr << "Testing dense first..." << std::endl;

  svtkNew<svtkDenseArray<double> > da;
  da->Resize(10, 10);
  da->SetName("dense");
  svtkNew<svtkSparseArray<double> > sa;
  sa->Resize(10, 10);
  sa->SetName("sparse");
  for (int i = 0; i < 10; ++i)
  {
    sa->SetValue(i, 0, i);
    for (int j = 0; j < 10; ++j)
    {
      da->SetValue(i, j, i * j);
    }
  }

  svtkNew<svtkArrayData> d;
  d->AddArray(da);
  d->AddArray(sa);

  svtkNew<svtkArrayDataWriter> w;
  w->SetInputData(d);
  w->WriteToOutputStringOn();
  w->Write();
  svtkStdString s = w->GetOutputString();

  svtkNew<svtkArrayDataReader> r;
  r->ReadFromInputStringOn();
  r->SetInputString(s);
  r->Update();
  svtkArrayData* d_out = r->GetOutput();

  if (d_out->GetNumberOfArrays() != 2)
  {
    std::cerr << "Wrong number of arrays" << std::endl;
    return 1;
  }

  svtkArray* da_out = d_out->GetArray(0);
  if (da_out->GetDimensions() != 2)
  {
    std::cerr << "da wrong number of dimensions" << std::endl;
    return 1;
  }
  if (!da_out->IsDense())
  {
    std::cerr << "da wrong array type" << std::endl;
    return 1;
  }
  if (da_out->GetSize() != 100)
  {
    std::cerr << "da wrong array size" << std::endl;
    return 1;
  }

  svtkArray* sa_out = d_out->GetArray(1);
  if (sa_out->GetDimensions() != 2)
  {
    std::cerr << "sa wrong number of dimensions" << std::endl;
    return 1;
  }
  if (sa_out->IsDense())
  {
    std::cerr << "sa wrong array type" << std::endl;
    return 1;
  }
  if (sa_out->GetSize() != 100)
  {
    std::cerr << "sa wrong array size" << std::endl;
    return 1;
  }

  std::cerr << "Testing sparse first..." << std::endl;
  d->ClearArrays();
  d->AddArray(sa);
  d->AddArray(da);
  w->Update();
  s = w->GetOutputString();
  r->SetInputString(s);
  r->Update();
  d_out = r->GetOutput();
  if (d_out->GetNumberOfArrays() != 2)
  {
    std::cerr << "Wrong number of arrays" << std::endl;
    return 1;
  }

  std::cerr << "Testing binary writer..." << std::endl;
  w->BinaryOn();
  w->Update();
  s = w->GetOutputString();
  r->SetInputString(s);
  r->Update();
  d_out = r->GetOutput();
  if (d_out->GetNumberOfArrays() != 2)
  {
    std::cerr << "Wrong number of arrays" << std::endl;
    return 1;
  }

  return 0;
}
