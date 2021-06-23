/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPeriodicTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

#include "svtkPeriodicTable.h"

#include "svtkAbstractArray.h"
#include "svtkBlueObeliskData.h"
#include "svtkColor.h"
#include "svtkDebugLeaks.h"
#include "svtkFloatArray.h"
#include "svtkLookupTable.h"
#include "svtkMutexLock.h"
#include "svtkObjectFactory.h"
#include "svtkStdString.h"
#include "svtkStringArray.h"
#include "svtkUnsignedShortArray.h"

#include <cassert>
#include <cctype>
#include <cstring>
#include <string>

// Setup static variables
svtkNew<svtkBlueObeliskData> svtkPeriodicTable::BlueObeliskData;

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPeriodicTable);

//----------------------------------------------------------------------------
svtkPeriodicTable::svtkPeriodicTable()
{
  this->BlueObeliskData->GetWriteMutex()->Lock();

  if (!this->BlueObeliskData->IsInitialized())
  {
    this->BlueObeliskData->Initialize();
  }

  this->BlueObeliskData->GetWriteMutex()->Unlock();
}

//----------------------------------------------------------------------------
svtkPeriodicTable::~svtkPeriodicTable() = default;

//----------------------------------------------------------------------------
void svtkPeriodicTable::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "BlueObeliskData:\n";
  this->BlueObeliskData->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
unsigned short svtkPeriodicTable::GetNumberOfElements()
{
  return this->BlueObeliskData->GetNumberOfElements();
}

//----------------------------------------------------------------------------
const char* svtkPeriodicTable::GetSymbol(unsigned short atomicNum)
{
  if (atomicNum > this->GetNumberOfElements())
  {
    svtkWarningMacro("Atomic number out of range ! Using 0 instead of " << atomicNum);
    atomicNum = 0;
  }

  return this->BlueObeliskData->GetSymbols()->GetValue(atomicNum).c_str();
}

//----------------------------------------------------------------------------
const char* svtkPeriodicTable::GetElementName(unsigned short atomicNum)
{
  if (atomicNum > this->GetNumberOfElements())
  {
    svtkWarningMacro("Atomic number out of range ! Using 0 instead of " << atomicNum);
    atomicNum = 0;
  }

  return this->BlueObeliskData->GetNames()->GetValue(atomicNum).c_str();
}

//----------------------------------------------------------------------------
unsigned short svtkPeriodicTable::GetAtomicNumber(const svtkStdString& str)
{
  return this->GetAtomicNumber(str.c_str());
}

//----------------------------------------------------------------------------
unsigned short svtkPeriodicTable::GetAtomicNumber(const char* str)
{
  // If the string is null or the BODR object is not initialized, just
  // return 0.
  if (!str)
  {
    return 0;
  }

  // First attempt to just convert the string to an integer. If this
  // works, return the integer
  int atoi_num = atoi(str);
  if (atoi_num > 0 && atoi_num <= static_cast<int>(this->GetNumberOfElements()))
  {
    return static_cast<unsigned short>(atoi_num);
  }

  // Convert str to lowercase (see note about casts in
  // https://en.cppreference.com/w/cpp/string/byte/tolower)
  std::string lowerStr(str);
  std::transform(lowerStr.cbegin(), lowerStr.cend(), lowerStr.begin(),
    [](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });

  // Cache pointers:
  svtkStringArray* lnames = this->BlueObeliskData->GetLowerNames();
  svtkStringArray* lsymbols = this->BlueObeliskData->GetLowerSymbols();
  const unsigned short numElements = this->GetNumberOfElements();

  // Compare with other lowercase strings
  for (unsigned short ind = 0; ind <= numElements; ++ind)
  {
    if (lnames->GetValue(ind).compare(lowerStr) == 0 ||
      lsymbols->GetValue(ind).compare(lowerStr) == 0)
    {
      return ind;
    }
  }

  // Manually test some non-standard names:
  // - Deuterium
  if (lowerStr == "d" || lowerStr == "deuterium")
  {
    return 1;
  }
  // - Tritium
  else if (lowerStr == "t" || lowerStr == "tritium")
  {
    return 1;
  }
  // - Aluminum (vs. Aluminium)
  else if (lowerStr == "aluminum")
  {
    return 13;
  }

  return 0;
}

//----------------------------------------------------------------------------
float svtkPeriodicTable::GetCovalentRadius(unsigned short atomicNum)
{
  if (atomicNum > this->GetNumberOfElements())
  {
    svtkWarningMacro("Atomic number out of range ! Using 0 instead of " << atomicNum);
    atomicNum = 0;
  }

  return this->BlueObeliskData->GetCovalentRadii()->GetValue(atomicNum);
}

//----------------------------------------------------------------------------
float svtkPeriodicTable::GetVDWRadius(unsigned short atomicNum)
{
  if (atomicNum > this->GetNumberOfElements())
  {
    svtkWarningMacro("Atomic number out of range ! Using 0 instead of " << atomicNum);
    atomicNum = 0;
  }

  return this->BlueObeliskData->GetVDWRadii()->GetValue(atomicNum);
}

//----------------------------------------------------------------------------
float svtkPeriodicTable::GetMaxVDWRadius()
{
  float maxRadius = 0;
  for (unsigned short i = 0; i < this->GetNumberOfElements(); i++)
  {
    maxRadius = std::max(maxRadius, this->GetVDWRadius(i));
  }
  return maxRadius;
}

//----------------------------------------------------------------------------
void svtkPeriodicTable::GetDefaultLUT(svtkLookupTable* lut)
{
  const unsigned short numColors = this->GetNumberOfElements() + 1;
  svtkFloatArray* colors = this->BlueObeliskData->GetDefaultColors();
  lut->SetNumberOfColors(numColors);
  lut->SetIndexedLookup(true);
  float rgb[3];
  for (svtkIdType i = 0; static_cast<unsigned int>(i) < numColors; ++i)
  {
    colors->GetTypedTuple(i, rgb);
    lut->SetTableValue(i, rgb[0], rgb[1], rgb[2]);
    lut->SetAnnotation(i, this->GetSymbol(static_cast<unsigned short>(i)));
  }
}

//----------------------------------------------------------------------------
void svtkPeriodicTable::GetDefaultRGBTuple(unsigned short atomicNum, float rgb[3])
{
  this->BlueObeliskData->GetDefaultColors()->GetTypedTuple(atomicNum, rgb);
}

//----------------------------------------------------------------------------
svtkColor3f svtkPeriodicTable::GetDefaultRGBTuple(unsigned short atomicNum)
{
  svtkColor3f result;
  this->BlueObeliskData->GetDefaultColors()->GetTypedTuple(atomicNum, result.GetData());
  return result;
}
