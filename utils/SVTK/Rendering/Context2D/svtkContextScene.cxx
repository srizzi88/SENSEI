/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextScene.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkContextScene.h"

#include "svtkContext2D.h"
#include "svtkContextItem.h"
#include "svtkContextKeyEvent.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScenePrivate.h"
#include "svtkMatrix3x3.h"
#include "svtkNew.h"
#include "svtkTransform2D.h"

// Get my new commands
#include "svtkCommand.h"

#include "svtkAbstractContextBufferId.h"
#include "svtkAnnotationLink.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

// My STL containers
#include <cassert>

//-----------------------------------------------------------------------------
// Minimal storage class for STL containers etc.
class svtkContextScene::Private
{
public:
  Private()
  {
    this->Event.SetButton(svtkContextMouseEvent::NO_BUTTON);
    this->IsDirty = true;
  }
  ~Private() = default;

  // The item with a current mouse down
  svtkWeakPointer<svtkAbstractContextItem> itemMousePressCurrent;
  // Item the mouse was last over
  svtkWeakPointer<svtkAbstractContextItem> itemPicked;
  // Mouse event structure
  svtkContextMouseEvent Event;
  bool IsDirty;
};

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkContextScene);
svtkCxxSetObjectMacro(svtkContextScene, AnnotationLink, svtkAnnotationLink);

//-----------------------------------------------------------------------------
svtkContextScene::svtkContextScene()
{
  this->Storage = new Private;
  this->AnnotationLink = nullptr;
  this->Geometry[0] = 0;
  this->Geometry[1] = 0;
  this->BufferId = nullptr;
  this->BufferIdDirty = true;
  this->BufferIdSupportTested = false;
  this->BufferIdSupported = false;
  this->UseBufferId = true;
  this->ScaleTiles = true;
  this->Transform = nullptr;
  this->Children = new svtkContextScenePrivate(nullptr);
  this->Children->SetScene(this);
}

//-----------------------------------------------------------------------------
svtkContextScene::~svtkContextScene()
{
  delete this->Storage;
  this->Storage = nullptr;
  this->SetAnnotationLink(nullptr);
  if (this->BufferId != nullptr)
  {
    this->BufferId->Delete();
  }
  if (this->Transform)
  {
    this->Transform->Delete();
  }
  delete this->Children;
}

//-----------------------------------------------------------------------------
void svtkContextScene::SetRenderer(svtkRenderer* r)
{
  this->Renderer = r;
  this->BufferIdSupportTested = false;
}

//-----------------------------------------------------------------------------
svtkRenderer* svtkContextScene::GetRenderer()
{
  return this->Renderer;
}

//-----------------------------------------------------------------------------
bool svtkContextScene::Paint(svtkContext2D* painter)
{
  svtkDebugMacro("Paint event called.");
  size_t size = this->Children->size();
  if (size && this->Transform)
  {
    painter->PushMatrix();
    painter->SetTransform(this->Transform);
  }
  this->Children->PaintItems(painter);
  if (size && this->Transform)
  {
    painter->PopMatrix();
  }
  if (this->Storage->IsDirty)
  {
    this->BufferIdDirty = true;
  }
  this->Storage->IsDirty = false;
  this->LastPainter = painter;
  return true;
}

//-----------------------------------------------------------------------------
void svtkContextScene::PaintIds()
{
  svtkDebugMacro("PaintId called.");
  size_t size = this->Children->size();

  if (size > 16777214) // 24-bit limit, 0 reserved for background encoding.
  {
    svtkWarningMacro(<< "picking will not work properly as there are two many items. Items over "
                       "16777214 will be ignored.");
    size = 16777214;
  }
  for (size_t i = 0; i < size; ++i)
  {
    this->LastPainter->ApplyId(static_cast<svtkIdType>(i + 1));
    (*this->Children)[i]->Paint(this->LastPainter);
  }
  this->Storage->IsDirty = false;
}

//-----------------------------------------------------------------------------
unsigned int svtkContextScene::AddItem(svtkAbstractContextItem* item)
{
  return this->Children->AddItem(item);
}

