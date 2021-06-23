/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBlueObeliskDataParser.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

#include "svtkBlueObeliskDataParser.h"

#include "svtkAbstractArray.h"
#include "svtkBlueObeliskData.h"
#include "svtkFloatArray.h"
#include "svtkObjectFactory.h"
#include "svtkStdString.h"
#include "svtkStringArray.h"
#include "svtkUnsignedShortArray.h"

#include <svtksys/SystemTools.hxx>

// Defines SVTK_BODR_DATA_PATH
#include "svtkChemistryConfigure.h"

#include <locale>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#ifdef MSC_VER
#define stat _stat
#endif

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkBlueObeliskDataParser);

//----------------------------------------------------------------------------
svtkBlueObeliskDataParser::svtkBlueObeliskDataParser()
  : svtkXMLParser()
  , Target(nullptr)
  , IsProcessingAtom(false)
  , IsProcessingValue(false)
  , CurrentValueType(None)
  , CurrentSymbol(new svtkStdString)
  , CurrentName(new svtkStdString)
  , CurrentPeriodicTableBlock(new svtkStdString)
  , CurrentElectronicConfiguration(new svtkStdString)
  , CurrentFamily(new svtkStdString)
{
}

//----------------------------------------------------------------------------
svtkBlueObeliskDataParser::~svtkBlueObeliskDataParser()
{
  this->SetTarget(nullptr);
  delete CurrentSymbol;
  delete CurrentName;
  delete CurrentPeriodicTableBlock;
  delete CurrentElectronicConfiguration;
  delete CurrentFamily;
}

//----------------------------------------------------------------------------
void svtkBlueObeliskDataParser::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkBlueObeliskDataParser::SetTarget(svtkBlueObeliskData* bodr)
{
  svtkSetObjectBodyMacro(Target, svtkBlueObeliskData, bodr);
}

//----------------------------------------------------------------------------
int svtkBlueObeliskDataParser::Parse()
{
  if (!this->Target)
  {
    svtkWarningMacro(<< "No target set. Aborting.");
    return 0;
  }

  // Setup BlueObeliskData arrays
  this->Target->Reset();
  this->Target->Allocate(119); // 118 elements + dummy (0)

  int ret = this->Superclass::Parse();

  this->Target->Squeeze();

  // Set number of elements to the length of the symbol array minus
  // one (index 0 is a dummy atom type)
  this->Target->NumberOfElements = this->Target->Symbols->GetNumberOfTuples() - 1;

  return ret;
}

//----------------------------------------------------------------------------
int svtkBlueObeliskDataParser::Parse(const char*)
{
  return this->Parse();
}

//----------------------------------------------------------------------------
int svtkBlueObeliskDataParser::Parse(const char*, unsigned int)
{
  return this->Parse();
}

//----------------------------------------------------------------------------
void svtkBlueObeliskDataParser::StartElement(const char* name, const char** attr)
{
  if (this->GetDebug())
  {
    std::string desc;
    desc += "Encountered BODR Element. Name: ";
    desc += name;
    desc += "\n\tAttributes: ";
    int attrIndex = 0;
    while (const char* cur = attr[attrIndex])
    {
      if (attrIndex > 0)
      {
        desc.push_back(' ');
      }
      desc += cur;
      ++attrIndex;
    }
    svtkDebugMacro(<< desc);
  }

  if (strcmp(name, "atom") == 0)
  {
    this->NewAtomStarted(attr);
  }
  else if (strcmp(name, "scalar") == 0 || strcmp(name, "label") == 0 || strcmp(name, "array") == 0)
  {
    this->NewValueStarted(attr);
  }
  else if (this->GetDebug())
  {
    svtkDebugMacro(<< "Unhandled BODR element: " << name);
  }
}

//----------------------------------------------------------------------------
void svtkBlueObeliskDataParser::EndElement(const char* name)
{
  if (strcmp(name, "atom") == 0)
  {
    this->NewAtomFinished();
  }
  else if (strcmp(name, "scalar") == 0 || strcmp(name, "label") == 0 || strcmp(name, "array") == 0)
  {
    this->NewValueFinished();
  }
}

