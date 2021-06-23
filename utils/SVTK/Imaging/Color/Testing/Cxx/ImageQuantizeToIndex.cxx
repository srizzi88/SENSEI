#include <array>
#include <set>

#include "svtkImageData.h"
#include "svtkImageQuantizeRGBToIndex.h"
#include "svtkLookupTable.h"
#include "svtkSmartPointer.h"
#include "svtkTIFFReader.h"

#include "svtkTestUtilities.h"

#include <cmath>

int ImageQuantizeToIndex(int argc, char* argv[])
{
  char* fname =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/libtiff/gourds_tiled_200x300.tif");
  // \note first I used earth.ppm, but the test failed because
  // the lookup table contains duplicate colors (the non-sorted and sorted are identical).

  auto reader = svtkSmartPointer<svtkTIFFReader>::New();
  reader->SetFileName(fname);
  delete[] fname;

  reader->Update();

  auto filter = svtkSmartPointer<svtkImageQuantizeRGBToIndex>::New();
  filter->SetInputConnection(reader->GetOutputPort());
  filter->SetNumberOfColors(16);
  filter->SetSortIndexByLuminance(false);
  filter->Update();
  auto lut = filter->GetLookupTable();

  auto filter2 = svtkSmartPointer<svtkImageQuantizeRGBToIndex>::New();
  filter2->SetInputConnection(reader->GetOutputPort());
  filter2->SetNumberOfColors(16);
  filter2->SetSortIndexByLuminance(true);
  filter2->Update();
  auto lut2 = filter2->GetLookupTable();

  bool retVal = lut->GetNumberOfColors() == lut2->GetNumberOfColors();
  retVal = retVal && lut->GetNumberOfColors() == 16;
  if (retVal)
  {
    // SortIndexByLuminance should produce the same colors, just at a different index
    std::array<int, 16> mapping;
    double rgba[4];
    double rgba2[4];

    for (int i = 0; i < 16; i++)
    {
      lut->GetTableValue(i, rgba);
      mapping[i] = 0;
      double best = SVTK_DOUBLE_MAX;

      for (int j = 0; j < 16; j++)
      {
        lut2->GetTableValue(j, rgba2);
        double dist = std::pow(rgba[0] - rgba2[0], 2.) + std::pow(rgba[1] - rgba2[1], 2.) +
          std::pow(rgba[2] - rgba2[2], 2.);

        if (dist < best)
        {
          best = dist;
          mapping[i] = j;
        }
      }
    }
    // check mapping is correct, i.e. one-to-one correspondence
    std::set<int> unique_mapped_values(mapping.begin(), mapping.end());
    retVal = unique_mapped_values.size() == 16;
    std::cerr << "mapping = " << unique_mapped_values.size() << std::endl;

    if (retVal)
    {
      const auto data = static_cast<unsigned short*>(filter->GetOutput()->GetScalarPointer());
      const auto data2 = static_cast<unsigned short*>(filter2->GetOutput()->GetScalarPointer());
      for (svtkIdType i = 0; i < filter->GetOutput()->GetNumberOfPoints(); i++)
      {
        retVal = retVal && (mapping.at(data[i]) == data2[i]);
      }
    }
  }

  return !retVal;
}
