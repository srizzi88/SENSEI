/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTextMapper.h"

#include "svtkActor2D.h"
#include "svtkCellArray.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkRenderer.h"
#include "svtkStdString.h"
#include "svtkTextProperty.h"
#include "svtkTextRenderer.h"
#include "svtkTexture.h"
#include "svtkWindow.h"

#include <algorithm>

//----------------------------------------------------------------------------
svtkObjectFactoryNewMacro(svtkTextMapper);
//----------------------------------------------------------------------------
svtkCxxSetObjectMacro(svtkTextMapper, TextProperty, svtkTextProperty);

//----------------------------------------------------------------------------
// Creates a new text mapper
svtkTextMapper::svtkTextMapper()
{
  this->Input = nullptr;
  this->TextProperty = nullptr;

  this->RenderedDPI = 0;

  svtkNew<svtkTextProperty> tprop;
  this->SetTextProperty(tprop);

  this->Points->SetNumberOfPoints(4);
  this->Points->SetPoint(0, 0., 0., 0.);
  this->Points->SetPoint(1, 0., 0., 0.);
  this->Points->SetPoint(2, 0., 0., 0.);
  this->Points->SetPoint(3, 0., 0., 0.);
  this->PolyData->SetPoints(this->Points);

  svtkNew<svtkCellArray> quad;
  quad->InsertNextCell(4);
  quad->InsertCellPoint(0);
  quad->InsertCellPoint(1);
  quad->InsertCellPoint(2);
  quad->InsertCellPoint(3);
  this->PolyData->SetPolys(quad);

  svtkNew<svtkFloatArray> tcoords;
  tcoords->SetNumberOfComponents(2);
  tcoords->SetNumberOfTuples(4);
  tcoords->SetTuple2(0, 0., 0.);
  tcoords->SetTuple2(1, 0., 0.);
  tcoords->SetTuple2(2, 0., 0.);
  tcoords->SetTuple2(3, 0., 0.);
  this->PolyData->GetPointData()->SetTCoords(tcoords);
  this->Mapper->SetInputData(this->PolyData);

  this->Texture->SetInputData(this->Image);
  this->TextDims[0] = this->TextDims[1] = 0;
}

//----------------------------------------------------------------------------
// Shallow copy of an actor.
void svtkTextMapper::ShallowCopy(svtkAbstractMapper* m)
{
  auto tm = svtkTextMapper::SafeDownCast(m);
  if (tm != nullptr)
  {
    this->SetInput(tm->GetInput());
    this->SetTextProperty(tm->GetTextProperty());
  }

  // Now do superclass
  this->svtkMapper2D::ShallowCopy(m);
}

//----------------------------------------------------------------------------
svtkTextMapper::~svtkTextMapper()
{
  delete[] this->Input;
  this->SetTextProperty(nullptr);
}

//----------------------------------------------------------------------------
void svtkTextMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->TextProperty)
  {
    os << indent << "Text Property:\n";
    this->TextProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Text Property: (none)\n";
  }

  os << indent << "Input: " << (this->Input ? this->Input : "(none)") << "\n";

  os << indent << "TextDims: " << this->TextDims[0] << ", " << this->TextDims[1] << "\n";

  os << indent << "CoordsTime: " << this->CoordsTime.GetMTime() << "\n";
  os << indent << "TCoordsTime: " << this->TCoordsTime.GetMTime() << "\n";
  os << indent << "Image:\n";
  this->Image->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Points:\n";
  this->Points->PrintSelf(os, indent.GetNextIndent());
  os << indent << "PolyData:\n";
  this->PolyData->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Mapper:\n";
  this->Mapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Texture:\n";
  this->Texture->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
void svtkTextMapper::GetSize(svtkViewport* vp, int size[2])
{
  svtkWindow* win = vp ? vp->GetSVTKWindow() : nullptr;
  if (!win)
  {
    size[0] = size[1] = 0;
    svtkErrorMacro(<< "No render window available: cannot determine DPI.");
    return;
  }

  this->UpdateImage(win->GetDPI());
  size[0] = this->TextDims[0];
  size[1] = this->TextDims[1];
}

//----------------------------------------------------------------------------
int svtkTextMapper::GetWidth(svtkViewport* viewport)
{
  int size[2];
  this->GetSize(viewport, size);
  return size[0];
}

