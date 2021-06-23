/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVolume.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkVolume.h"

#include "svtkAbstractVolumeMapper.h"
#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkLinearTransform.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPointData.h"
#include "svtkRenderer.h"
#include "svtkTransform.h"
#include "svtkVolumeCollection.h"
#include "svtkVolumeProperty.h"

#include <cmath>

svtkStandardNewMacro(svtkVolume);

// Creates a Volume with the following defaults: origin(0,0,0)
// position=(0,0,0) scale=1 visibility=1 pickable=1 dragable=1
// orientation=(0,0,0).
svtkVolume::svtkVolume()
{
  this->Mapper = nullptr;
  this->Property = nullptr;

  for (int i = 0; i < SVTK_MAX_VRCOMP; i++)
  {
    this->ScalarOpacityArray[i] = nullptr;
    this->RGBArray[i] = nullptr;
    this->GrayArray[i] = nullptr;
    this->CorrectedScalarOpacityArray[i] = nullptr;
    this->GradientOpacityConstant[i] = 0;
  }

  this->CorrectedStepSize = -1;
  this->ArraySize = 0;
}

// Destruct a volume
svtkVolume::~svtkVolume()
{
  if (this->Property)
  {
    this->Property->UnRegister(this);
  }

  this->SetMapper(nullptr);

  for (int i = 0; i < SVTK_MAX_VRCOMP; i++)
  {
    delete[] this->ScalarOpacityArray[i];
    delete[] this->RGBArray[i];
    delete[] this->GrayArray[i];
    delete[] this->CorrectedScalarOpacityArray[i];
  }
}

void svtkVolume::GetVolumes(svtkPropCollection* vc)
{
  vc->AddItem(this);
}

// Shallow copy of an volume.
void svtkVolume::ShallowCopy(svtkProp* prop)
{
  svtkVolume* v = svtkVolume::SafeDownCast(prop);

  if (v != nullptr)
  {
    this->SetMapper(v->GetMapper());
    this->SetProperty(v->GetProperty());
  }

  // Now do superclass
  this->svtkProp3D::ShallowCopy(prop);
}

float* svtkVolume::GetScalarOpacityArray(int index)
{
  if (index < 0 || index >= SVTK_MAX_VRCOMP)
  {
    svtkErrorMacro("Index out of range [0-" << SVTK_MAX_VRCOMP << "]: " << index);
    return nullptr;
  }
  return this->ScalarOpacityArray[index];
}

float* svtkVolume::GetCorrectedScalarOpacityArray(int index)
{
  if (index < 0 || index >= SVTK_MAX_VRCOMP)
  {
    svtkErrorMacro("Index out of range [0-" << SVTK_MAX_VRCOMP << "]: " << index);
    return nullptr;
  }
  return this->CorrectedScalarOpacityArray[index];
}

float* svtkVolume::GetGradientOpacityArray(int index)
{
  if (index < 0 || index >= SVTK_MAX_VRCOMP)
  {
    svtkErrorMacro("Index out of range [0-" << SVTK_MAX_VRCOMP << "]: " << index);
    return nullptr;
  }
  return this->GradientOpacityArray[index];
}

float svtkVolume::GetGradientOpacityConstant(int index)
{
  if (index < 0 || index >= SVTK_MAX_VRCOMP)
  {
    svtkErrorMacro("Index out of range [0-" << SVTK_MAX_VRCOMP << "]: " << index);
    return 0;
  }
  return this->GradientOpacityConstant[index];
}

float* svtkVolume::GetGrayArray(int index)
{
  if (index < 0 || index >= SVTK_MAX_VRCOMP)
  {
    svtkErrorMacro("Index out of range [0-" << SVTK_MAX_VRCOMP << "]: " << index);
    return nullptr;
  }
  return this->GrayArray[index];
}

float* svtkVolume::GetRGBArray(int index)
{
  if (index < 0 || index >= SVTK_MAX_VRCOMP)
  {
    svtkErrorMacro("Index out of range [0-" << SVTK_MAX_VRCOMP << "]: " << index);
    return nullptr;
  }
  return this->RGBArray[index];
}

void svtkVolume::SetMapper(svtkAbstractVolumeMapper* mapper)
{
  if (this->Mapper != mapper)
  {
    if (this->Mapper != nullptr)
    {
      this->Mapper->UnRegister(this);
    }
    this->Mapper = mapper;
    if (this->Mapper != nullptr)
    {
      this->Mapper->Register(this);
    }
    this->Modified();
  }
}

