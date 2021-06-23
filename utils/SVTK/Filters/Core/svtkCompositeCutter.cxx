/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCompositeCutter.h"

#include "svtkAppendPolyData.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkCompositeDataSet.h"
#include "svtkCompositeDataSetRange.h"
#include "svtkDataSet.h"
#include "svtkImplicitFunction.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkPlane.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include <cassert>
#include <cmath>

svtkStandardNewMacro(svtkCompositeCutter);

#ifdef DEBUGME
#define PRINT(x)                                                                                   \
  {                                                                                                \
    cout << x << endl;                                                                             \
  }
#else
#define PRINT(x)
#endif
namespace
{
inline double Sign(double a)
{
  return a == 0.0 ? 0.0 : (a < 0.0 ? -1.0 : 1.0);
}
inline bool IntersectBox(svtkImplicitFunction* func, double bounds[6], double value)
{
  double fVal[8];
  fVal[0] = func->EvaluateFunction(bounds[0], bounds[2], bounds[4]);
  fVal[1] = func->EvaluateFunction(bounds[0], bounds[2], bounds[5]);
  fVal[2] = func->EvaluateFunction(bounds[0], bounds[3], bounds[4]);
  fVal[3] = func->EvaluateFunction(bounds[0], bounds[3], bounds[5]);
  fVal[4] = func->EvaluateFunction(bounds[1], bounds[2], bounds[4]);
  fVal[5] = func->EvaluateFunction(bounds[1], bounds[2], bounds[5]);
  fVal[6] = func->EvaluateFunction(bounds[1], bounds[3], bounds[4]);
  fVal[7] = func->EvaluateFunction(bounds[1], bounds[3], bounds[5]);

  double sign0 = Sign(fVal[0] - value);
  for (int i = 1; i < 8; i++)
  {
    if (Sign(fVal[i] - value) != sign0)
    {
      // this corner is on different side than first, piece
      // intersects and cannot be rejected
      return true;
    }
  }
  return false;
}
};

svtkCompositeCutter::svtkCompositeCutter(svtkImplicitFunction* cf)
  : svtkCutter(cf)
{
}

//----------------------------------------------------------------------------
svtkCompositeCutter::~svtkCompositeCutter() = default;

int svtkCompositeCutter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}

int svtkCompositeCutter::RequestUpdateExtent(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector*)
{
  svtkDebugMacro("Request-Update");

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  for (svtkIdType c = 0; c < this->ContourValues->GetNumberOfContours(); c++)
  {
    svtkDebugMacro("Contours " << this->ContourValues->GetValue(c));
  }

  // Check if metadata are passed downstream
  if (inInfo->Has(svtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()))
  {
    std::vector<int> intersected;

    svtkCompositeDataSet* meta = svtkCompositeDataSet::SafeDownCast(
      inInfo->Get(svtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()));

    for (auto node : svtk::Range(meta))
    {
      svtkInformation* info = node.GetMetaData();
      double* bb = info->Get(svtkDataObject::BOUNDING_BOX());
      for (svtkIdType c = 0; c < this->ContourValues->GetNumberOfContours(); c++)
      {
        if (IntersectBox(this->GetCutFunction(), bb, this->ContourValues->GetValue(c)))
        {
          intersected.push_back(static_cast<int>(node.GetFlatIndex()));
          break;
        }
      }
    }
    PRINT("Cutter demand " << intersected.size() << " blocks");
    inInfo->Set(svtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(), &intersected[0],
      static_cast<int>(intersected.size()));
  }
  return 1;
}

int svtkCompositeCutter::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkSmartPointer<svtkCompositeDataSet> inData =
    svtkCompositeDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!inData)
  {
    return Superclass::RequestData(request, inputVector, outputVector);
  }

  svtkNew<svtkAppendPolyData> append;
  int numObjects(0);

  using Opts = svtk::CompositeDataSetOptions;
  for (svtkDataObject* dObj : svtk::Range(inData, Opts::SkipEmptyNodes))
  {
    svtkDataSet* data = svtkDataSet::SafeDownCast(dObj);
    assert(data);
    inInfo->Set(svtkDataObject::DATA_OBJECT(), data);
    svtkNew<svtkPolyData> out;
    outInfo->Set(svtkDataObject::DATA_OBJECT(), out);
    this->Superclass::RequestData(request, inputVector, outputVector);
    append->AddInputData(out);
    numObjects++;
  }
  append->Update();

  svtkPolyData* appoutput = append->GetOutput();
  inInfo->Set(svtkDataObject::DATA_OBJECT(), inData); // set it back to the original input
  outInfo->Set(svtkDataObject::DATA_OBJECT(), appoutput);
  return 1;
}

void svtkCompositeCutter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