//----------------------------------------------------------------------------
int svtkTextMapper::GetHeight(svtkViewport* viewport)
{
  int size[2];
  this->GetSize(viewport, size);
  return size[1];
}

//----------------------------------------------------------------------------
int svtkTextMapper::SetConstrainedFontSize(svtkViewport* viewport, int targetWidth, int targetHeight)
{
  return this->SetConstrainedFontSize(this, viewport, targetWidth, targetHeight);
}

//----------------------------------------------------------------------------
int svtkTextMapper::SetConstrainedFontSize(
  svtkTextMapper* tmapper, svtkViewport* viewport, int targetWidth, int targetHeight)
{
  // If target "empty" just return
  if (targetWidth == 0 && targetHeight == 0)
  {
    return 0;
  }

  svtkTextProperty* tprop = tmapper->GetTextProperty();
  if (!tprop)
  {
    svtkGenericWarningMacro(<< "Need text property to apply constraint");
    return 0;
  }
  int fontSize = tprop->GetFontSize();

  // Use the last size as a first guess
  int tempi[2];
  tmapper->GetSize(viewport, tempi);

  // Now get an estimate of the target font size using bissection
  // Based on experimentation with big and small font size increments,
  // ceil() gives the best result.
  // big:   floor: 10749, ceil: 10106, cast: 10749, svtkMath::Round: 10311
  // small: floor: 12122, ceil: 11770, cast: 12122, svtkMath::Round: 11768
  // I guess the best optim would be to have a look at the shape of the
  // font size growth curve (probably not that linear)
  if (tempi[0] && tempi[1])
  {
    float fx = targetWidth / static_cast<float>(tempi[0]);
    float fy = targetHeight / static_cast<float>(tempi[1]);
    fontSize = static_cast<int>(ceil(fontSize * ((fx <= fy) ? fx : fy)));
    tprop->SetFontSize(fontSize);
    tmapper->GetSize(viewport, tempi);
  }

  // While the size is too small increase it
  while (tempi[1] <= targetHeight && tempi[0] <= targetWidth && fontSize < 100)
  {
    fontSize++;
    tprop->SetFontSize(fontSize);
    tmapper->GetSize(viewport, tempi);
  }

  // While the size is too large decrease it
  while ((tempi[1] > targetHeight || tempi[0] > targetWidth) && fontSize > 0)
  {
    fontSize--;
    tprop->SetFontSize(fontSize);
    tmapper->GetSize(viewport, tempi);
  }

  return fontSize;
}

//----------------------------------------------------------------------------
int svtkTextMapper::SetMultipleConstrainedFontSize(svtkViewport* viewport, int targetWidth,
  int targetHeight, svtkTextMapper** mappers, int nbOfMappers, int* maxResultingSize)
{
  maxResultingSize[0] = maxResultingSize[1] = 0;

  if (nbOfMappers == 0)
  {
    return 0;
  }

  int fontSize, aSize;

  // First try to find the constrained font size of the first mapper: it
  // will be used minimize the search for the remaining mappers, given the
  // fact that all mappers are likely to have the same constrained font size.
  int i, first;
  for (first = 0; first < nbOfMappers && !mappers[first]; first++)
  {
  }

  if (first >= nbOfMappers)
  {
    return 0;
  }

  fontSize = mappers[first]->SetConstrainedFontSize(viewport, targetWidth, targetHeight);

  // Find the constrained font size for the remaining mappers and
  // pick the smallest
  for (i = first + 1; i < nbOfMappers; i++)
  {
    if (mappers[i])
    {
      mappers[i]->GetTextProperty()->SetFontSize(fontSize);
      aSize = mappers[i]->SetConstrainedFontSize(viewport, targetWidth, targetHeight);
      if (aSize < fontSize)
      {
        fontSize = aSize;
      }
    }
  }

  // Assign the smallest size to all text mappers and find the largest area
  int tempi[2];
  for (i = first; i < nbOfMappers; i++)
  {
    if (mappers[i])
    {
      mappers[i]->GetTextProperty()->SetFontSize(fontSize);
      mappers[i]->GetSize(viewport, tempi);
      if (tempi[0] > maxResultingSize[0])
      {
        maxResultingSize[0] = tempi[0];
      }
      if (tempi[1] > maxResultingSize[1])
      {
        maxResultingSize[1] = tempi[1];
      }
    }
  }

  // The above code could be optimized further since the mappers
  // labels are likely to have the same height: in that case, we could
  // have searched for the largest label, find the constrained size
  // for this one, then applied this size to all others.  But who
  // knows, maybe one day the text property will support a text
  // orientation/rotation, and in that case the height will vary.
  return fontSize;
}

