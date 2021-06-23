/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLHardwareSelector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLHardwareSelector.h"

#include "svtk_glew.h"

#include "svtkDataObject.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLRenderUtilities.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLState.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"

#include "svtkOpenGLError.h"

//#define svtkOpenGLHardwareSelectorDEBUG
#ifdef svtkOpenGLHardwareSelectorDEBUG
#include "svtkImageImport.h"
#include "svtkNew.h"
#include "svtkPNMWriter.h"
#include "svtkWindows.h" // OK on UNix etc
#include <sstream>
#endif

#define ID_OFFSET 1

namespace
{
void annotate(const std::string& str)
{
  svtkOpenGLRenderUtilities::MarkDebugEvent(str);
}
}

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkOpenGLHardwareSelector);

//----------------------------------------------------------------------------
svtkOpenGLHardwareSelector::svtkOpenGLHardwareSelector()
{
#ifdef svtkOpenGLHardwareSelectorDEBUG
  cerr << "=====svtkOpenGLHardwareSelector::svtkOpenGLHardwareSelector" << endl;
#endif
}

//----------------------------------------------------------------------------
svtkOpenGLHardwareSelector::~svtkOpenGLHardwareSelector()
{
#ifdef svtkOpenGLHardwareSelectorDEBUG
  cerr << "=====svtkOpenGLHardwareSelector::~svtkOpenGLHardwareSelector" << endl;
#endif
}

//----------------------------------------------------------------------------
void svtkOpenGLHardwareSelector::PreCapturePass(int pass)
{
  annotate(std::string("Starting pass: ") + this->PassTypeToString(static_cast<PassTypes>(pass)));

  // Disable blending
  svtkOpenGLRenderWindow* rwin =
    static_cast<svtkOpenGLRenderWindow*>(this->Renderer->GetRenderWindow());
  svtkOpenGLState* ostate = rwin->GetState();

  this->OriginalBlending = ostate->GetEnumState(GL_BLEND);
  ostate->svtkglDisable(GL_BLEND);
}

//----------------------------------------------------------------------------
void svtkOpenGLHardwareSelector::PostCapturePass(int pass)
{
  // Restore blending.
  svtkOpenGLRenderWindow* rwin =
    static_cast<svtkOpenGLRenderWindow*>(this->Renderer->GetRenderWindow());
  svtkOpenGLState* ostate = rwin->GetState();

  ostate->SetEnumState(GL_BLEND, this->OriginalBlending);
  annotate(std::string("Pass complete: ") + this->PassTypeToString(static_cast<PassTypes>(pass)));
}

//----------------------------------------------------------------------------
void svtkOpenGLHardwareSelector::BeginSelection()
{
  svtkOpenGLRenderWindow* rwin =
    static_cast<svtkOpenGLRenderWindow*>(this->Renderer->GetRenderWindow());

  this->OriginalMultiSample = rwin->GetMultiSamples();
  rwin->SetMultiSamples(0);

  svtkOpenGLState* ostate = rwin->GetState();
  ostate->ResetFramebufferBindings();

  // render normally to set the zbuffer
  if (this->FieldAssociation == svtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    svtkOpenGLState::ScopedglEnableDisable bsaver(ostate, GL_BLEND);
    ostate->svtkglDisable(GL_BLEND);

    rwin->Render();
    this->Renderer->PreserveDepthBufferOn();
  }

  return this->Superclass::BeginSelection();
}

//----------------------------------------------------------------------------
void svtkOpenGLHardwareSelector::EndSelection()
{
  // render normally to set the zbuffer
  if (this->FieldAssociation == svtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    this->Renderer->PreserveDepthBufferOff();
  }

  svtkOpenGLRenderWindow* rwin =
    static_cast<svtkOpenGLRenderWindow*>(this->Renderer->GetRenderWindow());
  rwin->SetMultiSamples(this->OriginalMultiSample);

  return this->Superclass::EndSelection();
}