//----------------------------------------------------------------------------
void svtkBlueObeliskDataParser::NewAtomStarted(const char**)
{
  this->CurrentAtomicNumber = -1;
  this->CurrentSymbol->clear();
  this->CurrentName->clear();
  this->CurrentPeriodicTableBlock->clear();
  this->CurrentElectronicConfiguration->clear();
  this->CurrentFamily->clear();
  this->CurrentMass = SVTK_FLOAT_MAX;
  this->CurrentExactMass = SVTK_FLOAT_MAX;
  this->CurrentIonizationEnergy = SVTK_FLOAT_MAX;
  this->CurrentElectronAffinity = SVTK_FLOAT_MAX;
  this->CurrentPaulingElectronegativity = SVTK_FLOAT_MAX;
  this->CurrentCovalentRadius = SVTK_FLOAT_MAX;
  this->CurrentVDWRadius = SVTK_FLOAT_MAX;
  this->CurrentDefaultColor[0] = 0.0;
  this->CurrentDefaultColor[1] = 0.0;
  this->CurrentDefaultColor[2] = 0.0;
  this->CurrentBoilingPoint = SVTK_FLOAT_MAX;
  this->CurrentMeltingPoint = SVTK_FLOAT_MAX;
  this->CurrentPeriod = SVTK_UNSIGNED_SHORT_MAX;
  this->CurrentGroup = SVTK_UNSIGNED_SHORT_MAX;

  this->CurrentValueType = None;

  this->IsProcessingAtom = true;
}

//----------------------------------------------------------------------------
void svtkBlueObeliskDataParser::NewAtomFinished()
{
  if (this->CurrentAtomicNumber < 0)
  {
    svtkWarningMacro(<< "Skipping invalid atom...");
    this->IsProcessingAtom = false;
    return;
  }

  svtkDebugMacro(<< "Adding info for atomic number: " << this->CurrentAtomicNumber);

  svtkIdType index = static_cast<svtkIdType>(this->CurrentAtomicNumber);

  this->ResizeAndSetValue(this->CurrentSymbol, this->Target->Symbols, index);
  // this->ToLower will modify the input string, so this must follow
  // this->Symbol
  this->ResizeAndSetValue(this->ToLower(this->CurrentSymbol), this->Target->LowerSymbols, index);
  this->ResizeAndSetValue(this->CurrentName, this->Target->Names, index);
  // this->ToLower will modify the input string, so this must follow
  // this->Name
  this->ResizeAndSetValue(this->ToLower(this->CurrentName), this->Target->LowerNames, index);
  this->ResizeAndSetValue(
    this->CurrentPeriodicTableBlock, this->Target->PeriodicTableBlocks, index);
  this->ResizeAndSetValue(
    this->CurrentElectronicConfiguration, this->Target->ElectronicConfigurations, index);
  this->ResizeAndSetValue(this->CurrentFamily, this->Target->Families, index);
  this->ResizeAndSetValue(this->CurrentMass, this->Target->Masses, index);
  this->ResizeAndSetValue(this->CurrentExactMass, this->Target->ExactMasses, index);
  this->ResizeAndSetValue(this->CurrentIonizationEnergy, this->Target->IonizationEnergies, index);
  this->ResizeAndSetValue(this->CurrentElectronAffinity, this->Target->ElectronAffinities, index);
  this->ResizeAndSetValue(
    this->CurrentPaulingElectronegativity, this->Target->PaulingElectronegativities, index);
  this->ResizeAndSetValue(this->CurrentCovalentRadius, this->Target->CovalentRadii, index);
  this->ResizeAndSetValue(this->CurrentVDWRadius, this->Target->VDWRadii, index);
  // Tuple handled differently
  this->ResizeArrayIfNeeded(this->Target->DefaultColors, index);
  this->Target->DefaultColors->SetTypedTuple(index, this->CurrentDefaultColor);
  this->ResizeAndSetValue(this->CurrentBoilingPoint, this->Target->BoilingPoints, index);
  this->ResizeAndSetValue(this->CurrentMeltingPoint, this->Target->MeltingPoints, index);
  this->ResizeAndSetValue(this->CurrentPeriod, this->Target->Periods, index);
  this->ResizeAndSetValue(this->CurrentGroup, this->Target->Groups, index);
  this->IsProcessingAtom = false;
}