//----------------------------------------------------------------------------
int svtkTextMapper::SetRelativeFontSize(svtkTextMapper* tmapper, svtkViewport* viewport,
  const int* targetSize, int* stringSize, float sizeFactor)
{
  sizeFactor = (sizeFactor <= 0.0f ? 0.015f : sizeFactor);

  int fontSize, targetWidth, targetHeight;
  // Find the best size for the font
  targetWidth = targetSize[0] > targetSize[1] ? targetSize[0] : targetSize[1];
  targetHeight = static_cast<int>(sizeFactor * targetSize[0] + sizeFactor * targetSize[1]);

  fontSize = tmapper->SetConstrainedFontSize(tmapper, viewport, targetWidth, targetHeight);
  tmapper->GetSize(viewport, stringSize);

  return fontSize;
}

//----------------------------------------------------------------------------
int svtkTextMapper::SetMultipleRelativeFontSize(svtkViewport* viewport, svtkTextMapper** textMappers,
  int nbOfMappers, int* targetSize, int* stringSize, float sizeFactor)
{
  int fontSize, targetWidth, targetHeight;

  // Find the best size for the font
  // WARNING: check that the below values are in sync with the above
  // similar function.

  targetWidth = targetSize[0] > targetSize[1] ? targetSize[0] : targetSize[1];

  targetHeight = static_cast<int>(sizeFactor * targetSize[0] + sizeFactor * targetSize[1]);

  fontSize = svtkTextMapper::SetMultipleConstrainedFontSize(
    viewport, targetWidth, targetHeight, textMappers, nbOfMappers, stringSize);

  return fontSize;
}

//----------------------------------------------------------------------------
void svtkTextMapper::RenderOverlay(svtkViewport* viewport, svtkActor2D* actor)
{
  // This is necessary for GL2PS exports when this actor/mapper are part of an
  // composite actor/mapper.
  if (!actor->GetVisibility())
  {
    return;
  }

  svtkDebugMacro(<< "RenderOverlay called");

  svtkRenderer* ren = nullptr;
  if (this->Input && this->Input[0])
  {
    svtkWindow* win = viewport->GetSVTKWindow();
    if (!win)
    {
      svtkErrorMacro(<< "No render window available: cannot determine DPI.");
      return;
    }

    this->UpdateImage(win->GetDPI());
    this->UpdateQuad(actor, win->GetDPI());

    ren = svtkRenderer::SafeDownCast(viewport);
    if (ren)
    {
      svtkDebugMacro(<< "Texture::Render called");
      this->Texture->Render(ren);
      svtkInformation* info = actor->GetPropertyKeys();
      if (!info)
      {
        info = svtkInformation::New();
        actor->SetPropertyKeys(info);
        info->Delete();
      }
      info->Set(svtkProp::GeneralTextureUnit(), this->Texture->GetTextureUnit());
    }

    svtkDebugMacro(<< "PolyData::RenderOverlay called");
    this->Mapper->RenderOverlay(viewport, actor);

    // clean up
    if (ren)
    {
      this->Texture->PostRender(ren);
    }
  }

  svtkDebugMacro(<< "Superclass::RenderOverlay called");
  this->Superclass::RenderOverlay(viewport, actor);
}

