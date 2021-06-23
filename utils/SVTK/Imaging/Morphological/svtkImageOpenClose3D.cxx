/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageOpenClose3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageOpenClose3D.h"

#include "svtkCommand.h"
#include "svtkGarbageCollector.h"
#include "svtkImageData.h"
#include "svtkImageDilateErode3D.h"
#include "svtkObjectFactory.h"

#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"

#include <cmath>

svtkStandardNewMacro(svtkImageOpenClose3D);

//----------------------------------------------------------------------------
// functions to convert progress calls.
class svtkImageOpenClose3DProgress : public svtkCommand
{
public:
  // generic new method
  static svtkImageOpenClose3DProgress* New() { return new svtkImageOpenClose3DProgress; }

  // the execute
  void Execute(svtkObject* caller, unsigned long event, void* svtkNotUsed(v)) override
  {
    svtkAlgorithm* alg = svtkAlgorithm::SafeDownCast(caller);
    if (event == svtkCommand::ProgressEvent && alg)
    {
      this->Self->UpdateProgress(this->Offset + 0.5 * alg->GetProgress());
    }
  }

  // some ivars that should be set
  svtkImageOpenClose3D* Self;
  double Offset;
};

//----------------------------------------------------------------------------
svtkImageOpenClose3D::svtkImageOpenClose3D()
{
  // create the filter chain
  this->Filter0 = svtkImageDilateErode3D::New();
  svtkImageOpenClose3DProgress* cb = svtkImageOpenClose3DProgress::New();
  cb->Self = this;
  cb->Offset = 0;
  this->Filter0->AddObserver(svtkCommand::ProgressEvent, cb);
  cb->Delete();

  this->Filter1 = svtkImageDilateErode3D::New();
  cb = svtkImageOpenClose3DProgress::New();
  cb->Self = this;
  cb->Offset = 0.5;
  this->Filter1->AddObserver(svtkCommand::ProgressEvent, cb);
  cb->Delete();
  this->SetOpenValue(0.0);
  this->SetCloseValue(255.0);

  // connect up the internal pipeline
  this->Filter1->SetInputConnection(this->Filter0->GetOutputPort());
}