//----------------------------------------------------------------------------
void svtkBlueObeliskDataParser::NewValueStarted(const char** attr)
{
  this->IsProcessingValue = true;
  unsigned int attrInd = 0;

  while (const char* cur = attr[attrInd])
  {
    if (strcmp(cur, "value") == 0)
    {
      this->SetCurrentValue(attr[++attrInd]);
    }
    else if (strcmp(cur, "bo:atomicNumber") == 0)
    {
      this->CurrentValueType = AtomicNumber;
    }
    else if (strcmp(cur, "bo:symbol") == 0)
    {
      this->CurrentValueType = Symbol;
    }
    else if (strcmp(cur, "bo:name") == 0)
    {
      this->CurrentValueType = Name;
    }
    else if (strcmp(cur, "bo:periodTableBlock") == 0)
    {
      this->CurrentValueType = PeriodicTableBlock;
    }
    else if (strcmp(cur, "bo:electronicConfiguration") == 0)
    {
      this->CurrentValueType = ElectronicConfiguration;
    }
    else if (strcmp(cur, "bo:family") == 0)
    {
      this->CurrentValueType = Family;
    }
    else if (strcmp(cur, "bo:mass") == 0)
    {
      this->CurrentValueType = Mass;
    }
    else if (strcmp(cur, "bo:exactMass") == 0)
    {
      this->CurrentValueType = ExactMass;
    }
    else if (strcmp(cur, "bo:ionization") == 0)
    {
      this->CurrentValueType = IonizationEnergy;
    }
    else if (strcmp(cur, "bo:electronAffinity") == 0)
    {
      this->CurrentValueType = ElectronAffinity;
    }
    else if (strcmp(cur, "bo:electronegativityPauling") == 0)
    {
      this->CurrentValueType = PaulingElectronegativity;
    }
    else if (strcmp(cur, "bo:radiusCovalent") == 0)
    {
      this->CurrentValueType = CovalentRadius;
    }
    else if (strcmp(cur, "bo:radiusVDW") == 0)
    {
      this->CurrentValueType = VDWRadius;
    }
    else if (strcmp(cur, "bo:elementColor") == 0)
    {
      this->CurrentValueType = DefaultColor;
    }
    else if (strcmp(cur, "bo:boilingpoint") == 0)
    {
      this->CurrentValueType = BoilingPoint;
    }
    else if (strcmp(cur, "bo:meltingpoint") == 0)
    {
      this->CurrentValueType = MeltingPoint;
    }
    else if (strcmp(cur, "bo:period") == 0)
    {
      this->CurrentValueType = Period;
    }
    else if (strcmp(cur, "bo:group") == 0)
    {
      this->CurrentValueType = Group;
    }
    ++attrInd;
  }
}

//----------------------------------------------------------------------------
void svtkBlueObeliskDataParser::NewValueFinished()
{
  this->CurrentValueType = None;
  this->IsProcessingValue = false;
  this->CharacterDataValueBuffer.clear();
}

//----------------------------------------------------------------------------
void svtkBlueObeliskDataParser::CharacterDataHandler(const char* data, int length)
{
  if (this->IsProcessingAtom && this->IsProcessingValue)
  {
    this->SetCurrentValue(data, length);
  }
}

//----------------------------------------------------------------------------
void svtkBlueObeliskDataParser::SetCurrentValue(const char* data, int length)
{
  this->CharacterDataValueBuffer += std::string(data, data + length);

  this->SetCurrentValue(this->CharacterDataValueBuffer.c_str());
}

