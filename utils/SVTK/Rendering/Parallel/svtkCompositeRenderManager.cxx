/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeRenderManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCompositeRenderManager.h"

#include "svtkCompressCompositer.h"
#include "svtkFloatArray.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkTimerLog.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkCompositeRenderManager);

svtkCxxSetObjectMacro(svtkCompositeRenderManager, Compositer, svtkCompositer);

//----------------------------------------------------------------------------
svtkCompositeRenderManager::svtkCompositeRenderManager()
{
  this->Compositer = svtkCompressCompositer::New();
  this->Compositer->Register(this);
  this->Compositer->Delete();

  this->DepthData = svtkFloatArray::New();
  this->TmpPixelData = svtkUnsignedCharArray::New();
  this->TmpDepthData = svtkFloatArray::New();

  this->DepthData->SetNumberOfComponents(1);
  this->TmpPixelData->SetNumberOfComponents(4);
  this->TmpDepthData->SetNumberOfComponents(1);
}

//----------------------------------------------------------------------------
svtkCompositeRenderManager::~svtkCompositeRenderManager()
{
  this->SetCompositer(nullptr);
  this->DepthData->Delete();
  this->TmpPixelData->Delete();
  this->TmpDepthData->Delete();
}

//----------------------------------------------------------------------------
void svtkCompositeRenderManager::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Compositer: " << endl;
  this->Compositer->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
void svtkCompositeRenderManager::PreRenderProcessing()
{
  svtkTimerLog::MarkStartEvent("Compositing");
  // Turn swap buffers off before the render so the end render method has a
  // chance to add to the back buffer.
  if (this->UseBackBuffer)
  {
    this->RenderWindow->SwapBuffersOff();
  }

  this->SavedMultiSamplesSetting = this->RenderWindow->GetMultiSamples();
  this->RenderWindow->SetMultiSamples(0);
}

//----------------------------------------------------------------------------
void svtkCompositeRenderManager::PostRenderProcessing()
{
  this->RenderWindow->SetMultiSamples(this->SavedMultiSamplesSetting);

  if (!this->UseCompositing || this->CheckForAbortComposite())
  {
    svtkTimerLog::MarkEndEvent("Compositing");
    return;
  }

  if (this->Controller->GetNumberOfProcesses() > 1)
  {
    // Read in data.
    this->ReadReducedImage();
    this->Timer->StartTimer();
    this->RenderWindow->GetZbufferData(
      0, 0, this->ReducedImageSize[0] - 1, this->ReducedImageSize[1] - 1, this->DepthData);

    // Set up temporary buffers.
    this->TmpPixelData->SetNumberOfComponents(this->ReducedImage->GetNumberOfComponents());
    this->TmpPixelData->SetNumberOfTuples(this->ReducedImage->GetNumberOfTuples());
    this->TmpDepthData->SetNumberOfComponents(this->DepthData->GetNumberOfComponents());
    this->TmpDepthData->SetNumberOfTuples(this->DepthData->GetNumberOfTuples());

    // Do composite
    this->Compositer->SetController(this->Controller);
    this->Compositer->CompositeBuffer(
      this->ReducedImage, this->DepthData, this->TmpPixelData, this->TmpDepthData);

    this->Timer->StopTimer();
    this->ImageProcessingTime = this->Timer->GetElapsedTime();
  }

  this->WriteFullImage();

  // Swap buffers here
  if (this->UseBackBuffer)
  {
    this->RenderWindow->SwapBuffersOn();
  }
  this->RenderWindow->Frame();

  svtkTimerLog::MarkEndEvent("Compositing");
}