//----------------------------------------------------------------------------
// just add debug output if compiled with svtkOpenGLHardwareSelectorDEBUG
void svtkOpenGLHardwareSelector::SavePixelBuffer(int passNo)
{
  this->Superclass::SavePixelBuffer(passNo);

#ifdef svtkOpenGLHardwareSelectorDEBUG

  svtkNew<svtkImageImport> ii;
  ii->SetImportVoidPointer(this->PixBuffer[passNo], 1);
  ii->SetDataScalarTypeToUnsignedChar();
  ii->SetNumberOfScalarComponents(3);
  ii->SetDataExtent(this->Area[0], this->Area[2], this->Area[1], this->Area[3], 0, 0);
  ii->SetWholeExtent(this->Area[0], this->Area[2], this->Area[1], this->Area[3], 0, 0);

  // change this to somewhere on your system
  // hardcoded as with MPI/parallel/client server it can be hard to
  // find these images sometimes.
  std::string fname = "C:/Users/ken.martin/Documents/pickbuffer_";

#if defined(_WIN32)
  std::ostringstream toString;
  toString.str("");
  toString.clear();
  toString << GetCurrentProcessId();
  fname += toString.str();
  fname += "_";
#endif
  fname += ('0' + passNo);
  fname += ".pnm";
  svtkNew<svtkPNMWriter> pw;
  pw->SetInputConnection(ii->GetOutputPort());
  pw->SetFileName(fname.c_str());
  pw->Write();
  cerr << "=====svtkOpenGLHardwareSelector wrote " << fname << "\n";
#endif
}

//----------------------------------------------------------------------------
void svtkOpenGLHardwareSelector::BeginRenderProp(svtkRenderWindow*)
{
#ifdef svtkOpenGLHardwareSelectorDEBUG
  cerr << "=====svtkOpenGLHardwareSelector::BeginRenderProp" << endl;
#endif
}

//----------------------------------------------------------------------------
void svtkOpenGLHardwareSelector::BeginRenderProp()
{
  this->InPropRender++;
  if (this->InPropRender != 1)
  {
    return;
  }

  // device specific prep
  svtkRenderWindow* renWin = this->Renderer->GetRenderWindow();
  this->BeginRenderProp(renWin);

  // cout << "In BeginRenderProp" << endl;
  if (this->CurrentPass == ACTOR_PASS)
  {
    int propid = this->PropID;
    if (propid >= 0xfffffe)
    {
      svtkErrorMacro("Too many props. Currently only " << 0xfffffe << " props are supported.");
      return;
    }
    float color[3];
    // Since 0 is reserved for nothing selected, we offset propid by 1.
    propid = propid + 1;
    svtkHardwareSelector::Convert(propid, color);
    this->SetPropColorValue(color);
  }
  else if (this->CurrentPass == PROCESS_PASS)
  {
    float color[3];
    // Since 0 is reserved for nothing selected, we offset propid by 1.
    svtkHardwareSelector::Convert(this->ProcessID + 1, color);
    this->SetPropColorValue(color);
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLHardwareSelector::EndRenderProp(svtkRenderWindow*)
{
#ifdef svtkOpenGLHardwareSelectorDEBUG
  cerr << "=====svtkOpenGLHardwareSelector::EndRenderProp" << endl;
#endif
}

//----------------------------------------------------------------------------
void svtkOpenGLHardwareSelector::EndRenderProp()
{
  this->Superclass::EndRenderProp();
}

//----------------------------------------------------------------------------
void svtkOpenGLHardwareSelector::RenderCompositeIndex(unsigned int index)
{

  if (index > 0xffffff)
  {
    svtkErrorMacro("Indices > 0xffffff are not supported.");
    return;
  }

  index += ID_OFFSET;

  if (this->CurrentPass == COMPOSITE_INDEX_PASS)
  {
    float color[3];
    svtkHardwareSelector::Convert(static_cast<svtkIdType>(0xffffff & index), color);
    this->SetPropColorValue(color);
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLHardwareSelector::RenderProcessId(unsigned int processid)
{
  if (this->CurrentPass == PROCESS_PASS && this->UseProcessIdFromData)
  {
    if (processid >= 0xffffff)
    {
      svtkErrorMacro("Invalid id: " << processid);
      return;
    }

    float color[3];
    svtkHardwareSelector::Convert(static_cast<svtkIdType>(processid + 1), color);
    this->SetPropColorValue(color);
  }
}

//----------------------------------------------------------------------------
void svtkOpenGLHardwareSelector::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