//----------------------------------------------------------------------------
void svtkBlueObeliskDataParser::SetCurrentValue(const char* data)
{
  svtkDebugMacro(<< "Parsing string '" << data << "' for datatype " << this->CurrentValueType
                << ".");
  switch (this->CurrentValueType)
  {
    case AtomicNumber:
      this->CurrentAtomicNumber = this->parseInt(data);
      return;
    case Symbol:
      this->CurrentSymbol->assign(data);
      return;
    case Name:
      this->CurrentName->assign(data);
      return;
    case PeriodicTableBlock:
      this->CurrentPeriodicTableBlock->assign(data);
      return;
    case ElectronicConfiguration:
      this->CurrentElectronicConfiguration->assign(data);
      return;
    case Family:
      this->CurrentFamily->assign(data);
      return;
    case Mass:
      this->CurrentMass = this->parseFloat(data);
      return;
    case ExactMass:
      this->CurrentExactMass = this->parseFloat(data);
      return;
    case IonizationEnergy:
      this->CurrentIonizationEnergy = this->parseFloat(data);
      return;
    case ElectronAffinity:
      this->CurrentElectronAffinity = this->parseFloat(data);
      return;
    case PaulingElectronegativity:
      this->CurrentPaulingElectronegativity = this->parseFloat(data);
      return;
    case CovalentRadius:
      this->CurrentCovalentRadius = this->parseFloat(data);
      return;
    case VDWRadius:
      this->CurrentVDWRadius = this->parseFloat(data);
      return;
    case DefaultColor:
      this->parseFloat3(data, this->CurrentDefaultColor);
      return;
    case BoilingPoint:
      this->CurrentBoilingPoint = this->parseFloat(data);
      return;
    case MeltingPoint:
      this->CurrentMeltingPoint = this->parseFloat(data);
      return;
    case Period:
      this->CurrentPeriod = this->parseUnsignedShort(data);
      return;
    case Group:
      this->CurrentGroup = this->parseUnsignedShort(data);
      return;
    case None:
    default:
      svtkDebugMacro(<< "Called with no CurrentValueType. data: " << data);
  }
}

//----------------------------------------------------------------------------
void svtkBlueObeliskDataParser::ResizeArrayIfNeeded(svtkAbstractArray* arr, svtkIdType ind)
{
  if (ind >= arr->GetNumberOfTuples())
  {
    arr->SetNumberOfTuples(ind + 1);
  }
}

//----------------------------------------------------------------------------
void svtkBlueObeliskDataParser::ResizeAndSetValue(
  svtkStdString* val, svtkStringArray* arr, svtkIdType ind)
{
  svtkBlueObeliskDataParser::ResizeArrayIfNeeded(arr, ind);
  arr->SetValue(ind, val->c_str());
}

//----------------------------------------------------------------------------
void svtkBlueObeliskDataParser::ResizeAndSetValue(float val, svtkFloatArray* arr, svtkIdType ind)
{
  svtkBlueObeliskDataParser::ResizeArrayIfNeeded(arr, ind);
  arr->SetValue(ind, val);
}

//----------------------------------------------------------------------------
void svtkBlueObeliskDataParser::ResizeAndSetValue(
  unsigned short val, svtkUnsignedShortArray* arr, svtkIdType ind)
{
  svtkBlueObeliskDataParser::ResizeArrayIfNeeded(arr, ind);
  arr->SetValue(ind, val);
}

//----------------------------------------------------------------------------
inline int svtkBlueObeliskDataParser::parseInt(const char* d)
{
  return atoi(d);
}

//----------------------------------------------------------------------------
inline float svtkBlueObeliskDataParser::parseFloat(const char* d)
{
  float value;
  std::stringstream stream(d);
  stream >> value;

  if (stream.fail())
  {
    return 0.f;
  }

  return value;
}

//----------------------------------------------------------------------------
inline void svtkBlueObeliskDataParser::parseFloat3(const char* str, float arr[3])
{
  unsigned short ind = 0;

  std::vector<std::string> tokens;
  svtksys::SystemTools::Split(str, tokens, ' ');

  for (auto&& tok : tokens)
  {
    arr[ind++] = std::stof(tok);
  }

  if (ind != 3)
  {
    arr[0] = arr[1] = arr[2] == SVTK_FLOAT_MAX;
  }
}

//----------------------------------------------------------------------------
inline unsigned short svtkBlueObeliskDataParser::parseUnsignedShort(const char* d)
{
  return static_cast<unsigned short>(atoi(d));
}

//----------------------------------------------------------------------------
inline svtkStdString* svtkBlueObeliskDataParser::ToLower(svtkStdString* str)
{
  for (svtkStdString::iterator it = str->begin(), it_end = str->end(); it != it_end; ++it)
  {
    *it = static_cast<char>(tolower(*it));
  }
  return str;
}
