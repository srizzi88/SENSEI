/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMaskFields.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMaskFields.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include <cctype>

svtkStandardNewMacro(svtkMaskFields);

char svtkMaskFields::FieldLocationNames[3][12] = { "OBJECT_DATA", "POINT_DATA", "CELL_DATA" };
char svtkMaskFields::AttributeNames[svtkDataSetAttributes::NUM_ATTRIBUTES][10] = { { 0 } };

svtkMaskFields::svtkMaskFields()
{

  this->CopyFieldFlags = nullptr;
  this->NumberOfFieldFlags = 0;
  this->CopyAllOn();

  // convert the attribute names to uppercase for local use
  if (svtkMaskFields::AttributeNames[0][0] == 0)
  {
    for (int i = 0; i < svtkDataSetAttributes::NUM_ATTRIBUTES; i++)
    {
      int l = static_cast<int>(strlen(svtkDataSetAttributes::GetAttributeTypeAsString(i)));
      for (int c = 0; c < l && c < 10; c++)
      {
        svtkMaskFields::AttributeNames[i][c] =
          toupper(svtkDataSetAttributes::GetAttributeTypeAsString(i)[c]);
      }
    }
  }
}

svtkMaskFields::~svtkMaskFields()
{

  this->ClearFieldFlags();
}

void svtkMaskFields::CopyFieldOnOff(int fieldLocation, const char* field, int onOff)
{
  if (!field)
  {
    return;
  }

  int index;
  // If the array is in the list, simply set IsCopied to onOff
  if ((index = this->FindFlag(field, fieldLocation)) != -1)
  {
    this->CopyFieldFlags[index].IsCopied = onOff;
  }
  else
  {
    // We need to reallocate the list of fields
    svtkMaskFields::CopyFieldFlag* newFlags =
      new svtkMaskFields::CopyFieldFlag[this->NumberOfFieldFlags + 1];
    // Copy old flags (pointer copy for name)
    for (int i = 0; i < this->NumberOfFieldFlags; i++)
    {
      newFlags[i].Name = this->CopyFieldFlags[i].Name;
      newFlags[i].Type = this->CopyFieldFlags[i].Type;
      newFlags[i].Location = this->CopyFieldFlags[i].Location;
      newFlags[i].IsCopied = this->CopyFieldFlags[i].IsCopied;
    }
    // Copy new flag (strcpy)
    char* newName = new char[strlen(field) + 1];
    strcpy(newName, field);
    newFlags[this->NumberOfFieldFlags].Name = newName;
    newFlags[this->NumberOfFieldFlags].Type = -1;
    newFlags[this->NumberOfFieldFlags].Location = fieldLocation;
    newFlags[this->NumberOfFieldFlags].IsCopied = onOff;
    this->NumberOfFieldFlags++;
    delete[] this->CopyFieldFlags;
    this->CopyFieldFlags = newFlags;
  }
  this->Modified();
}

void svtkMaskFields::CopyAttributeOnOff(int attributeLocation, int attributeType, int onOff)
{

  int index;
  // If the array is in the list, simply set IsCopied to onOff
  if ((index = this->FindFlag(attributeType, attributeLocation)) != -1)
  {
    this->CopyFieldFlags[index].IsCopied = onOff;
  }
  else
  {
    // We need to reallocate the list of fields
    svtkMaskFields::CopyFieldFlag* newFlags =
      new svtkMaskFields::CopyFieldFlag[this->NumberOfFieldFlags + 1];
    // Copy old flags (pointer copy for name)
    for (int i = 0; i < this->NumberOfFieldFlags; i++)
    {
      newFlags[i].Name = this->CopyFieldFlags[i].Name;
      newFlags[i].Type = this->CopyFieldFlags[i].Type;
      newFlags[i].Location = this->CopyFieldFlags[i].Location;
      newFlags[i].IsCopied = this->CopyFieldFlags[i].IsCopied;
    }
    // Copy new flag
    newFlags[this->NumberOfFieldFlags].Name = nullptr;
    newFlags[this->NumberOfFieldFlags].Type = attributeType;
    newFlags[this->NumberOfFieldFlags].Location = attributeLocation;
    newFlags[this->NumberOfFieldFlags].IsCopied = onOff;
    this->NumberOfFieldFlags++;
    delete[] this->CopyFieldFlags;
    this->CopyFieldFlags = newFlags;
  }
  this->Modified();
}

int svtkMaskFields::GetAttributeLocation(const char* attributeLoc)
{
  int numAttributeLocs = 3;
  int loc = -1;

  if (!attributeLoc)
  {
    return loc;
  }

  for (int i = 0; i < numAttributeLocs; i++)
  {
    if (!strcmp(attributeLoc, FieldLocationNames[i]))
    {
      loc = i;
      break;
    }
  }
  return loc;
}

