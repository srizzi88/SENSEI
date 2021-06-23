/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositedSynchronizedRenderers.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCompositedSynchronizedRenderers.h"

#include "svtkFloatArray.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLRenderer.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkTreeCompositer.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkCompositedSynchronizedRenderers);
svtkCxxSetObjectMacro(svtkCompositedSynchronizedRenderers, Compositer, svtkCompositer);
//----------------------------------------------------------------------------
svtkCompositedSynchronizedRenderers::svtkCompositedSynchronizedRenderers()
{
  this->Compositer = svtkTreeCompositer::New();
}

//----------------------------------------------------------------------------
svtkCompositedSynchronizedRenderers::~svtkCompositedSynchronizedRenderers()
{
  this->Compositer->Delete();
}

//----------------------------------------------------------------------------
void svtkCompositedSynchronizedRenderers::MasterEndRender()
{
  svtkRawImage& rawImage = this->CaptureRenderedImage();
  svtkFloatArray* depth_buffer = svtkFloatArray::New();
  this->CaptureRenderedDepthBuffer(depth_buffer);

  this->Compositer->SetController(this->ParallelController);
  svtkUnsignedCharArray* resultColor = svtkUnsignedCharArray::New();
  resultColor->SetNumberOfComponents(rawImage.GetRawPtr()->GetNumberOfComponents());
  resultColor->SetNumberOfTuples(rawImage.GetRawPtr()->GetNumberOfTuples());

  svtkFloatArray* result_depth = svtkFloatArray::New();
  result_depth->SetNumberOfTuples(depth_buffer->GetNumberOfTuples());

  this->Compositer->CompositeBuffer(rawImage.GetRawPtr(), depth_buffer, resultColor, result_depth);
  depth_buffer->Delete();
  result_depth->Delete();
  resultColor->Delete();
}

//----------------------------------------------------------------------------
void svtkCompositedSynchronizedRenderers::SlaveEndRender()
{
  svtkRawImage& rawImage = this->CaptureRenderedImage();
  svtkFloatArray* depth_buffer = svtkFloatArray::New();
  this->CaptureRenderedDepthBuffer(depth_buffer);
  this->Compositer->SetController(this->ParallelController);

  svtkUnsignedCharArray* resultColor = svtkUnsignedCharArray::New();
  resultColor->SetNumberOfComponents(rawImage.GetRawPtr()->GetNumberOfComponents());
  resultColor->SetNumberOfTuples(rawImage.GetRawPtr()->GetNumberOfTuples());
  svtkFloatArray* result_depth = svtkFloatArray::New();
  result_depth->SetNumberOfTuples(depth_buffer->GetNumberOfTuples());

  this->Compositer->CompositeBuffer(rawImage.GetRawPtr(), depth_buffer, resultColor, result_depth);
  depth_buffer->Delete();
  resultColor->Delete();
  result_depth->Delete();
}

//----------------------------------------------------------------------------
void svtkCompositedSynchronizedRenderers::CaptureRenderedDepthBuffer(svtkFloatArray* depth_buffer)
{
  double viewport[4];
  svtkOpenGLRenderer* ren = this->Renderer;
  ren->GetViewport(viewport);

  int window_size[2];
  window_size[0] = ren->GetSVTKWindow()->GetActualSize()[0];
  window_size[1] = ren->GetSVTKWindow()->GetActualSize()[1];

  int image_size[2];
  image_size[0] = static_cast<int>(window_size[0] * (viewport[2] - viewport[0]));
  image_size[1] = static_cast<int>(window_size[1] * (viewport[3] - viewport[1]));

  // using RGBA always?
  depth_buffer->SetNumberOfComponents(1);
  depth_buffer->SetNumberOfTuples(image_size[0] * image_size[1]);

  ren->GetRenderWindow()->GetZbufferData(static_cast<int>(window_size[0] * viewport[0]),
    static_cast<int>(window_size[1] * viewport[1]),
    static_cast<int>(window_size[0] * viewport[2]) - 1,
    static_cast<int>(window_size[1] * viewport[3]) - 1, depth_buffer->GetPointer(0));
}

//----------------------------------------------------------------------------
void svtkCompositedSynchronizedRenderers::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Compositer: ";
  if (this->Compositer)
  {
    this->Compositer->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}