//-----------------------------------------------------------------------------
bool svtkContextScene::RemoveItem(svtkAbstractContextItem* item)
{
  return this->Children->RemoveItem(item);
}

//-----------------------------------------------------------------------------
bool svtkContextScene::RemoveItem(unsigned int index)
{
  return this->Children->RemoveItem(index);
}

//-----------------------------------------------------------------------------
svtkAbstractContextItem* svtkContextScene::GetItem(unsigned int index)
{
  if (index < this->Children->size())
  {
    return this->Children->at(index);
  }
  else
  {
    return nullptr;
  }
}

//-----------------------------------------------------------------------------
unsigned int svtkContextScene::GetNumberOfItems()
{
  return static_cast<unsigned int>(this->Children->size());
}

//-----------------------------------------------------------------------------
void svtkContextScene::ClearItems()
{
  this->Children->Clear();
}

//-----------------------------------------------------------------------------
int svtkContextScene::GetViewWidth()
{
  if (this->Renderer)
  {
    return this->Renderer->GetRenderWindow()->GetSize()[0];
  }
  else
  {
    return 0;
  }
}

//-----------------------------------------------------------------------------
int svtkContextScene::GetViewHeight()
{
  if (this->Renderer)
  {
    return this->Renderer->GetRenderWindow()->GetSize()[1];
  }
  else
  {
    return 0;
  }
}

//-----------------------------------------------------------------------------
int svtkContextScene::GetSceneWidth()
{
  return this->Geometry[0];
}

//-----------------------------------------------------------------------------
int svtkContextScene::GetSceneHeight()
{
  return this->Geometry[1];
}

//-----------------------------------------------------------------------------
svtkVector2i svtkContextScene::GetLogicalTileScale()
{
  svtkVector2i result(1);
  if (this->ScaleTiles && this->Renderer && this->Renderer->GetRenderWindow())
  {
    this->Renderer->GetRenderWindow()->GetTileScale(result.GetData());
  }
  return result;
}

//-----------------------------------------------------------------------------
void svtkContextScene::SetDirty(bool isDirty)
{
  if (this->Storage->IsDirty == isDirty)
  {
    return;
  }
  this->Storage->IsDirty = isDirty;
  if (this->Storage->IsDirty)
  {
    this->BufferIdDirty = true;
  }
  this->Modified();
}

//-----------------------------------------------------------------------------
bool svtkContextScene::GetDirty() const
{
  return this->Storage->IsDirty;
}

// ----------------------------------------------------------------------------
void svtkContextScene::ReleaseGraphicsResources()
{
  if (this->BufferId != nullptr)
  {
    this->BufferId->ReleaseGraphicsResources();
  }
  for (svtkContextScenePrivate::const_iterator it = this->Children->begin();
       it != this->Children->end(); ++it)
  {
    (*it)->ReleaseGraphicsResources();
  }
}

//-----------------------------------------------------------------------------
svtkWeakPointer<svtkContext2D> svtkContextScene::GetLastPainter()
{
  return this->LastPainter;
}

//-----------------------------------------------------------------------------
svtkAbstractContextBufferId* svtkContextScene::GetBufferId()
{
  return this->BufferId;
}

//-----------------------------------------------------------------------------
void svtkContextScene::SetTransform(svtkTransform2D* transform)
{
  if (this->Transform == transform)
  {
    return;
  }
  this->Transform->Delete();
  this->Transform = transform;
  this->Transform->Register(this);
}

//-----------------------------------------------------------------------------
svtkTransform2D* svtkContextScene::GetTransform()
{
  if (this->Transform)
  {
    return this->Transform;
  }
  else
  {
    this->Transform = svtkTransform2D::New();
    return this->Transform;
  }
}

//-----------------------------------------------------------------------------
bool svtkContextScene::ProcessSelectionEvent(unsigned int rect[5])
{
  cout << "ProcessSelectionEvent called! " << endl;
  cout << "Rect:";
  for (int i = 0; i < 5; ++i)
  {
    cout << "\t" << rect[i];
  }
  cout << endl;
  return false;
}

