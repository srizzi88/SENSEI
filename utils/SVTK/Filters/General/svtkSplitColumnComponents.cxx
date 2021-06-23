/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSplitColumnComponents.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSplitColumnComponents.h"

#include "svtkAbstractArray.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationStringKey.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkStringArray.h"
#include "svtkTable.h"

#include <cmath>
#include <sstream>

svtkStandardNewMacro(svtkSplitColumnComponents);
svtkInformationKeyMacro(svtkSplitColumnComponents, ORIGINAL_ARRAY_NAME, String);
svtkInformationKeyMacro(svtkSplitColumnComponents, ORIGINAL_COMPONENT_NUMBER, Integer);
//---------------------------------------------------------------------------
svtkSplitColumnComponents::svtkSplitColumnComponents()
  : CalculateMagnitudes(true)
  , NamingMode(svtkSplitColumnComponents::NUMBERS_WITH_PARENS)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//---------------------------------------------------------------------------
svtkSplitColumnComponents::~svtkSplitColumnComponents() = default;

//---------------------------------------------------------------------------
// Templated function in an anonymous namespace to copy the data from the
// specified component in one column to a single component column
namespace
{

template <typename T>
void CopyArrayData(T* source, T* destination, int components, int c, unsigned int length)
{
  for (unsigned int i = 0; i < length; ++i)
  {
    destination[i] = source[i * components + c];
  }
}

template <typename T>
void CalculateMagnitude(T* source, T* destination, int components, unsigned int length)
{
  for (unsigned int i = 0; i < length; ++i)
  {
    double tmp = 0.0;
    for (int j = 0; j < components; ++j)
    {
      tmp += static_cast<double>(source[i * components + j] * source[i * components + j]);
    }
    destination[i] = static_cast<T>(sqrt(tmp));
  }
}

} // End of anonymous namespace

//---------------------------------------------------------------------------
int svtkSplitColumnComponents::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get input tables
  svtkInformation* table1Info = inputVector[0]->GetInformationObject(0);
  svtkTable* table = svtkTable::SafeDownCast(table1Info->Get(svtkDataObject::DATA_OBJECT()));

  // Get output table
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkTable* output = svtkTable::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Add columns from table, split multiple component columns as necessary
  for (int i = 0; i < table->GetNumberOfColumns(); ++i)
  {
    svtkAbstractArray* col = table->GetColumn(i);
    if (col->GetName() == nullptr)
    {
      svtkWarningMacro("Skipping column with no name!");
      continue;
    }

    int components = col->GetNumberOfComponents();
    if (components == 1)
    {
      output->AddColumn(col);
    }
    else if (components > 1)
    {
      // Split the multicomponent column up into individual columns
      int colSize = col->GetNumberOfTuples();
      for (int j = 0; j < components; ++j)
      {
        const std::string component_label = this->GetComponentLabel(col, j);
        svtkAbstractArray* newCol = svtkAbstractArray::CreateArray(col->GetDataType());
        newCol->SetName(component_label.c_str());
        newCol->SetNumberOfTuples(colSize);
        // pass component name overrides, if provided.
        if (col->HasAComponentName())
        {
          newCol->SetComponentName(0, col->GetComponentName(j));
        }
        // Now copy the components into their new columns
        switch (col->GetDataType())
        {
          svtkExtraExtendedTemplateMacro(CopyArrayData(static_cast<SVTK_TT*>(col->GetVoidPointer(0)),
            static_cast<SVTK_TT*>(newCol->GetVoidPointer(0)), components, j, colSize));
        }
        if (auto info = newCol->GetInformation())
        {
          info->Set(ORIGINAL_ARRAY_NAME(), col->GetName());
          info->Set(ORIGINAL_COMPONENT_NUMBER(), j);
        }
        output->AddColumn(newCol);
        newCol->Delete();
      }
      // Add a magnitude column and calculate values if requested
      if (this->CalculateMagnitudes && col->IsA("svtkDataArray"))
      {
        std::string component_label = this->GetComponentLabel(col, -1 /* for magnitude */);
        svtkAbstractArray* newCol = svtkAbstractArray::CreateArray(col->GetDataType());
        newCol->SetName(component_label.c_str());
        newCol->SetNumberOfTuples(colSize);
        // Now calculate the magnitude column
        switch (col->GetDataType())
        {
          svtkTemplateMacro(CalculateMagnitude(static_cast<SVTK_TT*>(col->GetVoidPointer(0)),
            static_cast<SVTK_TT*>(newCol->GetVoidPointer(0)), components, colSize));
        }
        if (auto info = newCol->GetInformation())
        {
          info->Set(ORIGINAL_ARRAY_NAME(), col->GetName());
          info->Set(ORIGINAL_COMPONENT_NUMBER(), -1); // for magnitude
        }
        output->AddColumn(newCol);
        newCol->Delete();
      }
    }
  }
  return 1;
}

