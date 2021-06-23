/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSeedRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSeedRepresentation.h"

#include "svtkActor2D.h"
#include "svtkCoordinate.h"
#include "svtkHandleRepresentation.h"
#include "svtkInteractorObserver.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProperty2D.h"
#include "svtkRenderer.h"
#include "svtkTextProperty.h"
#include <iterator>
#include <list>

svtkStandardNewMacro(svtkSeedRepresentation);

svtkCxxSetObjectMacro(svtkSeedRepresentation, HandleRepresentation, svtkHandleRepresentation);

// The svtkHandleList is a PIMPLed list<T>.
class svtkHandleList : public std::list<svtkHandleRepresentation*>
{
};
typedef std::list<svtkHandleRepresentation*>::iterator svtkHandleListIterator;

//----------------------------------------------------------------------
svtkSeedRepresentation::svtkSeedRepresentation()
{
  this->HandleRepresentation = nullptr;

  // The representation for the seed handles
  this->Handles = new svtkHandleList;
  this->ActiveHandle = -1;

  this->Tolerance = 5;
}

//----------------------------------------------------------------------
svtkSeedRepresentation::~svtkSeedRepresentation()
{
  if (this->HandleRepresentation)
  {
    this->HandleRepresentation->Delete();
  }

  // Loop over all handles releasing their observes and deleting them
  svtkHandleListIterator iter;
  for (iter = this->Handles->begin(); iter != this->Handles->end(); ++iter)
  {
    (*iter)->Delete();
  }
  delete this->Handles;
}

//----------------------------------------------------------------------
svtkHandleRepresentation* svtkSeedRepresentation ::GetHandleRepresentation(unsigned int num)
{
  if (num < this->Handles->size())
  {
    svtkHandleListIterator iter = this->Handles->begin();
    std::advance(iter, num);
    return (*iter);
  }
  else // create one
  {
    if (this->HandleRepresentation == nullptr)
    {
      svtkErrorMacro("GetHandleRepresentation "
        << num << ", no handle representation has been set yet, cannot create a new handle.");
      return nullptr;
    }
    svtkHandleRepresentation* rep = this->HandleRepresentation->NewInstance();
    rep->DeepCopy(this->HandleRepresentation);
    this->Handles->push_back(rep);
    return rep;
  }
}

//----------------------------------------------------------------------
void svtkSeedRepresentation::GetSeedWorldPosition(unsigned int seedNum, double pos[3])
{
  if (seedNum >= this->Handles->size())
  {
    svtkErrorMacro("Trying to access non-existent handle");
    return;
  }
  svtkHandleListIterator iter = this->Handles->begin();
  std::advance(iter, seedNum);
  (*iter)->GetWorldPosition(pos);
}

//----------------------------------------------------------------------------
void svtkSeedRepresentation::SetSeedWorldPosition(unsigned int seedNum, double pos[3])
{
  if (seedNum >= this->Handles->size())
  {
    svtkErrorMacro("Trying to access non-existent handle");
    return;
  }
  svtkHandleListIterator iter = this->Handles->begin();
  std::advance(iter, seedNum);
  (*iter)->SetWorldPosition(pos);
}

//----------------------------------------------------------------------
void svtkSeedRepresentation::SetSeedDisplayPosition(unsigned int seedNum, double pos[3])
{
  if (seedNum >= this->Handles->size())
  {
    svtkErrorMacro("Trying to access non-existent handle");
    return;
  }
  svtkHandleListIterator iter = this->Handles->begin();
  std::advance(iter, seedNum);
  (*iter)->SetDisplayPosition(pos);
}

//----------------------------------------------------------------------
void svtkSeedRepresentation::GetSeedDisplayPosition(unsigned int seedNum, double pos[3])
{
  if (seedNum >= this->Handles->size())
  {
    svtkErrorMacro("Trying to access non-existent handle");
    return;
  }
  svtkHandleListIterator iter = this->Handles->begin();
  std::advance(iter, seedNum);
  (*iter)->GetDisplayPosition(pos);
}

