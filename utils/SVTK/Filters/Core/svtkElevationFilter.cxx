/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkElevationFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkElevationFilter.h"

#include "svtkArrayDispatch.h"
#include "svtkCellData.h"
#include "svtkDataArrayRange.h"
#include "svtkDataSet.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkSMPTools.h"
#include "svtkSmartPointer.h"

svtkStandardNewMacro(svtkElevationFilter);

namespace {

// The heart of the algorithm plus interface to the SMP tools.
template <class PointArrayT>
struct svtkElevationAlgorithm
{
  svtkIdType NumPts;
  double LowPoint[3];
  double HighPoint[3];
  double ScalarRange[2];
  PointArrayT* PointArray;
  float* Scalars;
  const double* V;
  double L2;

  svtkElevationAlgorithm(PointArrayT* pointArray,
                        svtkElevationFilter* filter,
                        float* scalars,
                        const double* v,
                        double l2)
    : NumPts{pointArray->GetNumberOfTuples()}
    , PointArray{pointArray}
    , Scalars{scalars}
    , V{v}
    , L2{l2}
  {
    filter->GetLowPoint(this->LowPoint);
    filter->GetHighPoint(this->HighPoint);
    filter->GetScalarRange(this->ScalarRange);
  }

  // Interface implicit function computation to SMP tools.
  void operator()(svtkIdType begin, svtkIdType end)
  {
    const double* range = this->ScalarRange;
    const double diffScalar = range[1] - range[0];
    const double* v = this->V;
    const double l2 = this->L2;
    const double* lp = this->LowPoint;

    // output scalars:
    float* s = this->Scalars + begin;

    // input points:
    const auto pointRange = svtk::DataArrayTupleRange<3>(this->PointArray,
                                                        begin, end);

    for (const auto point : pointRange)
    {
      double vec[3];
      vec[0] = point[0] - lp[0];
      vec[1] = point[1] - lp[1];
      vec[2] = point[2] - lp[2];

      const double ns = svtkMath::ClampValue(svtkMath::Dot(vec, v) / l2, 0., 1.);

      // Store the resulting scalar value.
      *s = static_cast<float>(range[0] + ns*diffScalar);
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
                  svtkElevationFilter* filter,
                  double* v,
                  double l2,
                  float* scalars)
  {
    // Okay now generate samples using SMP tools
    svtkElevationAlgorithm<PointArrayT> algo{pointArray, filter, scalars, v, l2};
    svtkSMPTools::For(0, pointArray->GetNumberOfTuples(), algo);
  }
};

} // end anon namespace

//----------------------------------------------------------------------------
// Begin the class proper
svtkElevationFilter::svtkElevationFilter()
{
  this->LowPoint[0] = 0.0;
  this->LowPoint[1] = 0.0;
  this->LowPoint[2] = 0.0;

  this->HighPoint[0] = 0.0;
  this->HighPoint[1] = 0.0;
  this->HighPoint[2] = 1.0;

  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
}

//----------------------------------------------------------------------------
svtkElevationFilter::~svtkElevationFilter() = default;

//----------------------------------------------------------------------------
void svtkElevationFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Low Point: (" << this->LowPoint[0] << ", " << this->LowPoint[1] << ", "
     << this->LowPoint[2] << ")\n";
  os << indent << "High Point: (" << this->HighPoint[0] << ", " << this->HighPoint[1] << ", "
     << this->HighPoint[2] << ")\n";
  os << indent << "Scalar Range: (" << this->ScalarRange[0] << ", " << this->ScalarRange[1]
     << ")\n";
}

//----------------------------------------------------------------------------
int svtkElevationFilter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the input and output data objects.
  svtkDataSet* input = svtkDataSet::GetData(inputVector[0]);
  svtkDataSet* output = svtkDataSet::GetData(outputVector);

  // Check the size of the input.
  svtkIdType numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    svtkDebugMacro("No input!");
    return 1;
  }

  // Allocate space for the elevation scalar data.
  svtkSmartPointer<svtkFloatArray> newScalars = svtkSmartPointer<svtkFloatArray>::New();
  newScalars->SetNumberOfTuples(numPts);

  // Set up 1D parametric system and make sure it is valid.
  double diffVector[3] = { this->HighPoint[0] - this->LowPoint[0],
    this->HighPoint[1] - this->LowPoint[1], this->HighPoint[2] - this->LowPoint[2] };
  double length2 = svtkMath::Dot(diffVector, diffVector);
  if (length2 <= 0)
  {
    svtkErrorMacro("Bad vector, using (0,0,1).");
    diffVector[0] = 0;
    diffVector[1] = 0;
    diffVector[2] = 1;
    length2 = 1.0;
  }

  svtkDebugMacro("Generating elevation scalars!");

  // Create a fast path for point set input
  //
  svtkPointSet* ps = svtkPointSet::SafeDownCast(input);
  if (ps)
  {
    float *scalars = newScalars->GetPointer(0);
    svtkPoints *points = ps->GetPoints();
    svtkDataArray *pointsArray = points->GetData();

    Elevate worker; // Entry point to svtkElevationAlgorithm

    // Generate an optimized fast-path for float/double
    using FastValueTypes = svtkArrayDispatch::Reals;
    using Dispatcher = svtkArrayDispatch::DispatchByValueType<FastValueTypes>;
    if (!Dispatcher::Execute(pointsArray, worker, this, diffVector, length2,
                             scalars))
    { // fallback for unknown arrays and integral value types:
      worker(pointsArray, this, diffVector, length2, scalars);
    }
  } // fast path

  else
  {
    // Too bad, got to take the scenic route.
    // Support progress and abort.
    svtkIdType tenth = (numPts >= 10 ? numPts / 10 : 1);
    double numPtsInv = 1.0 / numPts;
    int abort = 0;

    // Compute parametric coordinate and map into scalar range.
    double diffScalar = this->ScalarRange[1] - this->ScalarRange[0];
    for (svtkIdType i = 0; i < numPts && !abort; ++i)
    {
      // Periodically update progress and check for an abort request.
      if (i % tenth == 0)
      {
        this->UpdateProgress((i + 1) * numPtsInv);
        abort = this->GetAbortExecute();
      }

      // Project this input point into the 1D system.
      double x[3];
      input->GetPoint(i, x);
      double v[3] = { x[0] - this->LowPoint[0], x[1] - this->LowPoint[1],
        x[2] - this->LowPoint[2] };
      double s = svtkMath::Dot(v, diffVector) / length2;
      s = (s < 0.0 ? 0.0 : s > 1.0 ? 1.0 : s);

      // Store the resulting scalar value.
      newScalars->SetValue(i, this->ScalarRange[0] + s * diffScalar);
    }
  }

  // Copy all the input geometry and data to the output.
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // Add the new scalars array to the output.
  newScalars->SetName("Elevation");
  output->GetPointData()->AddArray(newScalars);
  output->GetPointData()->SetActiveScalars("Elevation");

  return 1;
}
