/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLGL2PSExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenGLGL2PSExporter.h"

#include "svtkImageData.h"
#include "svtkImageShiftScale.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLGL2PSHelper.h"
#include "svtkRenderWindow.h"
#include "svtkWindowToImageFilter.h"
#include <svtksys/SystemTools.hxx>

#include "svtk_gl2ps.h"

#include <algorithm>
#include <cstdio>
#include <sstream>
#include <string>

svtkStandardNewMacro(svtkOpenGLGL2PSExporter);

//------------------------------------------------------------------------------
void svtkOpenGLGL2PSExporter::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
svtkOpenGLGL2PSExporter::svtkOpenGLGL2PSExporter() = default;

//------------------------------------------------------------------------------
svtkOpenGLGL2PSExporter::~svtkOpenGLGL2PSExporter() = default;

//------------------------------------------------------------------------------
void svtkOpenGLGL2PSExporter::WriteData()
{
  // Open file:
  if (this->FilePrefix == nullptr)
  {
    svtkErrorMacro(<< "Please specify a file prefix to use");
    return;
  }

  std::ostringstream fname;
  fname << this->FilePrefix << "." << this->GetFileExtension();
  if (this->Compress && this->FileFormat != PDF_FILE)
  {
    fname << ".gz";
  }
  FILE* file = svtksys::SystemTools::Fopen(fname.str(), "wb");
  if (!file)
  {
    svtkErrorMacro("Unable to open file: " << fname.str());
    return;
  }

  // Setup information that GL2PS will need to export the scene:
  std::string title = (this->Title && this->Title[0]) ? this->Title : "SVTK GL2PS Export";
  GLint options = static_cast<GLint>(this->GetGL2PSOptions());
  GLint sort = static_cast<GLint>(this->GetGL2PSSort());
  GLint format = static_cast<GLint>(this->GetGL2PSFormat());
  const int* winsize = this->RenderWindow->GetSize();
  GLint viewport[4] = { 0, 0, static_cast<GLint>(winsize[0]), static_cast<GLint>(winsize[1]) };

  // Setup helper class:
  svtkNew<svtkOpenGLGL2PSHelper> gl2ps;
  gl2ps->SetInstance(gl2ps);
  gl2ps->SetTextAsPath(this->TextAsPath);
  gl2ps->SetRenderWindow(this->RenderWindow);

  // Grab the image background:
  svtkNew<svtkImageData> background;
  if (!this->RasterizeBackground(background))
  {
    svtkErrorMacro("Error rasterizing background image. Exported image may be "
                  "incorrect.");
    background->Initialize();
    // Continue with export.
  }

  // Fixup options for no-opengl-context GL2PS rendering (we don't use the
  // context since we inject all geometry manually):
  options |= GL2PS_NO_OPENGL_CONTEXT | GL2PS_NO_BLENDING;
  // Print warning if the user requested no background -- we always draw it.
  if ((options & GL2PS_DRAW_BACKGROUND) == GL2PS_NONE)
  {
    svtkWarningMacro("Ignoring DrawBackground=false setting. The background is "
                    "always drawn on the OpenGL2 backend for GL2PS exports.");
  }
  // Turn the option off -- we don't want GL2PS drawing the background, it
  // comes from the raster image we draw in the image.
  options &= ~GL2PS_DRAW_BACKGROUND;

  // Export file. No worries about buffersize, since we're manually adding
  // geometry through svtkOpenGLGL2PSHelper::ProcessTransformFeedback.
  GLint err = gl2psBeginPage(title.c_str(), "SVTK", viewport, format, sort, options, GL_RGBA, 0,
    nullptr, 0, 0, 0, 0, file, fname.str().c_str());
  if (err != GL2PS_SUCCESS)
  {
    svtkErrorMacro("Error calling gl2psBeginPage. Error code: " << err);
    gl2ps->SetInstance(nullptr);
    fclose(file);
    return;
  }

  if (background->GetNumberOfPoints() > 0)
  {
    int dims[3];
    background->GetDimensions(dims);
    GL2PSvertex rasterPos;
    rasterPos.xyz[0] = 0.f;
    rasterPos.xyz[1] = 0.f;
    rasterPos.xyz[2] = 1.f;
    std::fill(rasterPos.rgba, rasterPos.rgba + 4, 0.f);

    gl2psForceRasterPos(&rasterPos);
    gl2psDrawPixels(dims[0], dims[1], 0, 0, GL_RGB, GL_FLOAT, background->GetScalarPointer());
    background->ReleaseData();
  }

  // Render the scene:
  if (!this->CaptureVectorProps())
  {
    svtkErrorMacro("Error capturing vectorizable props. Resulting image "
                  "may be incorrect.");
  }

  // Cleanup
  err = gl2psEndPage();
  gl2ps->SetInstance(nullptr);
  fclose(file);

  switch (err)
  {
    case GL2PS_SUCCESS:
      break;
    case GL2PS_NO_FEEDBACK:
      svtkErrorMacro("No data captured by GL2PS for vector graphics export.");
      break;
    default:
      svtkErrorMacro("Error calling gl2psEndPage. Error code: " << err);
      break;
  }

  // Re-render the window to remove any lingering after-effects...
  this->RenderWindow->Render();
}

bool svtkOpenGLGL2PSExporter::RasterizeBackground(svtkImageData* image)
{
  svtkNew<svtkWindowToImageFilter> windowToImage;
  windowToImage->SetInput(this->RenderWindow);
  windowToImage->SetInputBufferTypeToRGB();
  windowToImage->SetReadFrontBuffer(false);

  svtkNew<svtkImageShiftScale> byteToFloat;
  byteToFloat->SetOutputScalarTypeToFloat();
  byteToFloat->SetScale(1.0 / 255.0);
  byteToFloat->SetInputConnection(windowToImage->GetOutputPort());

  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
  gl2ps->SetActiveState(svtkOpenGLGL2PSHelper::Background);
  // Render twice to set the backbuffer:
  this->RenderWindow->Render();
  this->RenderWindow->Render();
  byteToFloat->Update();
  gl2ps->SetActiveState(svtkOpenGLGL2PSHelper::Inactive);

  image->ShallowCopy(byteToFloat->GetOutput());

  return true;
}

bool svtkOpenGLGL2PSExporter::CaptureVectorProps()
{
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
  gl2ps->SetActiveState(svtkOpenGLGL2PSHelper::Capture);
  this->RenderWindow->Render();
  gl2ps->SetActiveState(svtkOpenGLGL2PSHelper::Inactive);

  return true;
}
