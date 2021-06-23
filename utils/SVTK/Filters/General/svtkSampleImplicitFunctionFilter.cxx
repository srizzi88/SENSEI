/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSampleImplicitFunctionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSampleImplicitFunctionFilter.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkFloatArray.h"
#include "svtkGarbageCollector.h"
#include "svtkImplicitFunction.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSMPTools.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkSampleImplicitFunctionFilter);
svtkCxxSetObjectMacro(svtkSampleImplicitFunctionFilter, ImplicitFunction, svtkImplicitFunction);

// Interface between svtkSMPTools and the SVTK pipeline
namespace
{

struct SampleDataSet
{
  svtkDataSet* Input;
  svtkImplicitFunction* Function;
  float* Scalars;

  // Constructor
  SampleDataSet(svtkDataSet* input, svtkImplicitFunction* imp, float* s)
    : Input(input)
    , Function(imp)
    , Scalars(s)
  {
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    double x[3];
    float* n = this->Scalars + ptId;
    for (; ptId < endPtId; ++ptId)
    {
      this->Input->GetPoint(ptId, x);
      *n++ = this->Function->FunctionValue(x);
    }
  }
};

struct SampleDataSetWithGradients
{
  svtkDataSet* Input;
  svtkImplicitFunction* Function;
  float* Scalars;
  float* Gradients;

  // Constructor
  SampleDataSetWithGradients(svtkDataSet* input, svtkImplicitFunction* imp, float* s, float* g)
    : Input(input)
    , Function(imp)
    , Scalars(s)
    , Gradients(g)
  {
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    double x[3], g[3];
    float* n = this->Scalars + ptId;
    float* v = this->Gradients + 3 * ptId;
    for (; ptId < endPtId; ++ptId)
    {
      this->Input->GetPoint(ptId, x);
      *n++ = this->Function->FunctionValue(x);
      this->Function->FunctionGradient(x, g);
      *v++ = g[0];
      *v++ = g[1];
      *v++ = g[2];
    }
  }
};

} // anonymous namespace

//----------------------------------------------------------------------------
// Okay define the SVTK class proper
svtkSampleImplicitFunctionFilter::svtkSampleImplicitFunctionFilter()
{
  this->ImplicitFunction = nullptr;

  this->ComputeGradients = 1;

  this->ScalarArrayName = nullptr;
  this->SetScalarArrayName("Implicit scalars");

  this->GradientArrayName = nullptr;
  this->SetGradientArrayName("Implicit gradients");
}

//----------------------------------------------------------------------------
svtkSampleImplicitFunctionFilter::~svtkSampleImplicitFunctionFilter()
{
  this->SetImplicitFunction(nullptr);
  this->SetScalarArrayName(nullptr);
  this->SetGradientArrayName(nullptr);
}

//----------------------------------------------------------------------------
// Produce the output data
int svtkSampleImplicitFunctionFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkDebugMacro(<< "Generating implicit data");

  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Check the input
  if (!input || !output)
  {
    return 1;
  }
  svtkIdType numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    return 1;
  }

  // Ensure implicit function is specified
  //
  if (!this->ImplicitFunction)
  {
    svtkErrorMacro(<< "No implicit function specified");
    return 1;
  }

  // The output geometic structure is the same as the input
  output->CopyStructure(input);

  // Pass the output attribute data
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // Set up for execution
  svtkFloatArray* newScalars = svtkFloatArray::New();
  newScalars->SetNumberOfTuples(numPts);
  float* scalars = newScalars->WritePointer(0, numPts);

  svtkFloatArray* newGradients = nullptr;
  float* gradients = nullptr;
  if (this->ComputeGradients)
  {
    newGradients = svtkFloatArray::New();
    newGradients->SetNumberOfComponents(3);
    newGradients->SetNumberOfTuples(numPts);
    gradients = newGradients->WritePointer(0, numPts);
  }

  // Threaded execute
  if (this->ComputeGradients)
  {
    SampleDataSetWithGradients sample(input, this->ImplicitFunction, scalars, gradients);
    svtkSMPTools::For(0, numPts, sample);
  }
  else
  {
    SampleDataSet sample(input, this->ImplicitFunction, scalars);
    svtkSMPTools::For(0, numPts, sample);
  }

  // Update self
  newScalars->SetName(this->ScalarArrayName);
  output->GetPointData()->AddArray(newScalars);
  output->GetPointData()->SetActiveScalars(this->ScalarArrayName);
  newScalars->Delete();

  if (this->ComputeGradients)
  {
    newGradients->SetName(this->GradientArrayName);
    output->GetPointData()->AddArray(newGradients);
    output->GetPointData()->SetActiveVectors(this->GradientArrayName);
    newGradients->Delete();
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkSampleImplicitFunctionFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
svtkMTimeType svtkSampleImplicitFunctionFilter::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType impFuncMTime;

  if (this->ImplicitFunction != nullptr)
  {
    impFuncMTime = this->ImplicitFunction->GetMTime();
    mTime = (impFuncMTime > mTime ? impFuncMTime : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
void svtkSampleImplicitFunctionFilter::ReportReferences(svtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  svtkGarbageCollectorReport(collector, this->ImplicitFunction, "ImplicitFunction");
}

//----------------------------------------------------------------------------
void svtkSampleImplicitFunctionFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->ImplicitFunction)
  {
    os << indent << "Implicit Function: " << this->ImplicitFunction << "\n";
  }
  else
  {
    os << indent << "No Implicit function defined\n";
  }

  os << indent << "Compute Gradients: " << (this->ComputeGradients ? "On\n" : "Off\n");

  os << indent << "Scalar Array Name: ";
  if (this->ScalarArrayName != nullptr)
  {
    os << this->ScalarArrayName << endl;
  }
  else
  {
    os << "(none)" << endl;
  }

  os << indent << "Gradient Array Name: ";
  if (this->GradientArrayName != nullptr)
  {
    os << this->GradientArrayName << endl;
  }
  else
  {
    os << "(none)" << endl;
  }
}
