/*=========================================================================

  Program:   ParaView
  Module:    svtkClientServerSynchronizedRenderers.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkClientServerSynchronizedRenderers.h"

#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"

#include <cassert>

svtkStandardNewMacro(svtkClientServerSynchronizedRenderers);
//----------------------------------------------------------------------------
svtkClientServerSynchronizedRenderers::svtkClientServerSynchronizedRenderers() {}

//----------------------------------------------------------------------------
svtkClientServerSynchronizedRenderers::~svtkClientServerSynchronizedRenderers() {}

//----------------------------------------------------------------------------
void svtkClientServerSynchronizedRenderers::MasterEndRender()
{
  // receive image from slave.
  assert(this->ParallelController->IsA("svtkSocketController"));

  svtkRawImage& rawImage = this->Image;

  int header[4];
  this->ParallelController->Receive(header, 4, 1, 0x023430);
  if (header[0] > 0)
  {
    rawImage.Resize(header[1], header[2], header[3]);
    this->ParallelController->Receive(rawImage.GetRawPtr(), 1, 0x023430);
    rawImage.MarkValid();
  }
}

//----------------------------------------------------------------------------
void svtkClientServerSynchronizedRenderers::SlaveEndRender()
{
  assert(this->ParallelController->IsA("svtkSocketController"));

  svtkRawImage& rawImage = this->CaptureRenderedImage();

  int header[4];
  header[0] = rawImage.IsValid() ? 1 : 0;
  header[1] = rawImage.GetWidth();
  header[2] = rawImage.GetHeight();
  header[3] = rawImage.IsValid() ? rawImage.GetRawPtr()->GetNumberOfComponents() : 0;

  // send the image to the client.
  this->ParallelController->Send(header, 4, 1, 0x023430);
  if (rawImage.IsValid())
  {
    this->ParallelController->Send(rawImage.GetRawPtr(), 1, 0x023430);
  }
}

//----------------------------------------------------------------------------
void svtkClientServerSynchronizedRenderers::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
