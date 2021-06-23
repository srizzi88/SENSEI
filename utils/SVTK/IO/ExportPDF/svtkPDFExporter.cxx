/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPDFExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPDFExporter.h"

#include "svtkContext2D.h"
#include "svtkContextActor.h"
#include "svtkContextScene.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPDFContextDevice2D.h"
#include "svtkProp.h"
#include "svtkPropCollection.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkRendererCollection.h"

#include <svtk_libharu.h>

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace
{

void HPDF_STDCALL handle_libharu_error(HPDF_STATUS error, HPDF_STATUS detail, void*)
{
  std::ostringstream out;
  out << "LibHaru failed during PDF export. Error=0x" << std::hex << error << " detail=" << std::dec
      << detail;
  throw std::runtime_error(out.str());
}

} // end anon namespace

//------------------------------------------------------------------------------
// svtkPDFExporter::Details
//------------------------------------------------------------------------------

struct svtkPDFExporter::Details
{
  HPDF_Doc Document;
  HPDF_Page Page;
};

//------------------------------------------------------------------------------
// svtkPDFExporter
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
svtkStandardNewMacro(svtkPDFExporter);

//------------------------------------------------------------------------------
void svtkPDFExporter::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
svtkPDFExporter::svtkPDFExporter()
  : Title(nullptr)
  , FileName(nullptr)
  , Impl(new Details)
{
  this->SetTitle("SVTK Exported Scene");
}

//------------------------------------------------------------------------------
svtkPDFExporter::~svtkPDFExporter()
{
  this->SetTitle(nullptr);
  this->SetFileName(nullptr);
  delete this->Impl;
}

//------------------------------------------------------------------------------
void svtkPDFExporter::WriteData()
{
  if (!this->FileName || !*this->FileName)
  {
    svtkErrorMacro("FileName not specified.");
    return;
  }

  this->Impl->Document = HPDF_New(handle_libharu_error, nullptr);

  if (!this->Impl->Document)
  {
    svtkErrorMacro("Error initializing LibHaru PDF document: HPDF_New failed.");
    return;
  }

  try
  {
    this->WritePDF();
    HPDF_SaveToFile(this->Impl->Document, this->FileName);
  }
  catch (std::runtime_error& e)
  {
    svtkErrorMacro(<< e.what());
  }

  HPDF_Free(this->Impl->Document);
}

//------------------------------------------------------------------------------
void svtkPDFExporter::WritePDF()
{
  this->PrepareDocument();
  this->RenderContextActors();
}

//------------------------------------------------------------------------------
void svtkPDFExporter::PrepareDocument()
{
  // Compress everything:
  HPDF_SetCompressionMode(this->Impl->Document, HPDF_COMP_ALL);

  // Various metadata:
  HPDF_SetInfoAttr(this->Impl->Document, HPDF_INFO_CREATOR, "The Visualization ToolKit");
  HPDF_SetInfoAttr(this->Impl->Document, HPDF_INFO_TITLE, this->Title);

  this->Impl->Page = HPDF_AddPage(this->Impl->Document);
  HPDF_Page_SetWidth(this->Impl->Page, this->RenderWindow->GetSize()[0]);
  HPDF_Page_SetHeight(this->Impl->Page, this->RenderWindow->GetSize()[1]);
}

//------------------------------------------------------------------------------
void svtkPDFExporter::RenderContextActors()
{
  svtkRendererCollection* renCol = this->RenderWindow->GetRenderers();
  int numLayers = this->RenderWindow->GetNumberOfLayers();

  for (int i = 0; i < numLayers; ++i)
  {
    svtkCollectionSimpleIterator renIt;
    svtkRenderer* ren;
    for (renCol->InitTraversal(renIt); (ren = renCol->GetNextRenderer(renIt));)
    {
      if (this->ActiveRenderer && ren != this->ActiveRenderer)
      {
        // If ActiveRenderer is specified then ignore all other renderers
        continue;
      }
      if (ren->GetLayer() == i)
      {
        svtkPropCollection* props = ren->GetViewProps();
        svtkCollectionSimpleIterator propIt;
        svtkProp* prop;
        for (props->InitTraversal(propIt); (prop = props->GetNextProp(propIt));)
        {
          svtkContextActor* actor = svtkContextActor::SafeDownCast(prop);
          if (actor)
          {
            this->RenderContextActor(actor, ren);
          }
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void svtkPDFExporter::RenderContextActor(svtkContextActor* actor, svtkRenderer* ren)
{
  svtkContextDevice2D* oldForceDevice = actor->GetForceDevice();

  svtkNew<svtkPDFContextDevice2D> device;
  device->SetHaruObjects(&this->Impl->Document, &this->Impl->Page);
  device->SetRenderer(ren);
  actor->SetForceDevice(device);

  actor->RenderOverlay(ren);

  actor->SetForceDevice(oldForceDevice);
}
