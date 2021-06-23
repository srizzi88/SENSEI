/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMedicalImageReader2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMedicalImageReader2.h"
#include "svtkObjectFactory.h"

#include "svtkMedicalImageProperties.h"

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkMedicalImageReader2);

//----------------------------------------------------------------------------
svtkMedicalImageReader2::svtkMedicalImageReader2()
{
  this->MedicalImageProperties = svtkMedicalImageProperties::New();
}

//----------------------------------------------------------------------------
svtkMedicalImageReader2::~svtkMedicalImageReader2()
{
  if (this->MedicalImageProperties)
  {
    this->MedicalImageProperties->Delete();
    this->MedicalImageProperties = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkMedicalImageReader2::SetPatientName(const char* arg)
{
  if (this->MedicalImageProperties)
  {
    this->MedicalImageProperties->SetPatientName(arg);
  }
}

//----------------------------------------------------------------------------
const char* svtkMedicalImageReader2::GetPatientName()
{
  if (this->MedicalImageProperties)
  {
    return this->MedicalImageProperties->GetPatientName();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkMedicalImageReader2::SetPatientID(const char* arg)
{
  if (this->MedicalImageProperties)
  {
    this->MedicalImageProperties->SetPatientID(arg);
  }
}

//----------------------------------------------------------------------------
const char* svtkMedicalImageReader2::GetPatientID()
{
  if (this->MedicalImageProperties)
  {
    return this->MedicalImageProperties->GetPatientID();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkMedicalImageReader2::SetDate(const char* arg)
{
  if (this->MedicalImageProperties)
  {
    this->MedicalImageProperties->SetImageDate(arg);
  }
}

//----------------------------------------------------------------------------
const char* svtkMedicalImageReader2::GetDate()
{
  if (this->MedicalImageProperties)
  {
    return this->MedicalImageProperties->GetImageDate();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkMedicalImageReader2::SetSeries(const char* arg)
{
  if (this->MedicalImageProperties)
  {
    this->MedicalImageProperties->SetSeriesNumber(arg);
  }
}

//----------------------------------------------------------------------------
const char* svtkMedicalImageReader2::GetSeries()
{
  if (this->MedicalImageProperties)
  {
    return this->MedicalImageProperties->GetSeriesNumber();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkMedicalImageReader2::SetStudy(const char* arg)
{
  if (this->MedicalImageProperties)
  {
    this->MedicalImageProperties->SetStudyID(arg);
  }
}

//----------------------------------------------------------------------------
const char* svtkMedicalImageReader2::GetStudy()
{
  if (this->MedicalImageProperties)
  {
    return this->MedicalImageProperties->GetStudyID();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkMedicalImageReader2::SetImageNumber(const char* arg)
{
  if (this->MedicalImageProperties)
  {
    this->MedicalImageProperties->SetImageNumber(arg);
  }
}

//----------------------------------------------------------------------------
const char* svtkMedicalImageReader2::GetImageNumber()
{
  if (this->MedicalImageProperties)
  {
    return this->MedicalImageProperties->GetImageNumber();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkMedicalImageReader2::SetModality(const char* arg)
{
  if (this->MedicalImageProperties)
  {
    this->MedicalImageProperties->SetModality(arg);
  }
}

//----------------------------------------------------------------------------
const char* svtkMedicalImageReader2::GetModality()
{
  if (this->MedicalImageProperties)
  {
    return this->MedicalImageProperties->GetModality();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkMedicalImageReader2::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->MedicalImageProperties)
  {
    os << indent << "Medical Image Properties:\n";
    this->MedicalImageProperties->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "MedicalImageProperties: (none)\n";
  }
}
