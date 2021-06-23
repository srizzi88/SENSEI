/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProgrammableDataObjectSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkProgrammableDataObjectSource.h"

#include "svtkDataObject.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkProgrammableDataObjectSource);

// Construct programmable filter with empty execute method.
svtkProgrammableDataObjectSource::svtkProgrammableDataObjectSource()
{
  this->ExecuteMethod = nullptr;
  this->ExecuteMethodArg = nullptr;
  this->ExecuteMethodArgDelete = nullptr;

  svtkDataObject* output = svtkDataObject::New();
  this->SetOutput(output);
  // Releasing data for pipeline parallism.
  // Filters will know it is empty.
  output->ReleaseData();
  output->Delete();

  this->SetNumberOfInputPorts(0);
}

svtkProgrammableDataObjectSource::~svtkProgrammableDataObjectSource()
{
  // delete the current arg if there is one and a delete meth
  if ((this->ExecuteMethodArg) && (this->ExecuteMethodArgDelete))
  {
    (*this->ExecuteMethodArgDelete)(this->ExecuteMethodArg);
  }
}

// Specify the function to use to generate the source data. Note
// that the function takes a single (void *) argument.
void svtkProgrammableDataObjectSource::SetExecuteMethod(void (*f)(void*), void* arg)
{
  if (f != this->ExecuteMethod || arg != this->ExecuteMethodArg)
  {
    // delete the current arg if there is one and a delete meth
    if ((this->ExecuteMethodArg) && (this->ExecuteMethodArgDelete))
    {
      (*this->ExecuteMethodArgDelete)(this->ExecuteMethodArg);
    }
    this->ExecuteMethod = f;
    this->ExecuteMethodArg = arg;
    this->Modified();
  }
}

// Set the arg delete method. This is used to free user memory.
void svtkProgrammableDataObjectSource::SetExecuteMethodArgDelete(void (*f)(void*))
{
  if (f != this->ExecuteMethodArgDelete)
  {
    this->ExecuteMethodArgDelete = f;
    this->Modified();
  }
}

int svtkProgrammableDataObjectSource::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector*)
{
  svtkDebugMacro(<< "Executing programmable data object filter");

  // Now invoke the procedure, if specified.
  if (this->ExecuteMethod != nullptr)
  {
    (*this->ExecuteMethod)(this->ExecuteMethodArg);
  }

  return 1;
}

void svtkProgrammableDataObjectSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->ExecuteMethod)
  {
    os << indent << "An ExecuteMethod has been defined\n";
  }
  else
  {
    os << indent << "An ExecuteMethod has NOT been defined\n";
  }
}
