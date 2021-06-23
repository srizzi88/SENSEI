/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStringToImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkStringToImage.h"

#include "svtkObjectFactory.h"

//-----------------------------------------------------------------------------
svtkStringToImage::svtkStringToImage()
{
  this->Antialias = true;
  this->ScaleToPowerOfTwo = false;
}

//-----------------------------------------------------------------------------
svtkStringToImage::~svtkStringToImage() = default;

//-----------------------------------------------------------------------------
void svtkStringToImage::SetScaleToPowerOfTwo(bool scale)
{
  if (this->ScaleToPowerOfTwo != scale)
  {
    this->ScaleToPowerOfTwo = scale;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void svtkStringToImage::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ScaleToPowerOfTwo: " << this->ScaleToPowerOfTwo << endl;
}
