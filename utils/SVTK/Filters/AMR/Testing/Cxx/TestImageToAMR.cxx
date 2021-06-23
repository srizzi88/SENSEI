/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageToAMR.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test svtkImageToAMR filter.

#include "svtkCellData.h"
#include "svtkPointData.h"

#include "svtkAMRBox.h"
#include "svtkDataObject.h"
#include "svtkIdFilter.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkImageToAMR.h"
#include "svtkNew.h"
#include "svtkOverlappingAMR.h"
#include "svtkRTAnalyticSource.h"
#include "svtkUniformGrid.h"
#include "svtkVector.h"

#include <vector>
#define SVTK_SUCCESS 0
#define SVTK_FAILURE 1

namespace
{
//
int ComputeNumCells(svtkOverlappingAMR* amr)
{
  int n(0);
  for (unsigned int level = 0; level < amr->GetNumberOfLevels(); level++)
  {
    int numDataSets = amr->GetNumberOfDataSets(level);
    for (int i = 0; i < numDataSets; i++)
    {
      svtkUniformGrid* grid = amr->GetDataSet(level, i);
      int numCells = grid->GetNumberOfCells();
      for (int cellId = 0; cellId < numCells; cellId++)
      {
        n += grid->IsCellVisible(cellId) ? 1 : 0;
      }
    }
  }
  return n;
}

svtkIdType FindCell(svtkImageData* image, double point[3])
{
  double pcoords[3];
  int subid = 0;
  return image->svtkImageData::FindCell(point, nullptr, -1, 0.1, subid, pcoords, nullptr);
}
};

int TestImageToAMR(int, char*[])
{
  svtkNew<svtkRTAnalyticSource> imageSource;
  imageSource->SetWholeExtent(0, 0, -128, 128, -128, 128);

  svtkNew<svtkIdFilter> idFilter;
  idFilter->SetInputConnection(imageSource->GetOutputPort());

  svtkNew<svtkImageToAMR> amrConverter;
  amrConverter->SetInputConnection(idFilter->GetOutputPort());
  amrConverter->SetNumberOfLevels(4);

  std::vector<svtkVector3d> samples;
  for (int i = -118; i < 122; i += 10)
  {
    samples.push_back(svtkVector3d(0.0, (double)i, (double)i));
  }

  for (unsigned int numLevels = 1; numLevels <= 4; numLevels++)
  {
    for (int maxBlocks = 10; maxBlocks <= 50; maxBlocks += 10)
    {
      amrConverter->SetNumberOfLevels(numLevels);
      amrConverter->SetMaximumNumberOfBlocks(maxBlocks);
      amrConverter->Update();
      svtkImageData* image = svtkImageData::SafeDownCast(idFilter->GetOutputDataObject(0));
      svtkOverlappingAMR* amr =
        svtkOverlappingAMR::SafeDownCast(amrConverter->GetOutputDataObject(0));
      amr->Audit();
      // cout<<amr->GetTotalNumberOfBlocks()<<" "<<maxBlocks<<endl;
      if (amr->GetNumberOfLevels() != numLevels)
      {
        return SVTK_FAILURE;
      }
      if (maxBlocks < static_cast<int>(amr->GetTotalNumberOfBlocks()))
      {
        return SVTK_FAILURE;
      }
      if (ComputeNumCells(amr) != image->GetNumberOfCells())
      {
        return SVTK_FAILURE;
      }

      svtkIdTypeArray* cd =
        svtkArrayDownCast<svtkIdTypeArray>(image->GetCellData()->GetArray("svtkIdFilter_Ids"));
      assert(cd);
      for (std::vector<svtkVector3d>::iterator itr = samples.begin(); itr != samples.end(); ++itr)
      {
        double* x = (*itr).GetData();
        svtkIdType cellId = FindCell(image, x);
        svtkIdType value = cd->GetValue(cellId);
        assert(cellId == value);

        unsigned int level, id;
        if (amr->FindGrid(x, level, id))
        {
          svtkUniformGrid* grid = amr->GetDataSet(level, id);
          svtkIdTypeArray* cd1 =
            svtkArrayDownCast<svtkIdTypeArray>(grid->GetCellData()->GetArray("svtkIdFilter_Ids"));
          svtkIdType cellId1 = FindCell(grid, x);
          svtkIdType value1 = cd1->GetValue(cellId1);
          if (value1 != value)
          {
            return SVTK_FAILURE;
          }
        }
        else
        {
          return SVTK_FAILURE;
        }
      }
    }
  }

  return SVTK_SUCCESS;
}