double svtkVolume::ComputeScreenCoverage(svtkViewport* vp)
{
  double coverage = 1.0;

  svtkRenderer* ren = svtkRenderer::SafeDownCast(vp);

  if (ren)
  {
    svtkCamera* cam = ren->GetActiveCamera();
    ren->ComputeAspect();
    double* aspect = ren->GetAspect();
    svtkMatrix4x4* mat = cam->GetCompositeProjectionTransformMatrix(aspect[0] / aspect[1], 0.0, 1.0);
    const double* bounds = this->GetBounds();
    double minX = 1.0;
    double maxX = -1.0;
    double minY = 1.0;
    double maxY = -1.0;
    int i, j, k;
    double p[4];
    for (k = 0; k < 2; k++)
    {
      for (j = 0; j < 2; j++)
      {
        for (i = 0; i < 2; i++)
        {
          p[0] = bounds[i];
          p[1] = bounds[2 + j];
          p[2] = bounds[4 + k];
          p[3] = 1.0;
          mat->MultiplyPoint(p, p);
          if (p[3])
          {
            p[0] /= p[3];
            p[1] /= p[3];
            p[2] /= p[3];
          }

          minX = (p[0] < minX) ? (p[0]) : (minX);
          minY = (p[1] < minY) ? (p[1]) : (minY);
          maxX = (p[0] > maxX) ? (p[0]) : (maxX);
          maxY = (p[1] > maxY) ? (p[1]) : (maxY);
        }
      }
    }

    coverage = (maxX - minX) * (maxY - minY) * .25;
    coverage = (coverage > 1.0) ? (1.0) : (coverage);
    coverage = (coverage < 0.0) ? (0.0) : (coverage);
  }

  return coverage;
}

// Get the bounds for this Volume as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double* svtkVolume::GetBounds()
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

// Get the minimum X bound
double svtkVolume::GetMinXBound()
{
  this->GetBounds();
  return this->Bounds[0];
}

// Get the maximum X bound
double svtkVolume::GetMaxXBound()
{
  this->GetBounds();
  return this->Bounds[1];
}

// Get the minimum Y bound
double svtkVolume::GetMinYBound()
{
  this->GetBounds();
  return this->Bounds[2];
}

// Get the maximum Y bound
double svtkVolume::GetMaxYBound()
{
  this->GetBounds();
  return this->Bounds[3];
}

// Get the minimum Z bound
double svtkVolume::GetMinZBound()
{
  this->GetBounds();
  return this->Bounds[4];
}

// Get the maximum Z bound
double svtkVolume::GetMaxZBound()
{
  this->GetBounds();
  return this->Bounds[5];
}

// If the volume mapper is of type SVTK_FRAMEBUFFER_VOLUME_MAPPER, then
// this is its opportunity to render
int svtkVolume::RenderVolumetricGeometry(svtkViewport* vp)
{
  this->Update();

  if (!this->Mapper)
  {
    svtkErrorMacro(<< "You must specify a mapper!\n");
    return 0;
  }

  // If we don't have any input return silently
  if (!this->Mapper->GetDataObjectInput())
  {
    return 0;
  }

  // Force the creation of a property
  if (!this->Property)
  {
    this->GetProperty();
  }

  if (!this->Property)
  {
    svtkErrorMacro(<< "Error generating a property!\n");
    return 0;
  }

  this->Mapper->Render(static_cast<svtkRenderer*>(vp), this);
  this->EstimatedRenderTime += this->Mapper->GetTimeToDraw();

  return 1;
}

void svtkVolume::ReleaseGraphicsResources(svtkWindow* win)
{
  // pass this information onto the mapper
  if (this->Mapper)
  {
    this->Mapper->ReleaseGraphicsResources(win);
  }
}

void svtkVolume::Update()
{
  if (this->Mapper)
  {
    this->Mapper->Update();
  }
}

void svtkVolume::SetProperty(svtkVolumeProperty* property)
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
      this->Property->UpdateMTimes();
    }
    this->Modified();
  }
}

svtkVolumeProperty* svtkVolume::GetProperty()
{
  if (this->Property == nullptr)
  {
    this->Property = svtkVolumeProperty::New();
    this->Property->Register(this);
    this->Property->Delete();
  }
  return this->Property;
}

