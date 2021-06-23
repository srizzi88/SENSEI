/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkImageItem.h"

#include "svtkContext2D.h"
#include "svtkImageData.h"
#include "svtkObjectFactory.h"

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkImageItem);

//-----------------------------------------------------------------------------
svtkCxxSetObjectMacro(svtkImageItem, Image, svtkImageData);

//-----------------------------------------------------------------------------
svtkImageItem::svtkImageItem()
{
  this->Position[0] = this->Position[1] = 0;
  this->Image = nullptr;
}

//-----------------------------------------------------------------------------
svtkImageItem::~svtkImageItem()
{
  this->SetImage(nullptr);
}

//-----------------------------------------------------------------------------
bool svtkImageItem::Paint(svtkContext2D* painter)
{
  if (this->Image)
  {
    // Draw our image in the bottom left corner of the item
    painter->DrawImage(this->Position[0], this->Position[1], this->Image);
  }
  return true;
}

//-----------------------------------------------------------------------------
void svtkImageItem::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