int svtkMaskFields::GetAttributeType(const char* attributeType)
{

  int numAttr = svtkDataSetAttributes::NUM_ATTRIBUTES;
  int attrType = -1;

  if (!attributeType)
  {
    return attrType;
  }

  for (int i = 0; i < numAttr; i++)
  {
    if (!strcmp(attributeType, AttributeNames[i]))
    {
      attrType = i;
      break;
    }
  }
  return attrType;
}

void svtkMaskFields::CopyAttributeOn(const char* attributeLoc, const char* attributeType)
{
  if (!attributeType || !attributeLoc)
  {
    return;
  }

  // Convert strings to ints and call the appropriate CopyAttributeOn()

  int attrType = this->GetAttributeType(attributeType);
  if (attrType == -1)
  {
    svtkErrorMacro("Target attribute type is invalid.");
    return;
  }

  int loc = this->GetAttributeLocation(attributeLoc);
  if (loc == -1)
  {
    svtkErrorMacro("Target location for the attribute is invalid.");
    return;
  }

  this->CopyAttributeOn(loc, attrType);
}

void svtkMaskFields::CopyAttributeOff(const char* attributeLoc, const char* attributeType)
{
  if (!attributeType || !attributeLoc)
  {
    return;
  }

  // Convert strings to ints and call the appropriate CopyAttributeOn()

  int attrType = this->GetAttributeType(attributeType);
  if (attrType == -1)
  {
    svtkErrorMacro("Target attribute type is invalid.");
    return;
  }

  int loc = this->GetAttributeLocation(attributeLoc);
  if (loc == -1)
  {
    svtkErrorMacro("Target location for the attribute is invalid.");
    return;
  }

  this->CopyAttributeOff(loc, attrType);
}

void svtkMaskFields::CopyFieldOn(const char* fieldLoc, const char* name)
{
  if (!name || !fieldLoc)
  {
    return;
  }

  // Convert strings to ints and call the appropriate CopyAttributeOn()
  int loc = this->GetAttributeLocation(fieldLoc);
  if (loc == -1)
  {
    svtkErrorMacro("Target location for the attribute is invalid.");
    return;
  }

  this->CopyFieldOn(loc, name);
}

void svtkMaskFields::CopyFieldOff(const char* fieldLoc, const char* name)
{
  if (!name || !fieldLoc)
  {
    return;
  }

  // Convert strings to ints and call the appropriate CopyAttributeOn()
  int loc = this->GetAttributeLocation(fieldLoc);
  if (loc == -1)
  {
    svtkErrorMacro("Target location for the attribute is invalid.");
    return;
  }

  this->CopyFieldOff(loc, name);
}

// Turn on copying of all data.
void svtkMaskFields::CopyAllOn()
{
  this->CopyFields = 1;
  this->CopyAttributes = 1;
  this->Modified();
}

// Turn off copying of all data.
void svtkMaskFields::CopyAllOff()
{
  this->CopyFields = 0;
  this->CopyAttributes = 0;
  this->Modified();
}

// Deallocate and clear the list of fields.
void svtkMaskFields::ClearFieldFlags()
{
  if (this->NumberOfFieldFlags > 0)
  {
    for (int i = 0; i < this->NumberOfFieldFlags; i++)
    {
      delete[] this->CopyFieldFlags[i].Name;
    }
  }
  delete[] this->CopyFieldFlags;
  this->CopyFieldFlags = nullptr;
  this->NumberOfFieldFlags = 0;
}

// Find if field is in CopyFieldFlags.
// If it is, it returns the index otherwise it returns -1
int svtkMaskFields::FindFlag(const char* field, int loc)
{
  if (!field)
    return -1;
  for (int i = 0; i < this->NumberOfFieldFlags; i++)
  {
    if (this->CopyFieldFlags[i].Name && !strcmp(field, this->CopyFieldFlags[i].Name) &&
      this->CopyFieldFlags[i].Location == loc)
    {
      return i;
    }
  }
  return -1;
}

// Find if field is in CopyFieldFlags.
// If it is, it returns the index otherwise it returns -1
int svtkMaskFields::FindFlag(int attributeType, int loc)
{
  for (int i = 0; i < this->NumberOfFieldFlags; i++)
  {
    if (this->CopyFieldFlags[i].Type == attributeType && this->CopyFieldFlags[i].Location == loc)
    {
      return i;
    }
  }
  return -1;
}