// ----------------------------------------------------------------------------
void svtkContextScene::TestBufferIdSupport()
{
  if (!this->BufferIdSupportTested)
  {
    svtkNew<svtkAbstractContextBufferId> b;
    b->SetContext(this->Renderer->GetRenderWindow());
    this->BufferIdSupported = b->IsSupported();
    b->ReleaseGraphicsResources();
    this->BufferIdSupportTested = true;
  }
}

// ----------------------------------------------------------------------------
void svtkContextScene::UpdateBufferId()
{
  int lowerLeft[2];
  int width;
  int height;
  this->Renderer->GetTiledSizeAndOrigin(&width, &height, lowerLeft, lowerLeft + 1);

  if (this->BufferId == nullptr || this->BufferIdDirty || width != this->BufferId->GetWidth() ||
    height != this->BufferId->GetHeight())
  {
    if (this->BufferId == nullptr)
    {
      this->BufferId = svtkAbstractContextBufferId::New();
      this->BufferId->SetContext(this->Renderer->GetRenderWindow());
    }
    this->BufferId->SetWidth(width);
    this->BufferId->SetHeight(height);
    this->BufferId->Allocate();

    this->LastPainter->BufferIdModeBegin(this->BufferId);
    this->PaintIds();
    this->LastPainter->BufferIdModeEnd();

    this->BufferIdDirty = false;
  }
}

// ----------------------------------------------------------------------------
svtkAbstractContextItem* svtkContextScene::GetPickedItem()
{
  svtkContextMouseEvent& event = this->Storage->Event;
  for (svtkContextScenePrivate::const_reverse_iterator it = this->Children->rbegin();
       it != this->Children->rend(); ++it)
  {
    svtkAbstractContextItem* item = (*it)->GetPickedItem(event);
    if (item)
    {
      return item;
    }
  }
  return nullptr;
}

// ----------------------------------------------------------------------------
svtkIdType svtkContextScene::GetPickedItem(int x, int y)
{
  svtkIdType result = -1;
  this->TestBufferIdSupport();
  if (this->UseBufferId && this->BufferIdSupported)
  {
    this->UpdateBufferId();
    result = this->BufferId->GetPickedItem(x, y);
  }
  else
  {
    size_t i = this->Children->size() - 1;
    svtkContextMouseEvent& event = this->Storage->Event;
    for (svtkContextScenePrivate::const_reverse_iterator it = this->Children->rbegin();
         it != this->Children->rend(); ++it, --i)
    {
      if ((*it)->Hit(event))
      {
        result = static_cast<svtkIdType>(i);
        break;
      }
    }
  }

  // Work-around for Qt bug under Linux (and maybe other platforms), 4.5.2
  // or 4.6.2 :
  // when the cursor leaves the window, Qt returns an extra mouse move event
  // with coordinates out of the window area. The problem is that the pixel
  // underneath is not owned by the OpenGL context, hence the bufferid contains
  // garbage (see OpenGL pixel ownership test).
  // As a workaround, any value out of the scope of
  // [-1,this->GetNumberOfItems()-1] is set to -1 (<=> no hit)

  if (result < -1 || result >= static_cast<svtkIdType>(this->GetNumberOfItems()))
  {
    result = -1;
  }

  assert("post: valid_result" && result >= -1 &&
    result < static_cast<svtkIdType>(this->GetNumberOfItems()));
  return result;
}

