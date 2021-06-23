/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInformationExecutivePortVectorKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInformationExecutivePortVectorKey.h"

#include "svtkExecutive.h"
#include "svtkGarbageCollector.h"
#include "svtkInformation.h"

#include <algorithm>
#include <vector>

// should the pipeline be double or singly linked (referenced) list, single
// make garbage collecting easier but results in a weak reference.
#define SVTK_USE_SINGLE_REF 1

//----------------------------------------------------------------------------
svtkInformationExecutivePortVectorKey::svtkInformationExecutivePortVectorKey(
  const char* name, const char* location)
  : svtkInformationKey(name, location)
{
  svtkFilteringInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
svtkInformationExecutivePortVectorKey::~svtkInformationExecutivePortVectorKey() = default;

//----------------------------------------------------------------------------
void svtkInformationExecutivePortVectorKey::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
class svtkInformationExecutivePortVectorValue : public svtkObjectBase
{
public:
  svtkBaseTypeMacro(svtkInformationExecutivePortVectorValue, svtkObjectBase);
  std::vector<svtkExecutive*> Executives;
  std::vector<int> Ports;

  ~svtkInformationExecutivePortVectorValue() override;
  void UnRegisterAllExecutives();
};

//----------------------------------------------------------------------------
svtkInformationExecutivePortVectorValue ::~svtkInformationExecutivePortVectorValue()
{
  // Remove all our references to executives before erasing the vector.
  this->UnRegisterAllExecutives();
}

//----------------------------------------------------------------------------
void svtkInformationExecutivePortVectorValue::UnRegisterAllExecutives()
{
#ifndef SVTK_USE_SINGLE_REF
  for (std::vector<svtkExecutive*>::iterator i = this->Executives.begin();
       i != this->Executives.end(); ++i)
  {
    if (svtkExecutive* e = *i)
    {
      e->UnRegister(0);
    }
  }
#endif
}

//----------------------------------------------------------------------------
void svtkInformationExecutivePortVectorKey::Append(
  svtkInformation* info, svtkExecutive* executive, int port)
{
  if (svtkInformationExecutivePortVectorValue* v =
        static_cast<svtkInformationExecutivePortVectorValue*>(this->GetAsObjectBase(info)))
  {
    // The entry already exists.  Append to its vector.
#ifndef SVTK_USE_SINGLE_REF
    executive->Register(0);
#endif
    v->Executives.push_back(executive);
    v->Ports.push_back(port);
  }
  else
  {
    // The entry does not yet exist.  Just create it.
    this->Set(info, &executive, &port, 1);
  }
}

//----------------------------------------------------------------------------
void svtkInformationExecutivePortVectorKey::Remove(
  svtkInformation* info, svtkExecutive* executive, int port)
{
  if (svtkInformationExecutivePortVectorValue* v =
        static_cast<svtkInformationExecutivePortVectorValue*>(this->GetAsObjectBase(info)))
  {
    // The entry exists.  Find this executive/port pair and remove it.
    for (unsigned int i = 0; i < v->Executives.size(); ++i)
    {
      if (v->Executives[i] == executive && v->Ports[i] == port)
      {
        v->Executives.erase(v->Executives.begin() + i);
        v->Ports.erase(v->Ports.begin() + i);
#ifndef SVTK_USE_SINGLE_REF
        executive->UnRegister(0);
#endif
        break;
      }
    }

    // If the last entry was removed, remove the entire value.
    if (v->Executives.empty())
    {
      this->SetAsObjectBase(info, nullptr);
    }
  }
}

//----------------------------------------------------------------------------
void svtkInformationExecutivePortVectorKey::Set(
  svtkInformation* info, svtkExecutive** executives, int* ports, int length)
{
  if (executives && ports && length > 0)
  {
#ifndef SVTK_USE_SINGLE_REF
    // Register our references to all the given executives.
    for (int i = 0; i < length; ++i)
    {
      if (executives[i])
      {
        executives[i]->Register(0);
      }
    }
#endif
    // Store the vector of pointers.
    svtkInformationExecutivePortVectorValue* oldv =
      static_cast<svtkInformationExecutivePortVectorValue*>(this->GetAsObjectBase(info));
    if (oldv && static_cast<int>(oldv->Executives.size()) == length)
    {
      // Replace the existing value.
      oldv->UnRegisterAllExecutives();
      std::copy(executives, executives + length, oldv->Executives.begin());
      std::copy(ports, ports + length, oldv->Ports.begin());
      // Since this sets a value without call SetAsObjectBase(),
      // the info has to be modified here (instead of
      // svtkInformation::SetAsObjectBase()
      info->Modified();
    }
    else
    {
      // Allocate a new value.
      svtkInformationExecutivePortVectorValue* v = new svtkInformationExecutivePortVectorValue;
      v->InitializeObjectBase();
      v->Executives.insert(v->Executives.begin(), executives, executives + length);
      v->Ports.insert(v->Ports.begin(), ports, ports + length);
      this->SetAsObjectBase(info, v);
      v->Delete();
    }
  }
  else
  {
    this->SetAsObjectBase(info, nullptr);
  }
}

//----------------------------------------------------------------------------
svtkExecutive** svtkInformationExecutivePortVectorKey::GetExecutives(svtkInformation* info)
{
  svtkInformationExecutivePortVectorValue* v =
    static_cast<svtkInformationExecutivePortVectorValue*>(this->GetAsObjectBase(info));
  return (v && !v->Executives.empty()) ? (&v->Executives[0]) : nullptr;
}

//----------------------------------------------------------------------------
int* svtkInformationExecutivePortVectorKey::GetPorts(svtkInformation* info)
{
  svtkInformationExecutivePortVectorValue* v =
    static_cast<svtkInformationExecutivePortVectorValue*>(this->GetAsObjectBase(info));
  return (v && !v->Ports.empty()) ? (&v->Ports[0]) : nullptr;
}

//----------------------------------------------------------------------------
void svtkInformationExecutivePortVectorKey::Get(
  svtkInformation* info, svtkExecutive** executives, int* ports)
{
  if (svtkInformationExecutivePortVectorValue* v =
        static_cast<svtkInformationExecutivePortVectorValue*>(this->GetAsObjectBase(info)))
  {
    std::copy(v->Executives.begin(), v->Executives.end(), executives);
    std::copy(v->Ports.begin(), v->Ports.end(), ports);
  }
}

//----------------------------------------------------------------------------
int svtkInformationExecutivePortVectorKey::Length(svtkInformation* info)
{
  svtkInformationExecutivePortVectorValue* v =
    static_cast<svtkInformationExecutivePortVectorValue*>(this->GetAsObjectBase(info));
  return v ? static_cast<int>(v->Executives.size()) : 0;
}

//----------------------------------------------------------------------------
void svtkInformationExecutivePortVectorKey::ShallowCopy(svtkInformation* from, svtkInformation* to)
{
  this->Set(to, this->GetExecutives(from), this->GetPorts(from), this->Length(from));
}

//----------------------------------------------------------------------------
void svtkInformationExecutivePortVectorKey::Remove(svtkInformation* info)
{
  this->Superclass::Remove(info);
}

//----------------------------------------------------------------------------
void svtkInformationExecutivePortVectorKey::Print(ostream& os, svtkInformation* info)
{
  // Print the value.
  if (this->Has(info))
  {
    svtkExecutive** executives = this->GetExecutives(info);
    int* ports = this->GetPorts(info);
    int length = this->Length(info);
    const char* sep = "";
    for (int i = 0; i < length; ++i)
    {
      if (executives[i])
      {
        os << sep << executives[i]->GetClassName() << "(" << executives[i] << ") port " << ports[i];
      }
      else
      {
        os << sep << "(nullptr) port " << ports[i];
      }
      sep = ", ";
    }
  }
}

//----------------------------------------------------------------------------
void
#ifdef SVTK_USE_SINGLE_REF
svtkInformationExecutivePortVectorKey::Report(svtkInformation*,
                                             svtkGarbageCollector*)
{
#else
svtkInformationExecutivePortVectorKey::Report(svtkInformation* info,
                                             svtkGarbageCollector* collector)
{
  if (svtkInformationExecutivePortVectorValue* v =
        static_cast<svtkInformationExecutivePortVectorValue*>(this->GetAsObjectBase(info)))
  {
    for (std::vector<svtkExecutive*>::iterator i = v->Executives.begin(); i != v->Executives.end();
         ++i)
    {
      svtkGarbageCollectorReport(collector, *i, this->GetName());
    }
  }
#endif
}

//----------------------------------------------------------------------------
svtkExecutive** svtkInformationExecutivePortVectorKey ::GetExecutivesWatchAddress(
  svtkInformation* info)
{
  svtkInformationExecutivePortVectorValue* v =
    static_cast<svtkInformationExecutivePortVectorValue*>(this->GetAsObjectBase(info));
  return (v && !v->Executives.empty()) ? (&v->Executives[0]) : nullptr;
}

//----------------------------------------------------------------------------
int* svtkInformationExecutivePortVectorKey ::GetPortsWatchAddress(svtkInformation* info)
{
  svtkInformationExecutivePortVectorValue* v =
    static_cast<svtkInformationExecutivePortVectorValue*>(this->GetAsObjectBase(info));
  return (v && !v->Ports.empty()) ? (&v->Ports[0]) : nullptr;
}