//----------------------------------------------------------------------------
// Destructor: Delete the sub filters.
svtkImageOpenClose3D::~svtkImageOpenClose3D()
{
  if (this->Filter0)
  {
    this->Filter0->Delete();
  }

  if (this->Filter1)
  {
    this->Filter1->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkImageOpenClose3D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Filter0: \n";
  this->Filter0->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Filter1: \n";
  this->Filter1->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
// Turn debugging output on. (in sub filters also)
void svtkImageOpenClose3D::DebugOn()
{
  this->svtkObject::DebugOn();
  if (this->Filter0)
  {
    this->Filter0->DebugOn();
  }
  if (this->Filter1)
  {
    this->Filter1->DebugOn();
  }
}

//----------------------------------------------------------------------------
void svtkImageOpenClose3D::DebugOff()
{
  this->svtkObject::DebugOff();
  if (this->Filter0)
  {
    this->Filter0->DebugOff();
  }
  if (this->Filter1)
  {
    this->Filter1->DebugOff();
  }
}

//----------------------------------------------------------------------------
// Pass modified message to sub filters.
void svtkImageOpenClose3D::Modified()
{
  this->svtkObject::Modified();
  if (this->Filter0)
  {
    this->Filter0->Modified();
  }

  if (this->Filter1)
  {
    this->Filter1->Modified();
  }
}

//----------------------------------------------------------------------------
// This method considers the sub filters MTimes when computing this objects
// MTime
svtkMTimeType svtkImageOpenClose3D::GetMTime()
{
  svtkMTimeType t1, t2;

  t1 = this->Superclass::GetMTime();
  if (this->Filter0)
  {
    t2 = this->Filter0->GetMTime();
    if (t2 > t1)
    {
      t1 = t2;
    }
  }
  if (this->Filter1)
  {
    t2 = this->Filter1->GetMTime();
    if (t2 > t1)
    {
      t1 = t2;
    }
  }

  return t1;
}

//----------------------------------------------------------------------------
int svtkImageOpenClose3D::ComputePipelineMTime(svtkInformation* request,
  svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec, int requestFromOutputPort,
  svtkMTimeType* mtime)
{
  // Process the request on the internal pipeline.  Share our input
  // information with the first filter and our output information with
  // the last filter.
  svtkExecutive* exec0 = this->Filter0->GetExecutive();
  svtkExecutive* exec1 = this->Filter1->GetExecutive();
  exec0->SetSharedInputInformation(inInfoVec);
  exec1->SetSharedOutputInformation(outInfoVec);
  svtkMTimeType mtime1;
  if (exec1->ComputePipelineMTime(request, exec1->GetInputInformation(),
        exec1->GetOutputInformation(), requestFromOutputPort, &mtime1))
  {
    // Now run the request in this algorithm.
    return this->Superclass::ComputePipelineMTime(
      request, inInfoVec, outInfoVec, requestFromOutputPort, mtime);
  }
  else
  {
    // The internal pipeline failed to process the request.
    svtkErrorMacro("Internal pipeline failed to process pipeline modified "
                  "time request.");
    return 0;
  }
}

//----------------------------------------------------------------------------
svtkTypeBool svtkImageOpenClose3D::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  // Process the request on the internal pipeline.  Share our input
  // information with the first filter and our output information with
  // the last filter.
  svtkExecutive* exec0 = this->Filter0->GetExecutive();
  svtkExecutive* exec1 = this->Filter1->GetExecutive();
  exec0->SetSharedInputInformation(inInfoVec);
  exec1->SetSharedOutputInformation(outInfoVec);
  return exec1->ProcessRequest(
    request, exec1->GetInputInformation(), exec1->GetOutputInformation());
}

//----------------------------------------------------------------------------
// Selects the size of gaps or objects removed.
void svtkImageOpenClose3D::SetKernelSize(int size0, int size1, int size2)
{
  if (!this->Filter0 || !this->Filter1)
  {
    svtkErrorMacro(<< "SetKernelSize: Sub filter not created yet.");
    return;
  }

  this->Filter0->SetKernelSize(size0, size1, size2);
  this->Filter1->SetKernelSize(size0, size1, size2);
  // Sub filters take care of modified.
}

//----------------------------------------------------------------------------
// Determines the value that will closed.
// Close value is first dilated, and then eroded
void svtkImageOpenClose3D::SetCloseValue(double value)
{
  if (!this->Filter0 || !this->Filter1)
  {
    svtkErrorMacro(<< "SetCloseValue: Sub filter not created yet.");
    return;
  }

  this->Filter0->SetDilateValue(value);
  this->Filter1->SetErodeValue(value);
}

//----------------------------------------------------------------------------
double svtkImageOpenClose3D::GetCloseValue()
{
  if (!this->Filter0)
  {
    svtkErrorMacro(<< "GetCloseValue: Sub filter not created yet.");
    return 0.0;
  }

  return this->Filter0->GetDilateValue();
}

//----------------------------------------------------------------------------
// Determines the value that will opened.
// Open value is first eroded, and then dilated.
void svtkImageOpenClose3D::SetOpenValue(double value)
{
  if (!this->Filter0 || !this->Filter1)
  {
    svtkErrorMacro(<< "SetOpenValue: Sub filter not created yet.");
    return;
  }

  this->Filter0->SetErodeValue(value);
  this->Filter1->SetDilateValue(value);
}

//----------------------------------------------------------------------------
double svtkImageOpenClose3D::GetOpenValue()
{
  if (!this->Filter0)
  {
    svtkErrorMacro(<< "GetOpenValue: Sub filter not created yet.");
    return 0.0;
  }

  return this->Filter0->GetErodeValue();
}

//----------------------------------------------------------------------------
void svtkImageOpenClose3D::ReportReferences(svtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // These filters share our input and are therefore involved in a
  // reference loop.
  svtkGarbageCollectorReport(collector, this->Filter0, "Filter0");
  svtkGarbageCollectorReport(collector, this->Filter1, "Filter1");
}
