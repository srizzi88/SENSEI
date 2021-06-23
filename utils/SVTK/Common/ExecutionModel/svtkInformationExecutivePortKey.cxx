/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInformationExecutivePortKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInformationExecutivePortKey.h"

#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkSmartPointer.h"

//----------------------------------------------------------------------------
svtkInformationExecutivePortKey::svtkInformationExecutivePortKey(
  const char* name, const char* location)
  : svtkInformationKey(name, location)
{
  svtkFilteringInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
svtkInformationExecutivePortKey::~svtkInformationExecutivePortKey() = default;

//----------------------------------------------------------------------------
void svtkInformationExecutivePortKey::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
class svtkInformationExecutivePortValue : public svtkObjectBase
{
public:
  svtkBaseTypeMacro(svtkInformationExecutivePortValue, svtkObjectBase);
  svtkSmartPointer<svtkExecutive> Executive;
  int Port;
};

//----------------------------------------------------------------------------
void svtkInformationExecutivePortKey::Set(svtkInformation* info, svtkExecutive* executive, int port)
{
  if (executive)
  {
    if (svtkInformationExecutivePortValue* oldv =
          static_cast<svtkInformationExecutivePortValue*>(this->GetAsObjectBase(info)))
    {
      // Replace the existing value.
      oldv->Executive = executive;
      oldv->Port = port;
      // Since this sets a value without call SetAsObjectBase(),
      // the info has to be modified here (instead of
      // svtkInformation::SetAsObjectBase()
      info->Modified();
    }
    else
    {
      // Allocate a new value.
      svtkInformationExecutivePortValue* v = new svtkInformationExecutivePortValue;
      v->InitializeObjectBase();
      v->Executive = executive;
      v->Port = port;
      this->SetAsObjectBase(info, v);
      v->Delete();
    }
  }
  else
  {
    this->SetAsObjectBase(info, nullptr);
  }
}

void svtkInformationExecutivePortKey::Get(svtkInformation* info, svtkExecutive*& executive, int& port)
{
  if (svtkInformationExecutivePortValue* v =
        static_cast<svtkInformationExecutivePortValue*>(this->GetAsObjectBase(info)))
  {
    executive = v->Executive;
    port = v->Port;
    return;
  }

  executive = nullptr;
  port = 0;
}

//----------------------------------------------------------------------------
svtkExecutive* svtkInformationExecutivePortKey::GetExecutive(svtkInformation* info)
{
  if (svtkInformationExecutivePortValue* v =
        static_cast<svtkInformationExecutivePortValue*>(this->GetAsObjectBase(info)))
  {
    return v->Executive;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
int svtkInformationExecutivePortKey::GetPort(svtkInformation* info)
{
  svtkInformationExecutivePortValue* v =
    static_cast<svtkInformationExecutivePortValue*>(this->GetAsObjectBase(info));
  return v ? v->Port : 0;
}

//----------------------------------------------------------------------------
void svtkInformationExecutivePortKey::ShallowCopy(svtkInformation* from, svtkInformation* to)
{
  this->Set(to, this->GetExecutive(from), this->GetPort(from));
}

//----------------------------------------------------------------------------
void svtkInformationExecutivePortKey::Print(ostream& os, svtkInformation* info)
{
  // Print the value.
  if (this->Has(info))
  {
    svtkExecutive* executive = this->GetExecutive(info);
    int port = this->GetPort(info);
    if (executive)
    {
      os << executive->GetClassName() << "(" << executive << ") port " << port;
    }
    else
    {
      os << "(nullptr) port " << port;
    }
  }
}

//----------------------------------------------------------------------------
void svtkInformationExecutivePortKey::Report(svtkInformation* info, svtkGarbageCollector* collector)
{
  if (svtkInformationExecutivePortValue* v =
        static_cast<svtkInformationExecutivePortValue*>(this->GetAsObjectBase(info)))
  {
    v->Executive.Report(collector, this->GetName());
  }
}
