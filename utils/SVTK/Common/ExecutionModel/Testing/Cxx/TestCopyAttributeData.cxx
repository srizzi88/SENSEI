/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCopyAttributeData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test the svtkImageAlgorithm::CopyAttributeData() method, which copies
// all of the attribute data arrays (PointData and CellData) that is
// not usually handled by the Execute methods of the imaging filters
// (Execute methods typically process only the PointData Scalars).

#include "svtkCellData.h"
#include "svtkFloatArray.h"
#include "svtkImageAlgorithm.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStringArray.h"
#include "svtkVariant.h"

#include <cmath>
#include <cstring>

// Make a dummy image filter that does nothing but call CopyAttributeData.
class svtkDummyImageFilter : public svtkImageAlgorithm
{
public:
  static svtkDummyImageFilter* New();
  svtkTypeMacro(svtkDummyImageFilter, svtkImageAlgorithm);

protected:
  svtkDummyImageFilter() = default;
  ~svtkDummyImageFilter() override = default;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkDummyImageFilter(const svtkDummyImageFilter&) = delete;
  void operator=(const svtkDummyImageFilter&) = delete;
};

svtkStandardNewMacro(svtkDummyImageFilter);

int svtkDummyImageFilter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkImageData* inData = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkImageData* outData = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  int extent[6];
  outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent);
  this->AllocateOutputData(outData, outInfo, extent);

  // this is what we are testing
  this->CopyAttributeData(inData, outData, inputVector);

  // scalars would usually be processed here, but this is a dummy filter

  return 1;
}

int TestCopyAttributeData(int, char*[])
{
  int extent[6] = { 0, 6, 0, 4, 0, 2 };
  int outExt[6] = { 0, 4, 2, 2, 0, 2 };

  svtkSmartPointer<svtkImageData> image = svtkSmartPointer<svtkImageData>::New();
  image->SetExtent(extent);
  image->AllocateScalars(SVTK_UNSIGNED_CHAR, 1);

  svtkIdType numPoints = image->GetNumberOfPoints();
  svtkIdType numCells = image->GetNumberOfCells();

  svtkSmartPointer<svtkFloatArray> pointVectors = svtkSmartPointer<svtkFloatArray>::New();
  pointVectors->SetName("ImageVectors");
  pointVectors->SetNumberOfComponents(3);
  pointVectors->SetNumberOfTuples(numPoints);

  for (svtkIdType i = 0; i < numPoints; i++)
  {
    double v[3];
    v[0] = sin(i * 0.5);
    v[1] = cos(i * 0.5);
    v[2] = sin(i * 0.1);
    pointVectors->SetTuple(i, v);
  }

  svtkSmartPointer<svtkIntArray> cellScalars = svtkSmartPointer<svtkIntArray>::New();
  cellScalars->SetName("CellScalars");
  cellScalars->SetNumberOfValues(numCells);

  svtkSmartPointer<svtkStringArray> cellStrings = svtkSmartPointer<svtkStringArray>::New();
  cellStrings->SetName("CellStrings");
  cellStrings->SetNumberOfValues(numCells);

  for (svtkIdType j = 0; j < numCells; j++)
  {
    svtkVariant val(j);
    cellScalars->SetValue(j, j);
    cellStrings->SetValue(j, val.ToString());
  }

  image->GetPointData()->SetVectors(pointVectors);
  image->GetCellData()->SetScalars(cellScalars);
  image->GetCellData()->AddArray(cellStrings);

  svtkSmartPointer<svtkDummyImageFilter> filter = svtkSmartPointer<svtkDummyImageFilter>::New();

  filter->SetInputData(image);

  for (int r = 0; r < 2; r++)
  {
    filter->UpdateExtent(outExt);

    svtkImageData* output = filter->GetOutput();

    svtkDataArray* outPointVectors = output->GetPointData()->GetVectors();
    svtkDataArray* outCellScalars = output->GetCellData()->GetScalars();
    svtkStringArray* outCellStrings =
      svtkArrayDownCast<svtkStringArray>(output->GetCellData()->GetAbstractArray("CellStrings"));

    for (int zId = outExt[4]; zId <= outExt[5]; zId++)
    {
      for (int yId = outExt[2]; yId <= outExt[3]; yId++)
      {
        for (int xId = outExt[0]; xId <= outExt[1]; xId++)
        {
          svtkIdType inIdx = (zId - extent[4]) * (extent[3] - extent[2] + 1);
          inIdx = (inIdx + yId - extent[2]) * (extent[1] - extent[0] + 1);
          inIdx = inIdx + xId - extent[0];
          svtkIdType outIdx = (zId - outExt[4]) * (outExt[3] - outExt[2] + 1);
          outIdx = (outIdx + yId - outExt[2]) * (outExt[1] - outExt[0] + 1);
          outIdx = outIdx + xId - outExt[0];
          double v1[3], v2[3];
          pointVectors->GetTuple(inIdx, v1);
          outPointVectors->GetTuple(outIdx, v2);
          if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2])
          {
            cerr << "point attribute value mismatch ";
            return 1;
          }
        }
      }
    }

    for (int zId = outExt[4]; zId < outExt[5]; zId++)
    {
      int ye = (outExt[2] == outExt[3]);
      for (int yId = outExt[2]; yId < outExt[3] + ye; yId++)
      {
        for (int xId = outExt[0]; xId < outExt[1]; xId++)
        {
          svtkIdType inIdx = (zId - extent[4]) * (extent[3] - extent[2]);
          inIdx = (inIdx + yId - extent[2]) * (extent[1] - extent[0]);
          inIdx = inIdx + xId - extent[0];
          svtkIdType outIdx = (zId - outExt[4]) * (outExt[3] - outExt[2] + ye);
          outIdx = (outIdx + yId - outExt[2]) * (outExt[1] - outExt[0]);
          outIdx = outIdx + xId - outExt[0];
          double s1, s2;
          cellScalars->GetTuple(inIdx, &s1);
          outCellScalars->GetTuple(outIdx, &s2);
          if (s1 != s2)
          {
            cerr << "cell attribute value mismatch\n";
            return 1;
          }
          if (cellStrings->GetValue(inIdx) != outCellStrings->GetValue(outIdx))
          {
            cerr << "cell attribute string mismatch\n";
            return 1;
          }
        }
      }
    }

    // try again with full extent to test pass data
    outExt[0] = extent[0];
    outExt[1] = extent[1];
    outExt[2] = extent[2];
    outExt[3] = extent[3];
    outExt[4] = extent[4];
    outExt[5] = extent[5];
  }

  return 0;
}