// If there is no flag for this array, return -1.
// If there is one: return 0 if off, 1 if on
int svtkMaskFields::GetFlag(const char* field, int loc)
{
  int index = this->FindFlag(field, loc);
  if (index == -1)
  {
    return -1;
  }
  else
  {
    return this->CopyFieldFlags[index].IsCopied;
  }
}

// If there is no flag for this array, return -1.
// If there is one: return 0 if off, 1 if on
int svtkMaskFields::GetFlag(int arrayType, int loc)
{
  int index = this->FindFlag(arrayType, loc);
  if (index == -1)
  {
    return -1;
  }
  else
  {
    return this->CopyFieldFlags[index].IsCopied;
  }
}

int svtkMaskFields::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // This has to be here because it initializes all field data.
  output->CopyStructure(input);

  if (this->CopyFields && this->CopyAttributes)
  {
    svtkDebugMacro("Copying both fields and attributes.");
    output->GetPointData()->CopyAllOn();
    output->GetCellData()->CopyAllOn();

    output->GetFieldData()->CopyAllOn();
  }
  else if (!this->CopyFields && this->CopyAttributes)
  {
    svtkDebugMacro("Copying only attributes.");

    output->GetPointData()->CopyAllOff();
    output->GetCellData()->CopyAllOff();
    int ai;
    for (ai = 0; ai < svtkDataSetAttributes::NUM_ATTRIBUTES; ai++)
    {
      output->GetPointData()->SetCopyAttribute(1, ai);
      output->GetCellData()->SetCopyAttribute(1, ai);
    }
  }
  else if (this->CopyFields && !this->CopyAttributes)
  {
    svtkDebugMacro("Copying only fields.");
    output->GetPointData()->CopyAllOn();
    output->GetCellData()->CopyAllOn();
    int ai;
    for (ai = 0; ai < svtkDataSetAttributes::NUM_ATTRIBUTES; ai++)
    {
      output->GetPointData()->SetCopyAttribute(0, ai);
      output->GetCellData()->SetCopyAttribute(0, ai);
    }
    output->GetFieldData()->CopyAllOn();
  }
  else if (!this->CopyFields && !this->CopyAttributes)
  {
    svtkDebugMacro("Global copying off for fields and attributes.");
    output->GetPointData()->CopyAllOff();
    output->GetCellData()->CopyAllOff();

    output->GetFieldData()->CopyAllOff();
  }

  // individual flags take precedence, so all of above will be
  // overridden by individual flags...
  for (int i = 0; i < this->NumberOfFieldFlags; ++i)
  {

    switch (this->CopyFieldFlags[i].Location)
    {
      case svtkMaskFields::POINT_DATA:

        if (this->CopyFieldFlags[i].Type > -1)
        { // attribute data
          output->GetPointData()->SetCopyAttribute(
            this->CopyFieldFlags[i].Type, this->CopyFieldFlags[i].IsCopied);
        }
        else
        { // field data
          if (this->CopyFieldFlags[i].IsCopied == 1)
          {
            output->GetPointData()->CopyFieldOn(this->CopyFieldFlags[i].Name);
          }
          else
          {
            output->GetPointData()->CopyFieldOff(this->CopyFieldFlags[i].Name);
          }
        }
        break;
      case svtkMaskFields::CELL_DATA:
        if (this->CopyFieldFlags[i].Type > -1)
        { // attribute data
          output->GetCellData()->SetCopyAttribute(
            this->CopyFieldFlags[i].Type, this->CopyFieldFlags[i].IsCopied);
        }
        else
        { // field data
          if (this->CopyFieldFlags[i].IsCopied == 1)
          {
            output->GetCellData()->CopyFieldOn(this->CopyFieldFlags[i].Name);
          }
          else
          {
            output->GetCellData()->CopyFieldOff(this->CopyFieldFlags[i].Name);
          }
        }
        break;
      case svtkMaskFields::OBJECT_DATA:
        if (this->CopyFieldFlags[i].IsCopied == 1)
        {
          output->GetFieldData()->CopyFieldOn(this->CopyFieldFlags[i].Name);
        }
        else
        {
          output->GetFieldData()->CopyFieldOff(this->CopyFieldFlags[i].Name);
        }
        break;
      default:
        svtkErrorMacro("unknown location field");
        break;
    }
  }

  // Pass all.
  if (output->GetFieldData() && input->GetFieldData())
  {
    output->GetFieldData()->PassData(input->GetFieldData());
  }
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  return 1;
}

void svtkMaskFields::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number of field flags: " << this->NumberOfFieldFlags << endl;

  os << indent << "CopyFields: " << this->CopyFields << endl;

  os << indent << "CopyAttributes: " << this->CopyAttributes << endl;
}
