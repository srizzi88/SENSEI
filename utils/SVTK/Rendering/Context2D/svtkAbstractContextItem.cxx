/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractContextItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkAbstractContextItem.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScenePrivate.h"
#include "svtkObjectFactory.h"

// STL headers
#include <algorithm>

//-----------------------------------------------------------------------------
svtkAbstractContextItem::svtkAbstractContextItem()
{
  this->Scene = nullptr;
  this->Parent = nullptr;
  this->Children = new svtkContextScenePrivate(this);
  this->Visible = true;
  this->Interactive = true;
}

//-----------------------------------------------------------------------------
svtkAbstractContextItem::~svtkAbstractContextItem()
{
  delete this->Children;
}

//-----------------------------------------------------------------------------
bool svtkAbstractContextItem::Paint(svtkContext2D* painter)
{
  this->Children->PaintItems(painter);
  return true;
}

//-----------------------------------------------------------------------------
bool svtkAbstractContextItem::PaintChildren(svtkContext2D* painter)
{
  this->Children->PaintItems(painter);
  return true;
}

//-----------------------------------------------------------------------------
void svtkAbstractContextItem::Update() {}

//-----------------------------------------------------------------------------
svtkIdType svtkAbstractContextItem::AddItem(svtkAbstractContextItem* item)
{
  return this->Children->AddItem(item);
}

//-----------------------------------------------------------------------------
bool svtkAbstractContextItem::RemoveItem(svtkAbstractContextItem* item)
{
  return this->Children->RemoveItem(item);
}

//-----------------------------------------------------------------------------
bool svtkAbstractContextItem::RemoveItem(svtkIdType index)
{
  if (index >= 0 && index < static_cast<svtkIdType>(this->Children->size()))
  {
    return this->Children->RemoveItem(index);
  }
  else
  {
    return false;
  }
}

//-----------------------------------------------------------------------------
svtkAbstractContextItem* svtkAbstractContextItem::GetItem(svtkIdType index)
{
  if (index >= 0 && index < static_cast<svtkIdType>(this->Children->size()))
  {
    return this->Children->at(index);
  }
  else
  {
    return nullptr;
  }
}

//-----------------------------------------------------------------------------
svtkIdType svtkAbstractContextItem::GetItemIndex(svtkAbstractContextItem* item)
{
  svtkContextScenePrivate::const_iterator it =
    std::find(this->Children->begin(), this->Children->end(), item);
  if (it == this->Children->end())
  {
    return -1;
  }
  return it - this->Children->begin();
}

//-----------------------------------------------------------------------------
svtkIdType svtkAbstractContextItem::GetNumberOfItems()
{
  return static_cast<svtkIdType>(this->Children->size());
}

//-----------------------------------------------------------------------------
void svtkAbstractContextItem::ClearItems()
{
  this->Children->Clear();
}

//-----------------------------------------------------------------------------
svtkIdType svtkAbstractContextItem::Raise(svtkIdType index)
{
  return this->StackAbove(index, this->GetNumberOfItems() - 1);
}

//-----------------------------------------------------------------------------
svtkIdType svtkAbstractContextItem::StackAbove(svtkIdType index, svtkIdType under)
{
  svtkIdType res = index;
  if (index == under || index < 0)
  {
    return res;
  }
  svtkIdType start = 0;
  svtkIdType middle = 0;
  svtkIdType end = 0;
  if (under == -1)
  {
    start = 0;
    middle = index;
    end = index + 1;
    res = 0;
  }
  else if (index > under)
  {
    start = under + 1;
    middle = index;
    end = index + 1;
    res = start;
  }
  else // if (index < under)
  {
    start = index;
    middle = index + 1;
    end = under + 1;
    res = end - 1;
  }
  std::rotate(this->Children->begin() + start, this->Children->begin() + middle,
    this->Children->begin() + end);
  return res;
}

//-----------------------------------------------------------------------------
svtkIdType svtkAbstractContextItem::Lower(svtkIdType index)
{
  return this->StackUnder(index, 0);
}

//-----------------------------------------------------------------------------
svtkIdType svtkAbstractContextItem::StackUnder(svtkIdType child, svtkIdType above)
{
  return this->StackAbove(child, above - 1);
}