//----------------------------------------------------------------------------
void svtkTextMapper::ReleaseGraphicsResources(svtkWindow* win)
{
  this->Superclass::ReleaseGraphicsResources(win);
  this->Mapper->ReleaseGraphicsResources(win);
  this->Texture->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
svtkMTimeType svtkTextMapper::GetMTime()
{
  svtkMTimeType result = this->Superclass::GetMTime();
  result = std::max(result, this->CoordsTime.GetMTime());
  result = std::max(result, this->Image->GetMTime());
  result = std::max(result, this->Points->GetMTime());
  result = std::max(result, this->PolyData->GetMTime());
  result = std::max(result, this->Mapper->GetMTime());
  result = std::max(result, this->Texture->GetMTime());
  return result;
}

//----------------------------------------------------------------------------
void svtkTextMapper::UpdateQuad(svtkActor2D* actor, int dpi)
{
  svtkDebugMacro(<< "UpdateQuad called");

  // Update texture coordinates:
  if (this->Image->GetMTime() > this->TCoordsTime)
  {
    int dims[3];
    this->Image->GetDimensions(dims);

    // The coordinates are calculated to be centered on a texel and
    // trim the padding from the image. (padding is often added to
    // create textures that have power-of-two dimensions)
    float tw = static_cast<float>(this->TextDims[0]);
    float th = static_cast<float>(this->TextDims[1]);
    float iw = static_cast<float>(dims[0]);
    float ih = static_cast<float>(dims[1]);
    float tcXMin = 0;
    float tcYMin = 0;
    float tcXMax = static_cast<float>(tw) / iw;
    float tcYMax = static_cast<float>(th) / ih;
    if (svtkFloatArray* tc =
          svtkArrayDownCast<svtkFloatArray>(this->PolyData->GetPointData()->GetTCoords()))
    {
      svtkDebugMacro(<< "Setting tcoords: xmin, xmax, ymin, ymax: " << tcXMin << ", " << tcXMax
                    << ", " << tcYMin << ", " << tcYMax);
      tc->Reset();
      tc->InsertNextValue(tcXMin);
      tc->InsertNextValue(tcYMin);

      tc->InsertNextValue(tcXMin);
      tc->InsertNextValue(tcYMax);

      tc->InsertNextValue(tcXMax);
      tc->InsertNextValue(tcYMax);

      tc->InsertNextValue(tcXMax);
      tc->InsertNextValue(tcYMin);
      tc->Modified();

      this->TCoordsTime.Modified();
    }
    else
    {
      svtkErrorMacro(<< "Invalid texture coordinate array type.");
    }
  }

  if (this->CoordsTime < actor->GetMTime() || this->CoordsTime < this->TextProperty->GetMTime() ||
    this->CoordsTime < this->TCoordsTime)
  {
    int text_bbox[4];
    svtkTextRenderer* tren = svtkTextRenderer::GetInstance();
    if (tren)
    {
      if (!tren->GetBoundingBox(
            this->TextProperty, this->Input ? this->Input : std::string(), text_bbox, dpi))
      {
        svtkErrorMacro(<< "Error calculating bounding box.");
      }
    }
    else
    {
      svtkErrorMacro(<< "Could not locate svtkTextRenderer object.");
      text_bbox[0] = 0;
      text_bbox[2] = 0;
    }
    // adjust the quad so that the anchor point and a point with the same
    // coordinates fall on the same pixel.
    double shiftPixel = 1;
    double x = static_cast<double>(text_bbox[0]);
    double y = static_cast<double>(text_bbox[2]);
    double w = static_cast<double>(this->TextDims[0]);
    double h = static_cast<double>(this->TextDims[1]);

    this->Points->Reset();
    this->Points->InsertNextPoint(x - shiftPixel, y - shiftPixel, 0.);
    this->Points->InsertNextPoint(x - shiftPixel, y + h - shiftPixel, 0.);
    this->Points->InsertNextPoint(x + w - shiftPixel, y + h - shiftPixel, 0.);
    this->Points->InsertNextPoint(x + w - shiftPixel, y - shiftPixel, 0.);
    this->Points->Modified();
    this->CoordsTime.Modified();
  }
}

//----------------------------------------------------------------------------
void svtkTextMapper::UpdateImage(int dpi)
{
  svtkDebugMacro(<< "UpdateImage called");
  if (this->MTime > this->Image->GetMTime() || this->RenderedDPI != dpi ||
    this->TextProperty->GetMTime() > this->Image->GetMTime())
  {
    svtkTextRenderer* tren = svtkTextRenderer::GetInstance();
    if (tren)
    {
      if (!tren->RenderString(this->TextProperty, this->Input ? this->Input : std::string(),
            this->Image, this->TextDims, dpi))
      {
        svtkErrorMacro(<< "Texture generation failed.");
      }
      this->RenderedDPI = dpi;
      svtkDebugMacro(<< "Text rendered to " << this->TextDims[0] << ", " << this->TextDims[1]
                    << " buffer.");
    }
    else
    {
      svtkErrorMacro(<< "Could not locate svtkTextRenderer object.");
    }
  }
}
