/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBalloonWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBalloonWidget.h"
#include "svtkAssemblyPath.h"
#include "svtkBalloonRepresentation.h"
#include "svtkCommand.h"
#include "svtkImageData.h"
#include "svtkObjectFactory.h"
#include "svtkPickingManager.h"
#include "svtkProp.h"
#include "svtkPropPicker.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkRendererCollection.h"
#include "svtkStdString.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"

#include <cassert>
#include <map>

svtkStandardNewMacro(svtkBalloonWidget);

//-- Define the PIMPLd array of svtkProp and svtkString --
struct svtkBalloon
{
  svtkStdString Text;
  svtkImageData* Image;

  svtkBalloon()
    : Text()
    , Image(nullptr)
  {
  }
  svtkBalloon(const svtkBalloon& balloon)
    : Text(balloon.Text)
    , Image(balloon.Image)
  {
    if (this->Image)
    {
      this->Image->Register(nullptr);
    }
  }
  svtkBalloon(svtkStdString* str, svtkImageData* img)
  {
    this->Text = *str;
    this->Image = img;
    if (this->Image)
    {
      this->Image->Register(nullptr);
    }
  }
  svtkBalloon(const char* str, svtkImageData* img)
  {
    this->Text = svtkStdString(str);
    this->Image = img;
    if (this->Image)
    {
      this->Image->Register(nullptr);
    }
  }
  ~svtkBalloon()
  {
    if (this->Image)
    {
      this->Image->UnRegister(nullptr);
    }
  }
  svtkBalloon& operator=(const svtkBalloon& balloon)
  {
    this->Text = balloon.Text;

    // Don't leak if we already have an image.
    if (this->Image)
    {
      this->Image->UnRegister(nullptr);
      this->Image = nullptr;
    }

    this->Image = balloon.Image;
    if (this->Image)
    {
      this->Image->Register(nullptr);
    }

    return *this;
  }
  bool operator==(const svtkBalloon& balloon) const
  {
    if (this->Image == balloon.Image)
    {
      if (this->Text == balloon.Text)
      {
        return true;
      }
    }
    return false;
  }
  bool operator!=(const svtkBalloon& balloon) const { return !(*this == balloon); }
};

class svtkPropMap : public std::map<svtkProp*, svtkBalloon>
{
};
typedef std::map<svtkProp*, svtkBalloon>::iterator svtkPropMapIterator;

//-------------------------------------------------------------------------
svtkBalloonWidget::svtkBalloonWidget()
{
  this->Picker = svtkPropPicker::New();
  this->Picker->PickFromListOn();

  this->CurrentProp = nullptr;
  this->PropMap = new svtkPropMap;
}

//-------------------------------------------------------------------------
svtkBalloonWidget::~svtkBalloonWidget()
{
  this->Picker->Delete();

  if (this->CurrentProp)
  {
    this->CurrentProp->Delete();
    this->CurrentProp = nullptr;
  }

  this->PropMap->clear();
  delete this->PropMap;
}

//----------------------------------------------------------------------
void svtkBalloonWidget::SetEnabled(int enabling)
{
  this->Superclass::SetEnabled(enabling);

  if (this->Interactor && this->Interactor->GetRenderWindow())
  {
    this->SetCurrentRenderer(
      this->Interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer());
  }
  if (!this->CurrentRenderer)
  {
    return;
  }

  if (enabling)
  {
    this->CreateDefaultRepresentation();
    this->WidgetRep->SetRenderer(this->CurrentRenderer);
    this->WidgetRep->BuildRepresentation();
    this->CurrentRenderer->AddViewProp(this->WidgetRep);
  }
  else
  {
    this->CurrentRenderer->RemoveViewProp(this->WidgetRep);
    this->SetCurrentRenderer(nullptr);
  }
}

//----------------------------------------------------------------------
void svtkBalloonWidget::SetPicker(svtkAbstractPropPicker* picker)
{
  if (picker == nullptr || picker == this->Picker)
  {
    return;
  }

  // Configure picker appropriately
  picker->PickFromListOn();

  this->Picker->Delete();
  this->Picker = picker;
  this->Picker->Register(this);

  this->UnRegisterPickers();
  this->RegisterPickers();
  this->Modified();
}

//----------------------------------------------------------------------
void svtkBalloonWidget::RegisterPickers()
{
  svtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->Picker, this);
}

//----------------------------------------------------------------------
void svtkBalloonWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkBalloonRepresentation::New();
  }
}

