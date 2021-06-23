/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDateToNumeric.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDateToNumeric.h"

#include "svtkCommand.h"
#include "svtkDataArraySelection.h"
#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkStringArray.h"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

// old versions of gcc are missing some pices of c++11 such as std::get_time
// so use

#if (defined(__GNUC__) && (__GNUC__ < 5)) || defined(ANDROID)
#define USE_STRPTIME
#include "time.h"
#endif

svtkStandardNewMacro(svtkDateToNumeric);
//----------------------------------------------------------------------------
svtkDateToNumeric::svtkDateToNumeric()
  : DateFormat(nullptr)
{
}

//----------------------------------------------------------------------------
svtkDateToNumeric::~svtkDateToNumeric() {}

//----------------------------------------------------------------------------
int svtkDateToNumeric::FillInputPortInformation(int, svtkInformation* info)
{
  // Skip composite data sets so that executives will treat this as a simple filter
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGenericDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkHyperTreeGrid");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  return 1;
}

//----------------------------------------------------------------------------
int svtkDateToNumeric::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  auto input = svtkDataObject::GetData(inputVector[0], 0);
  auto output = svtkDataObject::GetData(outputVector, 0);
  output->ShallowCopy(input);

  std::vector<std::string> formats;
  if (this->DateFormat)
  {
    formats.push_back(this->DateFormat);
  }
  // default formats
  formats.push_back("%Y-%m-%d %H:%M:%S");
  formats.push_back("%d/%m/%Y %H:%M:%S");

  // now filter arrays for each of the associations.
  for (int association = 0; association < svtkDataObject::NUMBER_OF_ASSOCIATIONS; ++association)
  {
    if (association == svtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS)
    {
      continue;
    }

    auto inFD = input->GetAttributesAsFieldData(association);
    auto outFD = output->GetAttributesAsFieldData(association);
    if (!inFD || !outFD)
    {
      continue;
    }

    auto inDSA = svtkDataSetAttributes::SafeDownCast(inFD);
    auto outDSA = svtkDataSetAttributes::SafeDownCast(outFD);

    for (int idx = 0, max = inFD->GetNumberOfArrays(); idx < max; ++idx)
    {
      svtkStringArray* inarray = svtkStringArray::SafeDownCast(inFD->GetAbstractArray(idx));
      if (inarray && inarray->GetName())
      {
        // look at the first value to see if it is a date we can parse
        auto inval = inarray->GetValue(0);
        std::string useFormat;
        for (auto& format : formats)
        {
#ifdef USE_STRPTIME
          struct tm atime;
          auto result = strptime(inval.c_str(), format.c_str(), &atime);
          if (result)
          {
            useFormat = format;
            break;
          }
#else
          std::tm tm = {};
          std::stringstream ss(inval);
          ss >> std::get_time(&tm, format.c_str());
          if (!ss.fail())
          {
            useFormat = format;
            break;
          }
#endif
        }
        if (useFormat.size())
        {
          svtkNew<svtkDoubleArray> newArray;
          std::string newName = inarray->GetName();
          newName += "_numeric";
          newArray->SetName(newName.c_str());
          newArray->Allocate(inarray->GetNumberOfValues());
          for (svtkIdType i = 0; i < inarray->GetNumberOfValues(); ++i)
          {
            inval = inarray->GetValue(i);
#ifdef USE_STRPTIME
            struct tm atime;
            auto result = strptime(inval.c_str(), useFormat.c_str(), &atime);
            if (result)
            {
              auto etime = mktime(&atime);
              newArray->InsertNextValue(etime);
            }
#else
            std::tm tm = {};
            std::stringstream ss(inval);
            ss >> std::get_time(&tm, useFormat.c_str());
            if (!ss.fail())
            {
              auto etime = std::mktime(&tm);
              newArray->InsertNextValue(etime);
            }
#endif
            else
            {
              newArray->InsertNextValue(0.0);
            }
          }
          outFD->AddArray(newArray);

          // preserve attribute type flags.
          for (int attr = 0; inDSA && outDSA && (attr < svtkDataSetAttributes::NUM_ATTRIBUTES);
               ++attr)
          {
            if (inDSA->GetAbstractAttribute(attr) == inarray)
            {
              outDSA->SetAttribute(newArray, attr);
            }
          }
        }
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkDateToNumeric::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DateFormat: " << (this->DateFormat ? this->DateFormat : "(none)");
}
