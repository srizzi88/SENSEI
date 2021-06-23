/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageProcessingPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkImageProcessingPass.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLState.h"
#include "svtkRenderState.h"
#include "svtkRenderer.h"
#include "svtkTextureObject.h"
#include "svtk_glew.h"
#include <cassert>

// to be able to dump intermediate passes into png files for debugging.
// only for svtkImageProcessingPass developers.
//#define SVTK_IMAGE_PROCESSING_PASS_DEBUG

#include "svtkCamera.h"
#include "svtkMath.h"
#include "svtkPixelBufferObject.h"

svtkCxxSetObjectMacro(svtkImageProcessingPass, DelegatePass, svtkRenderPass);

// ----------------------------------------------------------------------------
svtkImageProcessingPass::svtkImageProcessingPass()
{
  this->DelegatePass = nullptr;
}

// ----------------------------------------------------------------------------
svtkImageProcessingPass::~svtkImageProcessingPass()
{
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->Delete();
  }
}

// ----------------------------------------------------------------------------
void svtkImageProcessingPass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DelegatePass:";
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}
// ----------------------------------------------------------------------------
// Description:
// Render delegate with a image of different dimensions than the
// original one.
// \pre s_exists: s!=0
// \pre fbo_exists: fbo!=0
// \pre fbo_has_context: fbo->GetContext()!=0
// \pre target_exists: target!=0
// \pre target_has_context: target->GetContext()!=0
void svtkImageProcessingPass::RenderDelegate(const svtkRenderState* s, int width, int height,
  int newWidth, int newHeight, svtkOpenGLFramebufferObject* fbo, svtkTextureObject* target)
{
  assert("pre: s_exists" && s != nullptr);
  assert("pre: fbo_exists" && fbo != nullptr);
  assert("pre: fbo_has_context" && fbo->GetContext() != nullptr);
  assert("pre: target_exists" && target != nullptr);
  assert("pre: target_has_context" && target->GetContext() != nullptr);

#ifdef SVTK_IMAGE_PROCESSING_PASS_DEBUG
  cout << "width=" << width << endl;
  cout << "height=" << height << endl;
  cout << "newWidth=" << newWidth << endl;
  cout << "newHeight=" << newHeight << endl;
#endif

  svtkRenderer* r = s->GetRenderer();
  svtkRenderState s2(r);
  s2.SetPropArrayAndCount(s->GetPropArray(), s->GetPropArrayCount());

  // Adapt camera to new window size
  svtkCamera* savedCamera = r->GetActiveCamera();
  savedCamera->Register(this);
  svtkCamera* newCamera = svtkCamera::New();
  newCamera->DeepCopy(savedCamera);

  svtkOpenGLState* ostate = static_cast<svtkOpenGLRenderWindow*>(r->GetSVTKWindow())->GetState();

#ifdef SVTK_IMAGE_PROCESSING_PASS_DEBUG
  cout << "old camera params=";
  savedCamera->Print(cout);
  cout << "new camera params=";
  newCamera->Print(cout);
#endif
  r->SetActiveCamera(newCamera);

  if (newCamera->GetParallelProjection())
  {
    newCamera->SetParallelScale(
      newCamera->GetParallelScale() * newHeight / static_cast<double>(height));
  }
  else
  {
    double large;
    double small;
    if (newCamera->GetUseHorizontalViewAngle())
    {
      large = newWidth;
      small = width;
    }
    else
    {
      large = newHeight;
      small = height;
    }
    double angle = svtkMath::RadiansFromDegrees(newCamera->GetViewAngle());

#ifdef SVTK_IMAGE_PROCESSING_PASS_DEBUG
    cout << "old angle =" << angle << " rad=" << svtkMath::DegreesFromRadians(angle) << " deg"
         << endl;
#endif

    angle = 2.0 * atan(tan(angle / 2.0) * large / static_cast<double>(small));

#ifdef SVTK_IMAGE_PROCESSING_PASS_DEBUG
    cout << "new angle =" << angle << " rad=" << svtkMath::DegreesFromRadians(angle) << " deg"
         << endl;
#endif

    newCamera->SetViewAngle(svtkMath::DegreesFromRadians(angle));
  }

  s2.SetFrameBuffer(fbo);

  if (target->GetWidth() != static_cast<unsigned int>(newWidth) ||
    target->GetHeight() != static_cast<unsigned int>(newHeight))
  {
    target->Create2D(newWidth, newHeight, 4, SVTK_UNSIGNED_CHAR, false);
  }

  fbo->Bind();
  fbo->AddColorAttachment(0, target);

  // because the same FBO can be used in another pass but with several color
  // buffers, force this pass to use 1, to avoid side effects from the
  // render of the previous frame.
  fbo->ActivateBuffer(0);

  fbo->AddDepthAttachment();
  fbo->StartNonOrtho(newWidth, newHeight);
  ostate->svtkglViewport(0, 0, newWidth, newHeight);
  ostate->svtkglScissor(0, 0, newWidth, newHeight);

  // 2. Delegate render in FBO
  ostate->svtkglEnable(GL_DEPTH_TEST);
  this->DelegatePass->Render(&s2);
  this->NumberOfRenderedProps += this->DelegatePass->GetNumberOfRenderedProps();

#ifdef SVTK_IMAGE_PROCESSING_PASS_DEBUG
  svtkPixelBufferObject* pbo = target->Download();

  unsigned int dims[2];
  svtkIdType continuousInc[3];

  dims[0] = static_cast<unsigned int>(newWidth);
  dims[1] = static_cast<unsigned int>(newHeight);
  continuousInc[0] = 0;
  continuousInc[1] = 0;
  continuousInc[2] = 0;

  int byteSize = newWidth * newHeight * 4 * sizeof(float);
  float* buffer = new float[newWidth * newHeight * 4];
  pbo->Download2D(SVTK_FLOAT, buffer, dims, 4, continuousInc);

  svtkImageImport* importer = svtkImageImport::New();
  importer->CopyImportVoidPointer(buffer, static_cast<int>(byteSize));
  importer->SetDataScalarTypeToFloat();
  importer->SetNumberOfScalarComponents(4);
  importer->SetWholeExtent(0, newWidth - 1, 0, newHeight - 1, 0, 0);
  importer->SetDataExtentToWholeExtent();

  importer->Update();

  svtkPNGWriter* writer = svtkPNGWriter::New();
  writer->SetFileName("ip.png");
  writer->SetInputConnection(importer->GetOutputPort());
  importer->Delete();
  cout << "Writing " << writer->GetFileName() << endl;
  writer->Write();
  cout << "Wrote " << writer->GetFileName() << endl;
  writer->Delete();

  pbo->Delete();
  delete[] buffer;
#endif

  newCamera->Delete();
  r->SetActiveCamera(savedCamera);
  savedCamera->UnRegister(this);
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void svtkImageProcessingPass::ReleaseGraphicsResources(svtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->ReleaseGraphicsResources(w);
  }
}