//-----------------------------------------------------------------------------
bool svtkAbstractContextItem::Hit(const svtkContextMouseEvent&)
{
  return false;
}

//-----------------------------------------------------------------------------
bool svtkAbstractContextItem::MouseEnterEvent(const svtkContextMouseEvent&)
{
  return false;
}

//-----------------------------------------------------------------------------
bool svtkAbstractContextItem::MouseMoveEvent(const svtkContextMouseEvent&)
{
  return false;
}

//-----------------------------------------------------------------------------
bool svtkAbstractContextItem::MouseLeaveEvent(const svtkContextMouseEvent&)
{
  return false;
}

//-----------------------------------------------------------------------------
bool svtkAbstractContextItem::MouseButtonPressEvent(const svtkContextMouseEvent&)
{
  return false;
}

//-----------------------------------------------------------------------------
bool svtkAbstractContextItem::MouseButtonReleaseEvent(const svtkContextMouseEvent&)
{
  return false;
}

//-----------------------------------------------------------------------------
bool svtkAbstractContextItem::MouseDoubleClickEvent(const svtkContextMouseEvent&)
{
  return false;
}

//-----------------------------------------------------------------------------
bool svtkAbstractContextItem::MouseWheelEvent(const svtkContextMouseEvent&, int)
{
  return false;
}

//-----------------------------------------------------------------------------
bool svtkAbstractContextItem::KeyPressEvent(const svtkContextKeyEvent&)
{
  return false;
}

//-----------------------------------------------------------------------------
bool svtkAbstractContextItem::KeyReleaseEvent(const svtkContextKeyEvent&)
{
  return false;
}

// ----------------------------------------------------------------------------
svtkAbstractContextItem* svtkAbstractContextItem::GetPickedItem(const svtkContextMouseEvent& mouse)
{
  svtkContextMouseEvent childMouse = mouse;
  childMouse.SetPos(this->MapFromParent(mouse.GetPos()));
  childMouse.SetLastPos(this->MapFromParent(mouse.GetLastPos()));
  for (svtkContextScenePrivate::const_reverse_iterator it = this->Children->rbegin();
       it != this->Children->rend(); ++it)
  {
    svtkAbstractContextItem* item = (*it)->GetPickedItem(childMouse);
    if (item)
    {
      return item;
    }
  }
  return this->Hit(mouse) ? this : nullptr;
}

// ----------------------------------------------------------------------------
void svtkAbstractContextItem::ReleaseGraphicsResources()
{
  for (svtkContextScenePrivate::const_iterator it = this->Children->begin();
       it != this->Children->end(); ++it)
  {
    (*it)->ReleaseGraphicsResources();
  }
}

// ----------------------------------------------------------------------------
void svtkAbstractContextItem::SetScene(svtkContextScene* scene)
{
  this->Scene = scene;
  this->Children->SetScene(scene);
}

// ----------------------------------------------------------------------------
void svtkAbstractContextItem::SetParent(svtkAbstractContextItem* parent)
{
  this->Parent = parent;
}

// ----------------------------------------------------------------------------
svtkVector2f svtkAbstractContextItem::MapToParent(const svtkVector2f& point)
{
  return point;
}

// ----------------------------------------------------------------------------
svtkVector2f svtkAbstractContextItem::MapFromParent(const svtkVector2f& point)
{
  return point;
}

// ----------------------------------------------------------------------------
svtkVector2f svtkAbstractContextItem::MapToScene(const svtkVector2f& point)
{
  if (this->Parent)
  {
    svtkVector2f p = this->MapToParent(point);
    p = this->Parent->MapToScene(p);
    return p;
  }
  else
  {
    return this->MapToParent(point);
  }
}

// ----------------------------------------------------------------------------
svtkVector2f svtkAbstractContextItem::MapFromScene(const svtkVector2f& point)
{
  if (this->Parent)
  {
    svtkVector2f p = this->Parent->MapFromScene(point);
    p = this->MapFromParent(p);
    return p;
  }
  else
  {
    return this->MapFromParent(point);
  }
}

//-----------------------------------------------------------------------------
void svtkAbstractContextItem::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