svtkMTimeType svtkVolume::GetMTime()
{
  svtkMTimeType mTime = this->svtkObject::GetMTime();
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

svtkMTimeType svtkVolume::GetRedrawMTime()
{
  svtkMTimeType mTime = this->GetMTime();
  svtkMTimeType time;

  if (this->Mapper != nullptr)
  {
    time = this->Mapper->GetMTime();
    mTime = (time > mTime ? time : mTime);
    if (this->GetMapper()->GetDataSetInput() != nullptr)
    {
      this->GetMapper()->GetInputAlgorithm()->Update();
      time = this->Mapper->GetDataSetInput()->GetMTime();
      mTime = (time > mTime ? time : mTime);
    }
  }

  if (this->Property != nullptr)
  {
    time = this->Property->GetMTime();
    mTime = (time > mTime ? time : mTime);

    int numComponents;

    if (this->Mapper && this->Mapper->GetDataSetInput() &&
      this->Mapper->GetDataSetInput()->GetPointData() &&
      this->Mapper->GetDataSetInput()->GetPointData()->GetScalars())
    {
      numComponents =
        this->Mapper->GetDataSetInput()->GetPointData()->GetScalars()->GetNumberOfComponents();
    }
    else
    {
      numComponents = 0;
    }

    for (int i = 0; i < numComponents; i++)
    {
      // Check the color transfer function (gray or rgb)
      if (this->Property->GetColorChannels(i) == 1)
      {
        time = this->Property->GetGrayTransferFunction(i)->GetMTime();
        mTime = (time > mTime ? time : mTime);
      }
      else
      {
        time = this->Property->GetRGBTransferFunction(i)->GetMTime();
        mTime = (time > mTime ? time : mTime);
      }

      // check the scalar opacity function
      time = this->Property->GetScalarOpacity(i)->GetMTime();
      mTime = (time > mTime ? time : mTime);

      // check the gradient opacity function
      time = this->Property->GetGradientOpacity(i)->GetMTime();
      mTime = (time > mTime ? time : mTime);
    }
  }

  return mTime;
}

void svtkVolume::UpdateTransferFunctions(svtkRenderer* svtkNotUsed(ren))
{
  int dataType;
  svtkPiecewiseFunction* sotf;
  svtkPiecewiseFunction* gotf;
  svtkPiecewiseFunction* graytf;
  svtkColorTransferFunction* rgbtf;
  int colorChannels;

  int arraySize;

  // Check that we have scalars
  if (this->Mapper == nullptr || this->Mapper->GetDataSetInput() == nullptr ||
    this->Mapper->GetDataSetInput()->GetPointData() == nullptr ||
    this->Mapper->GetDataSetInput()->GetPointData()->GetScalars() == nullptr)
  {
    svtkErrorMacro(<< "Need scalar data to volume render");
    return;
  }

  // What is the type of the data?
  dataType = this->Mapper->GetDataSetInput()->GetPointData()->GetScalars()->GetDataType();

  if (dataType == SVTK_UNSIGNED_CHAR)
  {
    arraySize = 256;
  }
  else if (dataType == SVTK_UNSIGNED_SHORT)
  {
    arraySize = 65536;
  }
  else
  {
    svtkErrorMacro("Unsupported data type");
    return;
  }

  int numComponents =
    this->Mapper->GetDataSetInput()->GetPointData()->GetScalars()->GetNumberOfComponents();

  for (int c = 0; c < numComponents; c++)
  {

    // Did our array size change? If so, free up all our previous arrays
    // and create new ones for the scalar opacity and corrected scalar
    // opacity
    if (arraySize != this->ArraySize)
    {
      delete[] this->ScalarOpacityArray[c];
      this->ScalarOpacityArray[c] = nullptr;

      delete[] this->CorrectedScalarOpacityArray[c];
      this->CorrectedScalarOpacityArray[c] = nullptr;

      delete[] this->GrayArray[c];
      this->GrayArray[c] = nullptr;

      delete[] this->RGBArray[c];
      this->RGBArray[c] = nullptr;

      // Allocate these two because we know we need them
      this->ScalarOpacityArray[c] = new float[arraySize];
      this->CorrectedScalarOpacityArray[c] = new float[arraySize];
    }

    // How many color channels for this component?
    colorChannels = this->Property->GetColorChannels(c);

    // If we have 1 color channel and no gray array, create it.
    // Free the rgb array if there is one.
    if (colorChannels == 1)
    {
      delete[] this->RGBArray[c];
      this->RGBArray[c] = nullptr;

      if (!this->GrayArray[c])
      {
        this->GrayArray[c] = new float[arraySize];
      }
    }

    // If we have 3 color channels and no rgb array, create it.
    // Free the gray array if there is one.
    if (colorChannels == 3)
    {
      delete[] this->GrayArray[c];
      this->GrayArray[c] = nullptr;

      if (!this->RGBArray[c])
      {
        this->RGBArray[c] = new float[3 * arraySize];
      }
    }

    // Get the various functions for this index. There is no chance of
    // these being nullptr since the property will create them if they were
    // not defined
    sotf = this->Property->GetScalarOpacity(c);
    gotf = this->Property->GetGradientOpacity(c);

    if (colorChannels == 1)
    {
      rgbtf = nullptr;
      graytf = this->Property->GetGrayTransferFunction(c);
    }
    else
    {
      rgbtf = this->Property->GetRGBTransferFunction(c);
      graytf = nullptr;
    }

    // Update the scalar opacity array if necessary
    if (sotf->GetMTime() > this->ScalarOpacityArrayMTime[c] ||
      this->Property->GetScalarOpacityMTime(c) > this->ScalarOpacityArrayMTime[c])
    {
      sotf->GetTable(
        0.0, static_cast<double>(arraySize - 1), arraySize, this->ScalarOpacityArray[c]);
      this->ScalarOpacityArrayMTime[c].Modified();
    }

    // Update the gradient opacity array if necessary
    if (gotf->GetMTime() > this->GradientOpacityArrayMTime[c] ||
      this->Property->GetGradientOpacityMTime(c) > this->GradientOpacityArrayMTime[c])
    {
      // Get values according to scale/bias from mapper 256 values are
      // in the table, the scale / bias values control what those 256 values
      // mean.
      float scale = this->Mapper->GetGradientMagnitudeScale(c);
      float bias = this->Mapper->GetGradientMagnitudeBias(c);

      float low = -bias;
      float high = 255 / scale - bias;

      gotf->GetTable(low, high, static_cast<int>(0x100), this->GradientOpacityArray[c]);

      if (!strcmp(gotf->GetType(), "Constant"))
      {
        this->GradientOpacityConstant[c] = this->GradientOpacityArray[c][0];
      }
      else
      {
        this->GradientOpacityConstant[c] = -1.0;
      }

      this->GradientOpacityArrayMTime[c].Modified();
    }

    // Update the RGB or Gray transfer function if necessary
    if (colorChannels == 1)
    {
      if (graytf->GetMTime() > this->GrayArrayMTime[c] ||
        this->Property->GetGrayTransferFunctionMTime(c) > this->GrayArrayMTime[c])
      {
        graytf->GetTable(0.0, static_cast<float>(arraySize - 1), arraySize, this->GrayArray[c]);
        this->GrayArrayMTime[c].Modified();
      }
    }
    else
    {
      if (rgbtf->GetMTime() > this->RGBArrayMTime[c] ||
        this->Property->GetRGBTransferFunctionMTime(c) > this->RGBArrayMTime[c])
      {
        rgbtf->GetTable(0.0, static_cast<float>(arraySize - 1), arraySize, this->RGBArray[c]);
        this->RGBArrayMTime[c].Modified();
      }
    }
  }

  // reset the array size to the current size
  this->ArraySize = arraySize;
}

// This method computes the corrected alpha blending for a given
// step size.  The ScalarOpacityArray reflects step size 1.
// The CorrectedScalarOpacityArray reflects step size CorrectedStepSize.
void svtkVolume::UpdateScalarOpacityforSampleSize(
  svtkRenderer* svtkNotUsed(ren), float sample_distance)
{
  int i;
  int needsRecomputing;
  float originalAlpha, correctedAlpha;
  float ray_scale;

  ray_scale = sample_distance;

  // step size changed
  needsRecomputing = this->CorrectedStepSize - ray_scale > 0.0001;

  needsRecomputing = needsRecomputing || this->CorrectedStepSize - ray_scale < -0.0001;

  // Check that we have scalars
  if (this->Mapper == nullptr || this->Mapper->GetDataSetInput() == nullptr ||
    this->Mapper->GetDataSetInput()->GetPointData() == nullptr ||
    this->Mapper->GetDataSetInput()->GetPointData()->GetScalars() == nullptr)
  {
    svtkErrorMacro(<< "Need scalar data to volume render");
    return;
  }

  int numComponents =
    this->Mapper->GetDataSetInput()->GetPointData()->GetScalars()->GetNumberOfComponents();

  if (needsRecomputing)
  {
    this->CorrectedStepSize = ray_scale;
  }

  for (int c = 0; c < numComponents; c++)
  {
    if (needsRecomputing ||
      this->ScalarOpacityArrayMTime[c] > this->CorrectedScalarOpacityArrayMTime[c])
    {
      this->CorrectedScalarOpacityArrayMTime[c].Modified();

      for (i = 0; i < this->ArraySize; i++)
      {
        originalAlpha = *(this->ScalarOpacityArray[c] + i);

        // this test is to accelerate the Transfer function correction
        if (originalAlpha > 0.0001)
        {
          correctedAlpha = 1.0f -
            static_cast<float>(pow(static_cast<double>(1.0f - originalAlpha),
              static_cast<double>(this->CorrectedStepSize)));
        }
        else
        {
          correctedAlpha = originalAlpha;
        }
        *(this->CorrectedScalarOpacityArray[c] + i) = correctedAlpha;
      }
    }
  }
}

void svtkVolume::PrintSelf(ostream& os, svtkIndent indent)
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
}
