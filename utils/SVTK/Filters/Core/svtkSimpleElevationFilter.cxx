/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSimpleElevationFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSimpleElevationFilter.h"

#include "svtkArrayDispatch.h"
#include "svtkCellData.h"
#include "svtkDataArrayRange.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkSMPTools.h"

svtkStandardNewMacro(svtkSimpleElevationFilter);

namespace {

// The heart of the algorithm plus interface to the SMP tools.
template <class PointArrayT>
class svtkSimpleElevationAlgorithm
{
public:
  svtkIdType NumPts;
  double Vector[3];
  PointArrayT *PointArray;
  float *Scalars;

  svtkSimpleElevationAlgorithm(PointArrayT* pointArray,
                              svtkSimpleElevationFilter* filter,
                              float* scalars)
    : NumPts{pointArray->GetNumberOfTuples()}
    , PointArray{pointArray}
    , Scalars{scalars}
  {
    filter->GetVector(this->Vector);
  }

  // Interface implicit function computation to SMP tools.
  void operator()(svtkIdType begin, svtkIdType end)
  {
    const double* v = this->Vector;
    float* s = this->Scalars + begin;

    const auto pointRange = svtk::DataArrayTupleRange<3>(this->PointArray,
                                                        begin, end);

    for (const auto p : pointRange)
    {
      *s = v[0]*p[0] + v[1]*p[1] + v[2]*p[2];
      ++s;
    }
  }
};

//----------------------------------------------------------------------------
// Templated class is glue between SVTK and templated algorithms.
struct Elevate
{
  template <typename PointArrayT>
  void operator()(PointArrayT* pointArray,
                  svtkSimpleElevationFilter* filter,
                  float* scalars)
  {
    svtkSimpleElevationAlgorithm<PointArrayT> algo{pointArray, filter, scalars};
    svtkSMPTools::For(0, pointArray->GetNumberOfTuples(), algo);
  }
};

} // end anon namespace

// Okay begin the class proper
//----------------------------------------------------------------------------
// Construct object with Vector=(0,0,1).
svtkSimpleElevationFilter::svtkSimpleElevationFilter()
{
  this->Vector[0] = 0.0;
  this->Vector[1] = 0.0;
  this->Vector[2] = 1.0;
}

//----------------------------------------------------------------------------
// Convert position along the ray into scalar value.  Example use includes
// coloring terrain by elevation.
//
int svtkSimpleElevationFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType i, numPts;
  svtkFloatArray* newScalars;
  double s, x[3];

  // Initialize
  //
  svtkDebugMacro(<< "Generating elevation scalars!");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  if (((numPts = input->GetNumberOfPoints()) < 1))
  {
    svtkDebugMacro(<< "No input!");
    return 1;
  }

  // Allocate
  //
  newScalars = svtkFloatArray::New();
  newScalars->SetNumberOfTuples(numPts);

  // Ensure that there is a valid vector
  //
  if (svtkMath::Dot(this->Vector, this->Vector) == 0.0)
  {
    svtkErrorMacro(<< "Bad vector, using (0,0,1)");
    this->Vector[0] = this->Vector[1] = 0.0;
    this->Vector[2] = 1.0;
  }

  // Create a fast path for point set input
  //
  svtkPointSet* ps = svtkPointSet::SafeDownCast(input);
  if (ps)
  {
    float* scalars = newScalars->GetPointer(0);
    svtkPoints* points = ps->GetPoints();
    svtkDataArray* pointsArray = points->GetData();

    Elevate worker; // Entry point to svtkSimpleElevationAlgorithm

    // Generate an optimized fast-path for float/double
    using FastValueTypes = svtkArrayDispatch::Reals;
    using Dispatcher = svtkArrayDispatch::DispatchByValueType<FastValueTypes>;
    if (!Dispatcher::Execute(pointsArray, worker, this, scalars))
    { // fallback for unknown arrays and integral value types:
      worker(pointsArray, this, scalars);
    }
  } // fast path

  else
  {
    // Too bad, got to take the scenic route.
    // Compute dot product.
    //
    int abort = 0;
    svtkIdType progressInterval = numPts / 20 + 1;
    for (i = 0; i < numPts && !abort; i++)
    {
      if (!(i % progressInterval))
      {
        this->UpdateProgress((double)i / numPts);
        abort = this->GetAbortExecute();
      }

      input->GetPoint(i, x);
      s = svtkMath::Dot(this->Vector, x);
      newScalars->SetComponent(i, 0, s);
    }
  }

  // Update self
  //
  output->GetPointData()->CopyScalarsOff();
  output->GetPointData()->PassData(input->GetPointData());

  output->GetCellData()->PassData(input->GetCellData());

  newScalars->SetName("Elevation");
  output->GetPointData()->AddArray(newScalars);
  output->GetPointData()->SetActiveScalars(newScalars->GetName());
  newScalars->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void svtkSimpleElevationFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Vector: (" << this->Vector[0] << ", " << this->Vector[1] << ", "
     << this->Vector[2] << ")\n";
}
