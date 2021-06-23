/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRenderWidget.h"

#include "svtkAbstractInteractionDevice.h"
#include "svtkAbstractRenderDevice.h"
#include "svtkObjectFactory.h"
#include "svtkRect.h"

svtkStandardNewMacro(svtkRenderWidget);

svtkRenderWidget::svtkRenderWidget()
  : Position(0, 0)
  , Size(300, 300)
  , Name("New SVTK RenderWidget!!!")
{
}

svtkRenderWidget::~svtkRenderWidget() = default;

void svtkRenderWidget::SetPosition(const svtkVector2i& pos)
{
  if (this->Position != pos)
  {
    this->Position = pos;
    this->Modified();
  }
}

void svtkRenderWidget::SetSize(const svtkVector2i& size)
{
  if (this->Size != size)
  {
    this->Size = size;
    this->Modified();
  }
}

void svtkRenderWidget::SetName(const std::string& name)
{
  if (this->Name != name)
  {
    this->Name = name;
    this->Modified();
  }
}

void svtkRenderWidget::Render()
{
  assert(this->RenderDevice != nullptr);
  cout << "Render called!!!" << endl;
}

void svtkRenderWidget::MakeCurrent()
{
  assert(this->RenderDevice != nullptr);
  this->RenderDevice->MakeCurrent();
}

void svtkRenderWidget::Initialize()
{
  assert(this->RenderDevice != nullptr && this->InteractionDevice != nullptr);
  this->InteractionDevice->SetRenderWidget(this);
  this->InteractionDevice->SetRenderDevice(this->RenderDevice);
  this->RenderDevice->CreateNewWindow(
    svtkRecti(this->Position.GetX(), this->Position.GetY(), this->Size.GetX(), this->Size.GetY()),
    this->Name);
  this->InteractionDevice->Initialize();
}

void svtkRenderWidget::Start()
{
  assert(this->InteractionDevice != nullptr);
  this->Initialize();
  this->InteractionDevice->Start();
}

void svtkRenderWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  // FIXME: Add methods for this...
  this->Superclass::PrintSelf(os, indent);
}
