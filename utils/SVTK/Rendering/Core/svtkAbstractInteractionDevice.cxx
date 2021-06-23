/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAbstractInteractionDevice.h"

#include "svtkObjectFactory.h"

svtkAbstractObjectFactoryNewMacro(svtkAbstractInteractionDevice);

svtkAbstractInteractionDevice::svtkAbstractInteractionDevice()
  : Initialized(false)
  , RenderWidget(nullptr)
  , RenderDevice(nullptr)
{
}

svtkAbstractInteractionDevice::~svtkAbstractInteractionDevice() = default;

void svtkAbstractInteractionDevice::SetRenderWidget(svtkRenderWidget* widget)
{
  if (this->RenderWidget != widget)
  {
    this->RenderWidget = widget;
    this->Modified();
  }
}

void svtkAbstractInteractionDevice::SetRenderDevice(svtkAbstractRenderDevice* d)
{
  if (this->RenderDevice != d)
  {
    this->RenderDevice = d;
    this->Modified();
  }
}

void svtkAbstractInteractionDevice::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