namespace
{
//----------------------------------------------------------------------------
std::string svtkDefaultComponentName(int componentNumber, int componentCount)
{
  if (componentCount <= 1)
  {
    return "";
  }
  else if (componentNumber == -1)
  {
    return "Magnitude";
  }
  else if (componentCount <= 3 && componentNumber < 3)
  {
    const char* titles[] = { "X", "Y", "Z" };
    return titles[componentNumber];
  }
  else if (componentCount == 6)
  {
    const char* titles[] = { "XX", "YY", "ZZ", "XY", "YZ", "XZ" };
    // Assume this is a symmetric matrix.
    return titles[componentNumber];
  }
  else
  {
    std::ostringstream buffer;
    buffer << componentNumber;
    return buffer.str();
  }
}
std::string svtkGetComponentName(svtkAbstractArray* array, int component_no)
{
  const char* name = array->GetComponentName(component_no);
  if (name)
  {
    return name;
  }
  return svtkDefaultComponentName(component_no, array->GetNumberOfComponents());
}
};

//---------------------------------------------------------------------------
std::string svtkSplitColumnComponents::GetComponentLabel(svtkAbstractArray* array, int component_no)
{
  std::ostringstream stream;
  switch (this->NamingMode)
  {
    case NUMBERS_WITH_PARENS:
      stream << array->GetName() << " (";
      if (component_no == -1)
      {
        stream << "Magnitude)";
      }
      else
      {
        stream << component_no << ")";
      }
      break;

    case NUMBERS_WITH_UNDERSCORES:
      stream << array->GetName() << "_";
      if (component_no == -1)
      {
        stream << "Magnitude";
      }
      else
      {
        stream << component_no;
      }
      break;

    case NAMES_WITH_PARENS:
      stream << array->GetName() << " (" << svtkGetComponentName(array, component_no).c_str() << ")";
      break;

    case NAMES_WITH_UNDERSCORES:
    default:
      stream << array->GetName() << "_" << svtkGetComponentName(array, component_no).c_str();
      break;
  }
  return stream.str();
}

//---------------------------------------------------------------------------
void svtkSplitColumnComponents::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CalculateMagnitudes: " << this->CalculateMagnitudes << endl;
  os << indent << "NamingMode: ";
  switch (this->NamingMode)
  {
    case NAMES_WITH_UNDERSCORES:
      os << "NAMES_WITH_UNDERSCORES" << endl;
      break;
    case NAMES_WITH_PARENS:
      os << "NAMES_WITH_PARENS" << endl;
      break;
    case NUMBERS_WITH_UNDERSCORES:
      os << "NUMBERS_WITH_UNDERSCORES" << endl;
      break;
    case NUMBERS_WITH_PARENS:
      os << "NUMBERS_WITH_PARENS" << endl;
      break;
    default:
      os << "INVALID" << endl;
  }
}