//-----------------------------------------------------------------------------
bool svtkContextScene::MouseMoveEvent(const svtkContextMouseEvent& e)
{
  bool res = false;
  svtkContextMouseEvent& event = this->Storage->Event;
  this->EventCopy(e);

  svtkAbstractContextItem* newItemPicked = this->GetPickedItem();
#if 0
  if (newItemPicked)
  {
    cerr << "picked a " << newItemPicked->GetClassName() << endl;
  }
  else
  {
    cerr << "picked nothing" << endl;
  }
#endif
  if (this->Storage->itemPicked != newItemPicked)
  {
    if (this->Storage->itemPicked)
    {
      // Make sure last picked object is still part of this scene.
      if (this->Storage->itemPicked->GetScene() == this)
      {
        svtkAbstractContextItem* cur = this->Storage->itemPicked;
        res = this->ProcessItem(cur, event, &svtkAbstractContextItem::MouseLeaveEvent) || res;
      }
    }
    if (newItemPicked)
    {
      svtkAbstractContextItem* cur = newItemPicked;
      res = this->ProcessItem(cur, event, &svtkAbstractContextItem::MouseEnterEvent) || res;
    }
  }

  this->Storage->itemPicked = newItemPicked;

  // Fire mouse move event regardless of where it occurred.

  // Check if there is a selected item that needs to receive a move event
  if (this->Storage->itemMousePressCurrent &&
    this->Storage->itemMousePressCurrent->GetScene() == this)
  {
    svtkAbstractContextItem* cur = this->Storage->itemMousePressCurrent;
    res = this->ProcessItem(cur, event, &svtkAbstractContextItem::MouseMoveEvent) || res;
  }
  else if (this->Storage->itemPicked)
  {
    svtkAbstractContextItem* cur = this->Storage->itemPicked;
    res = this->ProcessItem(cur, event, &svtkAbstractContextItem::MouseMoveEvent) || res;
  }

  // Update the last positions now
  event.SetLastScreenPos(event.GetScreenPos());
  event.SetLastScenePos(event.GetScenePos());
  event.SetLastPos(event.GetPos());
  return res;
}

//-----------------------------------------------------------------------------
bool svtkContextScene::ButtonPressEvent(const svtkContextMouseEvent& e)
{
  switch (e.GetButton())
  {
    case svtkContextMouseEvent::LEFT_BUTTON:
      this->InvokeEvent(svtkCommand::LeftButtonPressEvent);
      break;
    case svtkContextMouseEvent::MIDDLE_BUTTON:
      this->InvokeEvent(svtkCommand::MiddleButtonPressEvent);
      break;
    case svtkContextMouseEvent::RIGHT_BUTTON:
      this->InvokeEvent(svtkCommand::RightButtonPressEvent);
      break;
    default:
      break;
  }

  bool res = false;
  svtkContextMouseEvent& event = this->Storage->Event;
  this->EventCopy(e);
  event.SetLastScreenPos(event.GetScreenPos());
  event.SetLastScenePos(event.GetScenePos());
  event.SetLastPos(event.GetPos());
  event.SetButton(e.GetButton());

  svtkAbstractContextItem* newItemPicked = this->GetPickedItem();
  if (newItemPicked)
  {
    svtkAbstractContextItem* cur = newItemPicked;
    res = this->ProcessItem(cur, event, &svtkAbstractContextItem::MouseButtonPressEvent);
  }
  this->Storage->itemMousePressCurrent = newItemPicked;
  return res;
}

//-----------------------------------------------------------------------------
bool svtkContextScene::ButtonReleaseEvent(const svtkContextMouseEvent& e)
{
  switch (e.GetButton())
  {
    case svtkContextMouseEvent::LEFT_BUTTON:
      this->InvokeEvent(svtkCommand::LeftButtonReleaseEvent);
      break;
    case svtkContextMouseEvent::MIDDLE_BUTTON:
      this->InvokeEvent(svtkCommand::MiddleButtonReleaseEvent);
      break;
    case svtkContextMouseEvent::RIGHT_BUTTON:
      this->InvokeEvent(svtkCommand::RightButtonReleaseEvent);
      break;
    default:
      break;
  }

  bool res = false;
  if (this->Storage->itemMousePressCurrent)
  {
    svtkContextMouseEvent& event = this->Storage->Event;
    this->EventCopy(e);
    event.SetButton(e.GetButton());
    svtkAbstractContextItem* cur = this->Storage->itemMousePressCurrent;
    res = this->ProcessItem(cur, event, &svtkAbstractContextItem::MouseButtonReleaseEvent);
    this->Storage->itemMousePressCurrent = nullptr;
  }
  this->Storage->Event.SetButton(svtkContextMouseEvent::NO_BUTTON);
  return res;
}

