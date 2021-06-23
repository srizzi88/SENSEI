/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestResampleToImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This is just a simple test. svtkResampleToImage internally uses
// svtkProbeFilter, which is tested thoroughly in other tests.

#include "svtkResampleToImage.h"

#include "svtkClipDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkExtractVOI.h"
#include "svtkImageData.h"
#include "svtkNew.h"
#include "svtkRTAnalyticSource.h"
#include "svtkResampleToImage.h"
#include "svtkUnsignedCharArray.h"

#include "svtkCell.h"
#include "svtkCellType.h"
#include "svtkUnstructuredGrid.h"
#include <iostream>

int TestResampleToImage(int, char*[])
{
  // Create Pipeline
  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(0, 16, 0, 16, 0, 16);
  wavelet->SetCenter(8, 8, 8);

  svtkNew<svtkClipDataSet> clip;
  clip->SetInputConnection(wavelet->GetOutputPort());
  clip->SetValue(157);

  svtkNew<svtkResampleToImage> resample;
  resample->SetUseInputBounds(true);
  resample->SetSamplingDimensions(32, 32, 32);
  resample->SetInputConnection(clip->GetOutputPort());

  svtkNew<svtkExtractVOI> voi;
  voi->SetVOI(4, 27, 4, 27, 4, 27);
  voi->SetInputConnection(resample->GetOutputPort());
  voi->Update();

  svtkImageData* output = voi->GetOutput();
  svtkIdType numPoints = output->GetNumberOfPoints();
  svtkIdType numCells = output->GetNumberOfCells();
  if (numPoints != 13824 || numCells != 12167)
  {
    std::cout << "Number of points: expecting 13824, got " << numPoints << std::endl;
    std::cout << "Number of cells: expecting 12167, got " << numCells << std::endl;
    return 1;
  }

  svtkUnsignedCharArray* pointGhostArray = output->GetPointGhostArray();
  svtkIdType numHiddenPoints = 0;
  for (svtkIdType i = 0; i < numPoints; ++i)
  {
    if (pointGhostArray->GetValue(i) & svtkDataSetAttributes::HIDDENPOINT)
    {
      ++numHiddenPoints;
    }
  }

  if (numHiddenPoints != 2000)
  {
    std::cout << "Number of Hidden points: expecting 2000 got " << numHiddenPoints << std::endl;
    return 1;
  }

  svtkUnsignedCharArray* cellGhostArray = output->GetCellGhostArray();
  svtkIdType numHiddenCells = 0;
  for (svtkIdType i = 0; i < numCells; ++i)
  {
    if (cellGhostArray->GetValue(i) & svtkDataSetAttributes::HIDDENCELL)
    {
      ++numHiddenCells;
    }
  }

  if (numHiddenCells != 2171)
  {
    std::cout << "Number of Hidden cells: expecting 2171 got " << numHiddenCells << std::endl;
    return 1;
  }

  return 0;
}
