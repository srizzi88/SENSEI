/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkArrayNorm.cxx

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkArrayNorm.h"
#include "svtkCommand.h"
#include "svtkDenseArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"

#include <limits>
#include <sstream>
#include <stdexcept>

///////////////////////////////////////////////////////////////////////////////
// svtkArrayNorm

svtkStandardNewMacro(svtkArrayNorm);

svtkArrayNorm::svtkArrayNorm()
  : Dimension(0)
  , L(2)
  , Invert(false)
  , Window(0, std::numeric_limits<svtkIdType>::max())
{
}

svtkArrayNorm::~svtkArrayNorm() = default;

void svtkArrayNorm::SetWindow(const svtkArrayRange& window)
{
  if (window == this->Window)
    return;

  this->Window = window;
  this->Modified();
}

svtkArrayRange svtkArrayNorm::GetWindow()
{
  return this->Window;
}

void svtkArrayNorm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Dimension: " << this->Dimension << endl;
  os << indent << "L: " << this->L << endl;
  os << indent << "Invert: " << this->Invert << endl;
  os << indent << "Window: " << this->Window << endl;
}

void svtkArrayNorm::SetL(int value)
{
  if (value < 1)
  {
    svtkErrorMacro(<< "Cannot compute array norm for L < 1");
    return;
  }

  if (this->L == value)
    return;

  this->L = value;
  this->Modified();
}

int svtkArrayNorm::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  try
  {
    // Test our preconditions ...
    svtkArrayData* const input_data = svtkArrayData::GetData(inputVector[0]);
    if (!input_data)
      throw std::runtime_error("Missing svtkArrayData on input port 0.");
    if (input_data->GetNumberOfArrays() != 1)
      throw std::runtime_error("svtkArrayData on input port 0 must contain exactly one svtkArray.");
    svtkTypedArray<double>* const input_array =
      svtkTypedArray<double>::SafeDownCast(input_data->GetArray(static_cast<svtkIdType>(0)));
    if (!input_array)
      throw std::runtime_error("svtkArray on input port 0 must be a svtkTypedArray<double>.");
    if (input_array->GetDimensions() != 2)
      throw std::runtime_error("svtkArray on input port 0 must be a matrix.");

    const svtkIdType vector_dimension = this->Dimension;
    if (vector_dimension < 0 || vector_dimension > 1)
      throw std::runtime_error("Dimension must be zero or one.");
    const svtkIdType element_dimension = 1 - vector_dimension;

    // Setup our output ...
    std::ostringstream array_name;
    array_name << "L" << this->L << "_norm";

    svtkDenseArray<double>* const output_array = svtkDenseArray<double>::New();
    output_array->SetName(array_name.str());
    output_array->Resize(input_array->GetExtent(vector_dimension));
    output_array->Fill(0.0);

    svtkArrayData* const output = svtkArrayData::GetData(outputVector);
    output->ClearArrays();
    output->AddArray(output_array);
    output_array->Delete();

    // Make it happen ...
    svtkArrayCoordinates coordinates;
    const svtkIdType non_null_count = input_array->GetNonNullSize();
    for (svtkIdType n = 0; n != non_null_count; ++n)
    {
      input_array->GetCoordinatesN(n, coordinates);
      if (!this->Window.Contains(coordinates[element_dimension]))
        continue;
      output_array->SetValue(coordinates[vector_dimension],
        output_array->GetValue(coordinates[vector_dimension]) +
          pow(input_array->GetValueN(n), this->L));
    }

    for (svtkArray::SizeT n = 0; n != output_array->GetNonNullSize(); ++n)
    {
      output_array->SetValueN(n, pow(output_array->GetValueN(n), 1.0 / this->L));
    }

    // Optionally invert the output vector
    if (this->Invert)
    {
      for (svtkArray::SizeT n = 0; n != output_array->GetNonNullSize(); ++n)
      {
        if (output_array->GetValueN(n))
          output_array->SetValueN(n, 1.0 / output_array->GetValueN(n));
      }
    }
  }
  catch (std::exception& e)
  {
    svtkErrorMacro(<< "unhandled exception: " << e.what());
    return 0;
  }
  catch (...)
  {
    svtkErrorMacro(<< "unknown exception");
    return 0;
  }

  return 1;
}
