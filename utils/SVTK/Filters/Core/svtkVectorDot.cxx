/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVectorDot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkVectorDot.h"

#include "svtkArrayDispatch.h"
#include "svtkDataArrayRange.h"
#include "svtkDataSet.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSMPTools.h"

#include <algorithm>
#include <limits>

svtkStandardNewMacro(svtkVectorDot);

namespace {

template <typename NormArrayT, typename VecArrayT>
struct DotWorker
{
  NormArrayT *Normals;
  VecArrayT *Vectors;
  svtkFloatArray *Scalars;

  svtkSMPThreadLocal<float> LocalMin;
  svtkSMPThreadLocal<float> LocalMax;

  DotWorker(NormArrayT *normals,
            VecArrayT *vectors,
            svtkFloatArray *scalars)
    : Normals{normals}
    , Vectors{vectors}
    , Scalars{scalars}
    , LocalMin{std::numeric_limits<float>::max()}
    , LocalMax{std::numeric_limits<float>::lowest()}
  {
  }

  void operator()(svtkIdType begin, svtkIdType end)
  {
    float &min = this->LocalMin.Local();
    float &max = this->LocalMax.Local();

    // Restrict the iterator ranges to [begin,end)
    const auto normals = svtk::DataArrayTupleRange<3>(this->Normals, begin, end);
    const auto vectors = svtk::DataArrayTupleRange<3>(this->Vectors, begin, end);
    auto scalars = svtk::DataArrayValueRange<1>(this->Scalars, begin, end);

    using NormalConstRef = typename decltype(normals)::ConstTupleReferenceType;
    using VectorConstRef = typename decltype(vectors)::ConstTupleReferenceType;

    auto computeScalars = [&](NormalConstRef n, VectorConstRef v) -> float
    {
      const float s = static_cast<float>(n[0]*v[0] + n[1]*v[1] + n[2]*v[2]);

      min = std::min(min, s);
      max = std::max(max, s);

      return s;
    };

    std::transform(normals.cbegin(), normals.cend(),
                   vectors.cbegin(),
                   scalars.begin(),
                   computeScalars);
  }
};

// Dispatcher entry point:
struct LaunchDotWorker
{
  template <typename NormArrayT, typename VecArrayT>
  void operator()(NormArrayT *normals, VecArrayT *vectors,
                  svtkFloatArray *scalars, float scalarRange[2])
  {
    const svtkIdType numPts = normals->GetNumberOfTuples();

    using Worker = DotWorker<NormArrayT, VecArrayT>;
    Worker worker{normals, vectors, scalars};

    svtkSMPTools::For(0, numPts, worker);

    // Reduce the scalar ranges:
    auto minElem = std::min_element(worker.LocalMin.begin(),
                                    worker.LocalMin.end());
    auto maxElem = std::max_element(worker.LocalMax.begin(),
                                    worker.LocalMax.end());

    // There should be at least one element in the range from worker
    // initialization:
    assert(minElem != worker.LocalMin.end());
    assert(maxElem != worker.LocalMax.end());

    scalarRange[0] = *minElem;
    scalarRange[1] = *maxElem;
  }
};

struct MapWorker
{
  svtkFloatArray *Scalars;
  float InMin;
  float InRange;
  float OutMin;
  float OutRange;

  void operator()(svtkIdType begin, svtkIdType end)
  {
    // Restrict the iterator range to [begin,end)
    auto scalars = svtk::DataArrayValueRange<1>(this->Scalars, begin, end);

    using ScalarRef = typename decltype(scalars)::ReferenceType;

    for (ScalarRef s : scalars)
    {
      // Map from inRange to outRange:
      s = this->OutMin + ((s - this->InMin) / this->InRange) * this->OutRange;
    }
  }
};

} // end anon namespace

//=================================Begin class proper=========================
//----------------------------------------------------------------------------
// Construct object with scalar range (-1,1).
svtkVectorDot::svtkVectorDot()
{
  this->MapScalars = 1;

  this->ScalarRange[0] = -1.0;
  this->ScalarRange[1] = 1.0;

  this->ActualRange[0] = -1.0;
  this->ActualRange[1] = 1.0;
}

//----------------------------------------------------------------------------
// Compute dot product.
//
int svtkVectorDot::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType numPts;
  svtkFloatArray* newScalars;
  svtkDataArray* inNormals;
  svtkDataArray* inVectors;
  svtkPointData *pd = input->GetPointData(), *outPD = output->GetPointData();

  // Initialize
  //
  svtkDebugMacro(<< "Generating vector/normal dot product!");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  if ((numPts = input->GetNumberOfPoints()) < 1)
  {
    svtkErrorMacro(<< "No points!");
    return 1;
  }
  if ((inNormals = pd->GetNormals()) == nullptr)
  {
    svtkErrorMacro(<< "No normals defined!");
    return 1;
  }
  if ((inVectors = pd->GetVectors()) == nullptr)
  {
    svtkErrorMacro(<< "No vectors defined!");
    return 1;
  }

  // Allocate
  //
  newScalars = svtkFloatArray::New();
  newScalars->SetNumberOfTuples(numPts);

  // This is potentiall a two pass algorithm. The first pass computes the dot
  // product and keeps track of min/max scalar values; and the second
  // (optional pass) maps the output into a specified range. Passes two and
  // three are optional.

  // Compute dot product. Use a fast path for double/float:
  using svtkArrayDispatch::Reals;
  using Dispatcher = svtkArrayDispatch::Dispatch2ByValueType<Reals, Reals>;
  LaunchDotWorker dotWorker;

  float aRange[2];
  if (!Dispatcher::Execute(inNormals, inVectors, dotWorker, newScalars, aRange))
  { // fallback to slow path:
    dotWorker(inNormals, inVectors, newScalars, aRange);
  }

  // Update ivars:
  this->ActualRange[0] = static_cast<double>(aRange[0]);
  this->ActualRange[1] = static_cast<double>(aRange[1]);

  // Map if requested:
  if (this->GetMapScalars())
  {
    MapWorker mapWorker{newScalars,
                        aRange[1] - aRange[0],
                        aRange[0],
                        static_cast<float>(this->ScalarRange[1] -
                                           this->ScalarRange[0]),
                        static_cast<float>(this->ScalarRange[0])};

    svtkSMPTools::For(0, newScalars->GetNumberOfValues(), mapWorker);
  }

  // Update self and release memory
  //
  outPD->PassData(input->GetPointData());

  int idx = outPD->AddArray(newScalars);
  outPD->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);
  newScalars->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void svtkVectorDot::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "MapScalars: " << (this->MapScalars ? "On\n" : "Off\n");

  os << indent << "Scalar Range: (" << this->ScalarRange[0] << ", " << this->ScalarRange[1]
     << ")\n";

  os << indent << "Actual Range: (" << this->ActualRange[0] << ", " << this->ActualRange[1]
     << ")\n";
}