//-------------------------------------------------------------------------
void svtkBalloonWidget::AddBalloon(svtkProp* prop, svtkStdString* str, svtkImageData* img)
{
  assert(prop);
  svtkPropMapIterator iter = this->PropMap->find(prop);
  if (iter == this->PropMap->end() || (*this->PropMap)[prop] != svtkBalloon(str, img))
  {
    (*this->PropMap)[prop] = svtkBalloon(str, img);
    this->Picker->DeletePickList(prop); // ensure only entered once
    this->Picker->AddPickList(prop);
    this->Modified();
  }
}

//-------------------------------------------------------------------------
void svtkBalloonWidget::AddBalloon(svtkProp* prop, const char* str, svtkImageData* img)
{
  svtkStdString s;
  if (str)
  {
    s = svtkStdString(str);
  }
  this->AddBalloon(prop, &s, img);
}

//-------------------------------------------------------------------------
void svtkBalloonWidget::RemoveBalloon(svtkProp* prop)
{
  svtkPropMapIterator iter = this->PropMap->find(prop);
  if (iter != this->PropMap->end())
  {
    this->PropMap->erase(iter);
    if (prop != nullptr)
    {
      this->Picker->DeletePickList(prop);
    }
    this->Modified();
  }
}

//-------------------------------------------------------------------------
const char* svtkBalloonWidget::GetBalloonString(svtkProp* prop)
{
  svtkPropMapIterator iter = this->PropMap->find(prop);
  if (iter != this->PropMap->end())
  {
    return (*iter).second.Text.c_str();
  }
  return nullptr;
}

//-------------------------------------------------------------------------
svtkImageData* svtkBalloonWidget::GetBalloonImage(svtkProp* prop)
{
  svtkPropMapIterator iter = this->PropMap->find(prop);
  if (iter != this->PropMap->end())
  {
    return (*iter).second.Image;
  }
  return nullptr;
}

//-------------------------------------------------------------------------
void svtkBalloonWidget::UpdateBalloonString(svtkProp* prop, const char* str)
{
  svtkPropMapIterator iter = this->PropMap->find(prop);
  if (iter != this->PropMap->end())
  {
    (*iter).second.Text = str;
    this->WidgetRep->Modified();
  }
}

//-------------------------------------------------------------------------
void svtkBalloonWidget::UpdateBalloonImage(svtkProp* prop, svtkImageData* image)
{
  svtkPropMapIterator iter = this->PropMap->find(prop);
  if (iter != this->PropMap->end())
  {
    (*iter).second.Image = image;
    this->WidgetRep->Modified();
  }
}

//-------------------------------------------------------------------------
int svtkBalloonWidget::SubclassHoverAction()
{
  double e[2];
  e[0] = static_cast<double>(this->Interactor->GetEventPosition()[0]);
  e[1] = static_cast<double>(this->Interactor->GetEventPosition()[1]);
  if (this->CurrentProp)
  {
    this->CurrentProp->UnRegister(this);
    this->CurrentProp = nullptr;
  }

  svtkAssemblyPath* path = this->GetAssemblyPath(e[0], e[1], 0., this->Picker);

  if (path != nullptr)
  {
    svtkPropMapIterator iter = this->PropMap->find(path->GetFirstNode()->GetViewProp());
    if (iter != this->PropMap->end())
    {
      this->CurrentProp = (*iter).first;
      this->CurrentProp->Register(this);
      reinterpret_cast<svtkBalloonRepresentation*>(this->WidgetRep)
        ->SetBalloonText((*iter).second.Text);
      reinterpret_cast<svtkBalloonRepresentation*>(this->WidgetRep)
        ->SetBalloonImage((*iter).second.Image);
      this->WidgetRep->StartWidgetInteraction(e);
      this->Render();
    }
  }

  return 1;
}

//-------------------------------------------------------------------------
int svtkBalloonWidget::SubclassEndHoverAction()
{
  double e[2];
  e[0] = static_cast<double>(this->Interactor->GetEventPosition()[0]);
  e[1] = static_cast<double>(this->Interactor->GetEventPosition()[1]);
  this->WidgetRep->EndWidgetInteraction(e);
  this->Render();

  return 1;
}

//-------------------------------------------------------------------------
void svtkBalloonWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Current Prop: ";
  if (this->CurrentProp)
  {
    os << this->CurrentProp << "\n";
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Picker: " << this->Picker << "\n";
}