//----------------------------------------------------------------------
int svtkSeedRepresentation::GetNumberOfSeeds()
{
  return static_cast<int>(this->Handles->size());
}

//----------------------------------------------------------------------
int svtkSeedRepresentation::ComputeInteractionState(
  int svtkNotUsed(X), int svtkNotUsed(Y), int svtkNotUsed(modify))
{
  // Loop over all the seeds to see if the point is close to any of them.
  int i;
  svtkHandleListIterator iter;
  for (i = 0, iter = this->Handles->begin(); iter != this->Handles->end(); ++iter, ++i)
  {
    if (*iter != nullptr)
    {
      if ((*iter)->GetInteractionState() != svtkHandleRepresentation::Outside)
      {
        this->ActiveHandle = i;
        this->InteractionState = svtkSeedRepresentation::NearSeed;
        return this->InteractionState;
      }
    }
  }

  // Nothing found, so it's outside
  this->InteractionState = svtkSeedRepresentation::Outside;
  return this->InteractionState;
}

//----------------------------------------------------------------------
int svtkSeedRepresentation::GetActiveHandle()
{
  return this->ActiveHandle;
}

//----------------------------------------------------------------------
void svtkSeedRepresentation::SetActiveHandle(int handleId)
{
  if (handleId >= static_cast<int>(this->Handles->size()))
  {
    return;
  }
  this->ActiveHandle = handleId;
}

//----------------------------------------------------------------------
int svtkSeedRepresentation::CreateHandle(double e[2])
{
  double pos[3];
  pos[0] = e[0];
  pos[1] = e[1];
  pos[2] = 0.0;

  svtkHandleRepresentation* rep =
    this->GetHandleRepresentation(static_cast<int>(this->Handles->size()));
  if (rep == nullptr)
  {
    svtkErrorMacro("CreateHandle: no handle representation set yet! Cannot create a new handle.");
    return -1;
  }
  rep->SetDisplayPosition(pos);
  rep->SetTolerance(this->Tolerance); // needed to ensure that picking is consistent
  this->ActiveHandle = static_cast<int>(this->Handles->size()) - 1;
  return this->ActiveHandle;
}

//----------------------------------------------------------------------
void svtkSeedRepresentation::RemoveLastHandle()
{
  if (this->Handles->empty())
  {
    return;
  }

  // Delete last handle
  this->Handles->back()->Delete();
  this->Handles->pop_back();
}

//----------------------------------------------------------------------
void svtkSeedRepresentation::RemoveHandle(int n)
{
  // Remove nth handle

  if (n == this->ActiveHandle)
  {
    this->RemoveActiveHandle();
    return;
  }

  if (static_cast<int>(this->Handles->size()) <= n)
  {
    return;
  }

  svtkHandleListIterator iter = this->Handles->begin();
  std::advance(iter, n);
  svtkHandleRepresentation* hr = *iter;
  this->Handles->erase(iter);
  hr->Delete();
}

//----------------------------------------------------------------------
void svtkSeedRepresentation::RemoveActiveHandle()
{
  if (this->Handles->empty())
  {
    return;
  }
  if (this->ActiveHandle >= 0 && this->ActiveHandle < static_cast<int>(this->Handles->size()))
  {
    svtkHandleListIterator iter = this->Handles->begin();
    std::advance(iter, this->ActiveHandle);
    svtkHandleRepresentation* hr = *iter;
    this->Handles->erase(iter);
    hr->Delete();
    this->ActiveHandle = -1;
  }
}

//----------------------------------------------------------------------
void svtkSeedRepresentation::BuildRepresentation()
{
  if (this->ActiveHandle >= 0 && this->ActiveHandle < static_cast<int>(this->Handles->size()))
  {
    svtkHandleRepresentation* rep = this->GetHandleRepresentation(this->ActiveHandle);
    if (rep)
    {
      rep->BuildRepresentation();
    }
  }
}

//----------------------------------------------------------------------
void svtkSeedRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Number of Seeds: " << this->GetNumberOfSeeds() << "\n";
}
