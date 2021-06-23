/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostLogWeighting.cxx

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

#include "svtkBoostLogWeighting.h"
#include "svtkArrayCoordinates.h"
#include "svtkCommand.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkTypedArray.h"

#include <boost/math/special_functions/log1p.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION < 103400
#error "svtkBoostLogWeighting requires Boost 1.34.0 or later"
#endif

#include <cmath>
#include <stdexcept>

///////////////////////////////////////////////////////////////////////////////
// svtkBoostLogWeighting

svtkStandardNewMacro(svtkBoostLogWeighting);

svtkBoostLogWeighting::svtkBoostLogWeighting()
  : Base(BASE_E)
  , EmitProgress(true)
{
}

svtkBoostLogWeighting::~svtkBoostLogWeighting() {}

void svtkBoostLogWeighting::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Base: " << this->Base << endl;
  os << indent << "EmitProgress: " << (this->EmitProgress ? "on" : "off") << endl;
}

int svtkBoostLogWeighting::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  try
  {
    svtkArrayData* const input_data = svtkArrayData::GetData(inputVector[0]);
    if (!input_data)
      throw std::runtime_error("Missing input svtkArrayData on port 0.");
    if (input_data->GetNumberOfArrays() != 1)
      throw std::runtime_error("Input svtkArrayData must contain exactly one array.");
    svtkTypedArray<double>* const input_array =
      svtkTypedArray<double>::SafeDownCast(input_data->GetArray(0));
    if (!input_array)
      throw std::runtime_error("Unsupported input array type.");

    svtkTypedArray<double>* const output_array =
      svtkTypedArray<double>::SafeDownCast(input_array->DeepCopy());
    svtkArrayData* const output = svtkArrayData::GetData(outputVector);
    output->ClearArrays();
    output->AddArray(output_array);
    output_array->Delete();

    const svtkIdType value_count = input_array->GetNonNullSize();
    switch (this->Base)
    {
      case BASE_E:
      {
        if (this->EmitProgress)
        {
          for (svtkIdType i = 0; i != value_count; ++i)
          {
            output_array->SetValueN(i, boost::math::log1p(output_array->GetValueN(i)));

            double progress = static_cast<double>(i) / static_cast<double>(value_count);
            this->InvokeEvent(svtkCommand::ProgressEvent, &progress);
          }
        }
        else
        {
          for (svtkIdType i = 0; i != value_count; ++i)
          {
            output_array->SetValueN(i, boost::math::log1p(output_array->GetValueN(i)));
          }
        }
        break;
      }
      case BASE_2:
      {
        const double ln2 = log(2.0);
        if (this->EmitProgress)
        {
          for (svtkIdType i = 0; i != value_count; ++i)
          {
            output_array->SetValueN(i, 1.0 + log(output_array->GetValueN(i)) / ln2);

            double progress = static_cast<double>(i) / static_cast<double>(value_count);
            this->InvokeEvent(svtkCommand::ProgressEvent, &progress);
          }
        }
        else
        {
          for (svtkIdType i = 0; i != value_count; ++i)
          {
            output_array->SetValueN(i, 1.0 + log(output_array->GetValueN(i)) / ln2);
          }
        }
        break;
      }
      default:
        throw std::runtime_error("Unknown Base type.");
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
