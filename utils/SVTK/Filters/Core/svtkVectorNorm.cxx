/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVectorNorm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkVectorNorm.h"

#include "svtkArrayDispatch.h"
#include "svtkCellData.h"
#include "svtkDataArrayRange.h"
#include "svtkDataSet.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSMPTools.h"

#include <cmath>

svtkStandardNewMacro(svtkVectorNorm);

namespace
{
// The heart of the algorithm plus interface to the SMP tools. Double templated
// over point and scalar types.
template <class TV>
struct svtkVectorNormAlgorithm
{
  TV* Vectors = nullptr;
  float* Scalars = nullptr;
};
// Interface dot product computation to SMP tools.
template <class T>
struct NormOp
{
  svtkVectorNormAlgorithm<T>* Algo;
  svtkSMPThreadLocal<double> Max;
  NormOp(svtkVectorNormAlgorithm<T>* algo)
    : Algo(algo)
    , Max(SVTK_DOUBLE_MIN)
  {
  }
  void operator()(svtkIdType k, svtkIdType end)
  {
    using ValueType = svtk::GetAPIType<T>;

    double& max = this->Max.Local();
    auto vectorRange = svtk::DataArrayTupleRange<3>(this->Algo->Vectors, k, end);
    float* s = this->Algo->Scalars + k;
    for (auto v : vectorRange)
    {
      const ValueType mag = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
      *s = static_cast<float>(sqrt(static_cast<double>(mag)));
      max = (*s > max ? *s : max);
      s++;
    }
  }
};

struct svtkVectorNormDispatch // Interface between SVTK and templated functions.
{
  template <typename ArrayT>
  void operator()(ArrayT* vectors, bool normalize, svtkIdType num, float* scalars) const
  {

    // Populate data into local storage
    svtkVectorNormAlgorithm<ArrayT> algo;

    algo.Vectors = vectors;
    algo.Scalars = scalars;

    // Okay now generate samples using SMP tools
    NormOp<ArrayT> norm(&algo);
    svtkSMPTools::For(0, num, norm);

    // Have to roll up the thread local storage and get the overall range
    double max = SVTK_DOUBLE_MIN;
    svtkSMPThreadLocal<double>::iterator itr;
    for (itr = norm.Max.begin(); itr != norm.Max.end(); ++itr)
    {
      if (*itr > max)
      {
        *itr = max;
      }
    }

    if (max > 0.0 && normalize)
    {
      svtkSMPTools::For(0, num, [&](svtkIdType i, svtkIdType end) {
        float* s = algo.Scalars + i;
        for (; i < end; ++i)
        {
          *s++ /= max;
        }
      });
    }
  }
};
}

//=================================Begin class proper=========================
//----------------------------------------------------------------------------
// Construct with normalize flag off.
svtkVectorNorm::svtkVectorNorm()
{
  this->Normalize = 0;
  this->AttributeMode = SVTK_ATTRIBUTE_MODE_DEFAULT;
}

//----------------------------------------------------------------------------
int svtkVectorNorm::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType numVectors;
  int computePtScalars = 1, computeCellScalars = 1;
  svtkFloatArray* newScalars;
  svtkDataArray *ptVectors, *cellVectors;
  svtkPointData *pd = input->GetPointData(), *outPD = output->GetPointData();
  svtkCellData *cd = input->GetCellData(), *outCD = output->GetCellData();

  // Initialize
  svtkDebugMacro(<< "Computing norm of vectors!");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  ptVectors = pd->GetVectors();
  cellVectors = cd->GetVectors();
  if (!ptVectors || this->AttributeMode == SVTK_ATTRIBUTE_MODE_USE_CELL_DATA)
  {
    computePtScalars = 0;
  }

  if (!cellVectors || this->AttributeMode == SVTK_ATTRIBUTE_MODE_USE_POINT_DATA)
  {
    computeCellScalars = 0;
  }

  if (!computeCellScalars && !computePtScalars)
  {
    svtkErrorMacro(<< "No vector norm to compute!");
    return 1;
  }

  // Needed for point and cell vector normals computation
  svtkVectorNormDispatch normDispatch;
  bool normalize = (this->GetNormalize() != 0);

  // Allocate / operate on point data
  if (computePtScalars)
  {
    numVectors = ptVectors->GetNumberOfTuples();
    newScalars = svtkFloatArray::New();
    newScalars->SetNumberOfTuples(numVectors);

    if (!svtkArrayDispatch::Dispatch::Execute(
          ptVectors, normDispatch, normalize, numVectors, newScalars->GetPointer(0)))
    {
      normDispatch(ptVectors, normalize, numVectors, newScalars->GetPointer(0));
    }

    int idx = outPD->AddArray(newScalars);
    outPD->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);
    newScalars->Delete();
    outPD->CopyScalarsOff();
  } // if computing point scalars

  this->UpdateProgress(0.50);

  // Allocate / operate on cell data
  if (computeCellScalars)
  {
    numVectors = cellVectors->GetNumberOfTuples();
    newScalars = svtkFloatArray::New();
    newScalars->SetNumberOfTuples(numVectors);

    if (!svtkArrayDispatch::Dispatch::Execute(
          cellVectors, normDispatch, normalize, numVectors, newScalars->GetPointer(0)))
    {
      normDispatch(cellVectors, normalize, numVectors, newScalars->GetPointer(0));
    }

    int idx = outCD->AddArray(newScalars);
    outCD->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);
    newScalars->Delete();
    outCD->CopyScalarsOff();
  } // if computing cell scalars

  // Pass appropriate data through to output
  outPD->PassData(pd);
  outCD->PassData(cd);

  return 1;
}

//----------------------------------------------------------------------------
// Return the method for generating scalar data as a string.
const char* svtkVectorNorm::GetAttributeModeAsString()
{
  if (this->AttributeMode == SVTK_ATTRIBUTE_MODE_DEFAULT)
  {
    return "Default";
  }
  else if (this->AttributeMode == SVTK_ATTRIBUTE_MODE_USE_POINT_DATA)
  {
    return "UsePointData";
  }
  else
  {
    return "UseCellData";
  }
}

//----------------------------------------------------------------------------
void svtkVectorNorm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Normalize: " << (this->Normalize ? "On\n" : "Off\n");
  os << indent << "Attribute Mode: " << this->GetAttributeModeAsString() << endl;
}
