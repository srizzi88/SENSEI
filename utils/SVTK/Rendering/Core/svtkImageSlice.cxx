/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageSlice.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageSlice.h"

#include "svtkCamera.h"
#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkImageMapper3D.h"
#include "svtkImageProperty.h"
#include "svtkLinearTransform.h"
#include "svtkLookupTable.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkRenderer.h"
#include "svtkTransform.h"

#include <cmath>

svtkStandardNewMacro(svtkImageSlice);

//----------------------------------------------------------------------------
class svtkImageToImageMapper3DFriendship
{
public:
  static void SetCurrentProp(svtkImageMapper3D* mapper, svtkImageSlice* prop)
  {
    mapper->CurrentProp = prop;
  }
  static void SetCurrentRenderer(svtkImageMapper3D* mapper, svtkRenderer* ren)
  {
    mapper->CurrentRenderer = ren;
  }
  static void SetStackedImagePass(svtkImageMapper3D* mapper, int pass)
  {
    switch (pass)
    {
      case 0:
        mapper->MatteEnable = true;
        mapper->ColorEnable = false;
        mapper->DepthEnable = false;
        break;
      case 1:
        mapper->MatteEnable = false;
        mapper->ColorEnable = true;
        mapper->DepthEnable = false;
        break;
      case 2:
        mapper->MatteEnable = false;
        mapper->ColorEnable = false;
        mapper->DepthEnable = true;
        break;
      default:
        mapper->MatteEnable = true;
        mapper->ColorEnable = true;
        mapper->DepthEnable = true;
        break;
    }
  }
};

//----------------------------------------------------------------------------
svtkImageSlice::svtkImageSlice()
{
  this->Mapper = nullptr;
  this->Property = nullptr;

  this->ForceTranslucent = false;
}

//----------------------------------------------------------------------------
svtkImageSlice::~svtkImageSlice()
{
  if (this->Property)
  {
    this->Property->UnRegister(this);
  }

  this->SetMapper(nullptr);
}

//----------------------------------------------------------------------------
void svtkImageSlice::GetImages(svtkPropCollection* vc)
{
  vc->AddItem(this);
}

//----------------------------------------------------------------------------
void svtkImageSlice::ShallowCopy(svtkProp* prop)
{
  svtkImageSlice* v = svtkImageSlice::SafeDownCast(prop);

  if (v != nullptr)
  {
    this->SetMapper(v->GetMapper());
    this->SetProperty(v->GetProperty());
  }

  // Now do superclass
  this->svtkProp3D::ShallowCopy(prop);
}