//-----------------------------------------------------------------------------
bool svtkContextScene::DoubleClickEvent(const svtkContextMouseEvent& e)
{
  bool res = false;
  svtkContextMouseEvent& event = this->Storage->Event;
  this->EventCopy(e);
  event.SetLastScreenPos(event.GetScreenPos());
  event.SetLastScenePos(event.GetScenePos());
  event.SetLastPos(event.GetPos());
  event.SetButton(e.GetButton());

  svtkAbstractContextItem* newItemPicked = this->GetPickedItem();
  if (newItemPicked)
  {
    svtkAbstractContextItem* cur = newItemPicked;
    res = this->ProcessItem(cur, event, &svtkAbstractContextItem::MouseDoubleClickEvent);
  }
  return res;
}

//-----------------------------------------------------------------------------
bool svtkContextScene::MouseWheelEvent(int delta, const svtkContextMouseEvent& e)
{
  bool res = false;
  svtkContextMouseEvent& event = this->Storage->Event;
  this->EventCopy(e);
  event.SetLastScreenPos(event.GetScreenPos());
  event.SetLastScenePos(event.GetScenePos());
  event.SetLastPos(event.GetPos());
  event.SetButton(svtkContextMouseEvent::NO_BUTTON);

  svtkAbstractContextItem* newItemPicked = this->GetPickedItem();
  if (newItemPicked)
  {
    svtkContextMouseEvent itemEvent = event;
    svtkAbstractContextItem* cur = newItemPicked;
    itemEvent.SetPos(cur->MapFromScene(event.GetPos()));
    itemEvent.SetLastPos(cur->MapFromScene(event.GetLastPos()));
    while (cur && !cur->MouseWheelEvent(itemEvent, delta))
    {
      cur = cur->GetParent();
      if (cur)
      {
        itemEvent.SetPos(cur->MapToParent(itemEvent.GetPos()));
        itemEvent.SetLastPos(cur->MapToParent(itemEvent.GetLastPos()));
      }
    }
    res = (cur != nullptr);
  }
  return res;
}

//-----------------------------------------------------------------------------
bool svtkContextScene::KeyPressEvent(const svtkContextKeyEvent& keyEvent)
{
  svtkContextMouseEvent& event = this->Storage->Event;
  event.SetScreenPos(keyEvent.GetPosition());
  svtkAbstractContextItem* newItemPicked = this->GetPickedItem();
  if (newItemPicked)
  {
    return newItemPicked->KeyPressEvent(keyEvent);
  }
  return false;
}

//-----------------------------------------------------------------------------
bool svtkContextScene::KeyReleaseEvent(const svtkContextKeyEvent& keyEvent)
{
  svtkContextMouseEvent& event = this->Storage->Event;
  event.SetScreenPos(keyEvent.GetPosition());
  svtkAbstractContextItem* newItemPicked = this->GetPickedItem();
  if (newItemPicked)
  {
    return newItemPicked->KeyReleaseEvent(keyEvent);
  }
  return false;
}

//-----------------------------------------------------------------------------
inline bool svtkContextScene::ProcessItem(
  svtkAbstractContextItem* cur, const svtkContextMouseEvent& event, MouseEvents eventPtr)
{
  bool res = false;
  svtkContextMouseEvent itemEvent = event;
  itemEvent.SetPos(cur->MapFromScene(event.GetPos()));
  itemEvent.SetLastPos(cur->MapFromScene(event.GetLastPos()));
  while (cur && !(cur->*eventPtr)(itemEvent))
  {
    cur = cur->GetParent();
    if (cur)
    {
      itemEvent.SetPos(cur->MapToParent(itemEvent.GetPos()));
      itemEvent.SetLastPos(cur->MapToParent(itemEvent.GetLastPos()));
    }
  }
  res = (cur != nullptr);
  return res;
}

//-----------------------------------------------------------------------------
inline void svtkContextScene::EventCopy(const svtkContextMouseEvent& e)
{
  svtkContextMouseEvent& event = this->Storage->Event;
  event.SetPos(e.GetPos());
  event.SetScreenPos(svtkVector2i(e.GetPos().Cast<int>().GetData()));
  event.SetScenePos(e.GetPos());
  event.SetInteractor(e.GetInteractor());
}

//-----------------------------------------------------------------------------
void svtkContextScene::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  // Print out the chart's geometry if it has been set
  os << indent << "Widthxheight: " << this->Geometry[0] << "\t" << this->Geometry[1] << endl;
}
