/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageImportExecutive.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageImportExecutive.h"

#include "svtkInformationIntegerKey.h"
#include "svtkInformationIntegerVectorKey.h"
#include "svtkObjectFactory.h"

#include "svtkAlgorithm.h"
#include "svtkAlgorithmOutput.h"
#include "svtkImageImport.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"

svtkStandardNewMacro(svtkImageImportExecutive);

//----------------------------------------------------------------------------
svtkTypeBool svtkImageImportExecutive::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  if (this->Algorithm && request->Has(REQUEST_INFORMATION()))
  {
    // Invoke the callback
    svtkImageImport* ii = svtkImageImport::SafeDownCast(this->Algorithm);
    ii->InvokeUpdateInformationCallbacks();
  }

  return this->Superclass::ProcessRequest(request, inInfoVec, outInfoVec);
}