//----------------------------------------------------------------------------
void svtkImageSlice::SetMapper(svtkImageMapper3D* mapper)
{
  if (this->Mapper != mapper)
  {
    if (this->Mapper != nullptr)
    {
      svtkImageToImageMapper3DFriendship::SetCurrentProp(this->Mapper, nullptr);
      this->Mapper->UnRegister(this);
    }
    this->Mapper = mapper;
    if (this->Mapper != nullptr)
    {
      this->Mapper->Register(this);
      svtkImageToImageMapper3DFriendship::SetCurrentProp(this->Mapper, this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// Get the bounds for this Volume as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double* svtkImageSlice::GetBounds()
{
  int i, n;
  double bbox[24], *fptr;

  // get the bounds of the Mapper if we have one
  if (!this->Mapper)
  {
    return this->Bounds;
  }

  const double* bounds = this->Mapper->GetBounds();
  // Check for the special case when the mapper's bounds are unknown
  if (!bounds)
  {
    return nullptr;
  }

  // fill out vertices of a bounding box
  bbox[0] = bounds[1];
  bbox[1] = bounds[3];
  bbox[2] = bounds[5];
  bbox[3] = bounds[1];
  bbox[4] = bounds[2];
  bbox[5] = bounds[5];
  bbox[6] = bounds[0];
  bbox[7] = bounds[2];
  bbox[8] = bounds[5];
  bbox[9] = bounds[0];
  bbox[10] = bounds[3];
  bbox[11] = bounds[5];
  bbox[12] = bounds[1];
  bbox[13] = bounds[3];
  bbox[14] = bounds[4];
  bbox[15] = bounds[1];
  bbox[16] = bounds[2];
  bbox[17] = bounds[4];
  bbox[18] = bounds[0];
  bbox[19] = bounds[2];
  bbox[20] = bounds[4];
  bbox[21] = bounds[0];
  bbox[22] = bounds[3];
  bbox[23] = bounds[4];

  // make sure matrix (transform) is up-to-date
  this->ComputeMatrix();

  // and transform into actors coordinates
  fptr = bbox;
  for (n = 0; n < 8; n++)
  {
    double homogeneousPt[4] = { fptr[0], fptr[1], fptr[2], 1.0 };
    this->Matrix->MultiplyPoint(homogeneousPt, homogeneousPt);
    fptr[0] = homogeneousPt[0] / homogeneousPt[3];
    fptr[1] = homogeneousPt[1] / homogeneousPt[3];
    fptr[2] = homogeneousPt[2] / homogeneousPt[3];
    fptr += 3;
  }

  // now calc the new bounds
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = SVTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -SVTK_DOUBLE_MAX;
  for (i = 0; i < 8; i++)
  {
    for (n = 0; n < 3; n++)
    {
      if (bbox[i * 3 + n] < this->Bounds[n * 2])
      {
        this->Bounds[n * 2] = bbox[i * 3 + n];
      }
      if (bbox[i * 3 + n] > this->Bounds[n * 2 + 1])
      {
        this->Bounds[n * 2 + 1] = bbox[i * 3 + n];
      }
    }
  }

  return this->Bounds;
}

//----------------------------------------------------------------------------
// Get the minimum X bound
double svtkImageSlice::GetMinXBound()
{
  this->GetBounds();
  return this->Bounds[0];
}

// Get the maximum X bound
double svtkImageSlice::GetMaxXBound()
{
  this->GetBounds();
  return this->Bounds[1];
}

// Get the minimum Y bound
double svtkImageSlice::GetMinYBound()
{
  this->GetBounds();
  return this->Bounds[2];
}

// Get the maximum Y bound
double svtkImageSlice::GetMaxYBound()
{
  this->GetBounds();
  return this->Bounds[3];
}

// Get the minimum Z bound
double svtkImageSlice::GetMinZBound()
{
  this->GetBounds();
  return this->Bounds[4];
}

// Get the maximum Z bound
double svtkImageSlice::GetMaxZBound()
{
  this->GetBounds();
  return this->Bounds[5];
}

//----------------------------------------------------------------------------
// Does this prop have some translucent polygonal geometry?
svtkTypeBool svtkImageSlice::HasTranslucentPolygonalGeometry()
{
  if (this->ForceTranslucent)
  {
    return 1;
  }

  // Unless forced to translucent, always render during opaque pass,
  // to keep the behavior predictable and because depth-peeling kills
  // alpha-blending.
  return 0;
}

//----------------------------------------------------------------------------
int svtkImageSlice::RenderTranslucentPolygonalGeometry(svtkViewport* viewport)
{
  svtkDebugMacro(<< "svtkImageSlice::RenderTranslucentPolygonalGeometry");

  if (this->HasTranslucentPolygonalGeometry())
  {
    this->Render(svtkRenderer::SafeDownCast(viewport));
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
int svtkImageSlice::RenderOpaqueGeometry(svtkViewport* viewport)
{
  svtkDebugMacro(<< "svtkImageSlice::RenderOpaqueGeometry");

  if (!this->HasTranslucentPolygonalGeometry())
  {
    this->Render(svtkRenderer::SafeDownCast(viewport));
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
int svtkImageSlice::RenderOverlay(svtkViewport* svtkNotUsed(viewport))
{
  svtkDebugMacro(<< "svtkImageSlice::RenderOverlay");

  // Render the image as an underlay

  return 0;
}

//----------------------------------------------------------------------------
void svtkImageSlice::Render(svtkRenderer* ren)
{
  // Force the creation of a property
  if (!this->Property)
  {
    this->GetProperty();
  }

  if (!this->Property)
  {
    svtkErrorMacro(<< "Error generating a property!\n");
    return;
  }

  if (!this->Mapper)
  {
    svtkErrorMacro(<< "You must specify a mapper!\n");
    return;
  }

  svtkImageToImageMapper3DFriendship::SetCurrentRenderer(this->Mapper, ren);

  this->Update();

  // only call the mapper if it has an input
  svtkImageData* input = this->Mapper->GetInput();
  int* extent = input->GetExtent();
  if (input && extent[0] <= extent[1] && extent[2] <= extent[3] && extent[4] <= extent[5])
  {
    this->Mapper->Render(ren, this);
    this->EstimatedRenderTime += this->Mapper->GetTimeToDraw();
  }

  svtkImageToImageMapper3DFriendship::SetCurrentRenderer(this->Mapper, nullptr);
}

//----------------------------------------------------------------------------
void svtkImageSlice::ReleaseGraphicsResources(svtkWindow* win)
{
  // pass this information onto the mapper
  if (this->Mapper)
  {
    this->Mapper->ReleaseGraphicsResources(win);
  }
}

//----------------------------------------------------------------------------
void svtkImageSlice::Update()
{
  if (this->Mapper)
  {
    svtkImageToImageMapper3DFriendship::SetCurrentProp(this->Mapper, this);
    this->Mapper->Update();
  }
}

//----------------------------------------------------------------------------
void svtkImageSlice::SetProperty(svtkImageProperty* property)
{
  if (this->Property != property)
  {
    if (this->Property != nullptr)
    {
      this->Property->UnRegister(this);
    }
    this->Property = property;
    if (this->Property != nullptr)
    {
      this->Property->Register(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
svtkImageProperty* svtkImageSlice::GetProperty()
{
  if (this->Property == nullptr)
  {
    this->Property = svtkImageProperty::New();
    this->Property->Register(this);
    this->Property->Delete();
  }
  return this->Property;
}

//----------------------------------------------------------------------------
svtkMTimeType svtkImageSlice::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;

  if (this->Property != nullptr)
  {
    time = this->Property->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  if (this->UserMatrix != nullptr)
  {
    time = this->UserMatrix->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  if (this->UserTransform != nullptr)
  {
    time = this->UserTransform->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
svtkMTimeType svtkImageSlice::GetRedrawMTime()
{
  svtkMTimeType mTime = this->GetMTime();
  svtkMTimeType time;

  if (this->Mapper != nullptr)
  {
    time = this->Mapper->GetMTime();
    mTime = (time > mTime ? time : mTime);
    if (this->GetMapper()->GetInputAlgorithm() != nullptr)
    {
      this->GetMapper()->GetInputAlgorithm()->Update();
      time = this->Mapper->GetInput()->GetMTime();
      mTime = (time > mTime ? time : mTime);
    }
  }

  if (this->Property != nullptr)
  {
    time = this->Property->GetMTime();
    mTime = (time > mTime ? time : mTime);

    if (this->Property->GetLookupTable() != nullptr)
    {
      // check the lookup table mtime
      time = this->Property->GetLookupTable()->GetMTime();
      mTime = (time > mTime ? time : mTime);
    }
  }

  return mTime;
}

//----------------------------------------------------------------------------
void svtkImageSlice::SetStackedImagePass(int pass)
{
  if (this->Mapper)
  {
    svtkImageToImageMapper3DFriendship::SetStackedImagePass(this->Mapper, pass);
  }
}

//----------------------------------------------------------------------------
void svtkImageSlice::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Property)
  {
    os << indent << "Property:\n";
    this->Property->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Property: (not defined)\n";
  }

  if (this->Mapper)
  {
    os << indent << "Mapper:\n";
    this->Mapper->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Mapper: (not defined)\n";
  }

  // make sure our bounds are up to date
  if (this->Mapper)
  {
    this->GetBounds();
    os << indent << "Bounds: (" << this->Bounds[0] << ", " << this->Bounds[1] << ") ("
       << this->Bounds[2] << ") (" << this->Bounds[3] << ") (" << this->Bounds[4] << ") ("
       << this->Bounds[5] << ")\n";
  }
  else
  {
    os << indent << "Bounds: (not defined)\n";
  }

  os << indent << "ForceTranslucent: " << (this->ForceTranslucent ? "On\n" : "Off\n");
}
