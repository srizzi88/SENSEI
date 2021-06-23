/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVolumeProperty.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkVolumeProperty.h"

#include "svtkColorTransferFunction.h"
#include "svtkContourValues.h"
#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPointData.h"

svtkStandardNewMacro(svtkVolumeProperty);

//-----------------------------------------------------------------------------
// Construct a new svtkVolumeProperty with default values
svtkVolumeProperty::svtkVolumeProperty()
{
  this->IndependentComponents = 1;

  this->InterpolationType = SVTK_NEAREST_INTERPOLATION;

  this->UseClippedVoxelIntensity = 0;
  this->ClippedVoxelIntensity = SVTK_FLOAT_MIN;

  for (int i = 0; i < SVTK_MAX_VRCOMP; i++)
  {
    this->ColorChannels[i] = 1;

    this->GrayTransferFunction[i] = nullptr;
    this->RGBTransferFunction[i] = nullptr;
    this->ScalarOpacity[i] = nullptr;
    this->ScalarOpacityUnitDistance[i] = 1.0;
    this->GradientOpacity[i] = nullptr;
    this->TransferFunction2D[i] = nullptr;
    this->DefaultGradientOpacity[i] = nullptr;
    this->DisableGradientOpacity[i] = 0;
    this->TransferFunctionMode = svtkVolumeProperty::TF_1D;

    this->ComponentWeight[i] = 1.0;

    this->Shade[i] = 0;
    this->Ambient[i] = 0.1;
    this->Diffuse[i] = 0.7;
    this->Specular[i] = 0.2;
    this->SpecularPower[i] = 10.0;
  }
}

//-----------------------------------------------------------------------------
// Destruct a svtkVolumeProperty
svtkVolumeProperty::~svtkVolumeProperty()
{
  for (int i = 0; i < SVTK_MAX_VRCOMP; i++)
  {
    if (this->GrayTransferFunction[i] != nullptr)
    {
      this->GrayTransferFunction[i]->UnRegister(this);
    }

    if (this->RGBTransferFunction[i] != nullptr)
    {
      this->RGBTransferFunction[i]->UnRegister(this);
    }

    if (this->ScalarOpacity[i] != nullptr)
    {
      this->ScalarOpacity[i]->UnRegister(this);
    }

    if (this->GradientOpacity[i] != nullptr)
    {
      this->GradientOpacity[i]->UnRegister(this);
    }

    if (this->TransferFunction2D[i] != nullptr)
    {
      this->TransferFunction2D[i]->UnRegister(this);
    }

    if (this->DefaultGradientOpacity[i] != nullptr)
    {
      this->DefaultGradientOpacity[i]->UnRegister(this);
    }
  }
  for (auto it = this->LabelColor.begin(); it != this->LabelColor.end(); ++it)
  {
    if ((*it).second)
    {
      (*it).second->UnRegister(this);
    }
  }
  for (auto it = this->LabelScalarOpacity.begin(); it != this->LabelScalarOpacity.end(); ++it)
  {
    if ((*it).second)
    {
      (*it).second->UnRegister(this);
    }
  }
  for (auto it = this->LabelGradientOpacity.begin(); it != this->LabelGradientOpacity.end(); ++it)
  {
    if ((*it).second)
    {
      (*it).second->UnRegister(this);
    }
  }
}

//-----------------------------------------------------------------------------
void svtkVolumeProperty::DeepCopy(svtkVolumeProperty* p)
{
  if (!p)
  {
    return;
  }

  this->IsoSurfaceValues->DeepCopy(p->IsoSurfaceValues);

  this->SetIndependentComponents(p->GetIndependentComponents());

  this->SetInterpolationType(p->GetInterpolationType());
  this->SetUseClippedVoxelIntensity(p->GetUseClippedVoxelIntensity());
  this->SetClippedVoxelIntensity(p->GetClippedVoxelIntensity());

  for (int i = 0; i < SVTK_MAX_VRCOMP; i++)
  {
    this->SetComponentWeight(i, p->GetComponentWeight(i));

    // Force ColorChannels to the right value and/or create a default tfunc
    // then DeepCopy all the points

    if (p->GetColorChannels(i) > 1)
    {
      this->SetColor(i, this->GetRGBTransferFunction(i));
      this->GetRGBTransferFunction(i)->DeepCopy(p->GetRGBTransferFunction(i));
    }
    else
    {
      this->SetColor(i, this->GetGrayTransferFunction(i));
      this->GetGrayTransferFunction(i)->DeepCopy(p->GetGrayTransferFunction(i));
    }

    this->GetScalarOpacity(i)->DeepCopy(p->GetScalarOpacity(i));

    this->SetScalarOpacityUnitDistance(i, p->GetScalarOpacityUnitDistance(i));

    this->GetGradientOpacity(i)->DeepCopy(p->GetGradientOpacity(i));

    this->SetDisableGradientOpacity(i, p->GetDisableGradientOpacity(i));

    this->SetShade(i, p->GetShade(i));
    this->SetAmbient(i, p->GetAmbient(i));
    this->SetDiffuse(i, p->GetDiffuse(i));
    this->SetSpecular(i, p->GetSpecular(i));
    this->SetSpecularPower(i, p->GetSpecularPower(i));
  }

  this->Modified();
}

//-----------------------------------------------------------------------------
void svtkVolumeProperty::UpdateMTimes()
{
  this->Modified();

  for (int i = 0; i < SVTK_MAX_VRCOMP; i++)
  {
    this->GrayTransferFunctionMTime[i].Modified();
    this->RGBTransferFunctionMTime[i].Modified();
    this->ScalarOpacityMTime[i].Modified();
    this->GradientOpacityMTime[i].Modified();
    this->TransferFunction2DMTime[i].Modified();
  }
  this->LabelColorMTime.Modified();
  this->LabelScalarOpacityMTime.Modified();
  this->LabelGradientOpacityMTime.Modified();
}

//-----------------------------------------------------------------------------
svtkMTimeType svtkVolumeProperty::GetMTime()
{
  svtkMTimeType mTime = this->svtkObject::GetMTime();
  svtkMTimeType time;

  for (int i = 0; i < SVTK_MAX_VRCOMP; i++)
  {
    // Color MTimes
    if (this->ColorChannels[i] == 1)
    {
      if (this->GrayTransferFunction[i])
      {
        // time that Gray transfer function pointer was set
        time = this->GrayTransferFunctionMTime[i];
        mTime = svtkMath::Max(mTime, time);

        // time that Gray transfer function was last modified
        time = this->GrayTransferFunction[i]->GetMTime();
        mTime = svtkMath::Max(mTime, time);
      }
    }
    else if (this->ColorChannels[i] == 3)
    {
      if (this->RGBTransferFunction[i])
      {
        // time that RGB transfer function pointer was set
        time = this->RGBTransferFunctionMTime[i];
        mTime = svtkMath::Max(mTime, time);

        // time that RGB transfer function was last modified
        time = this->RGBTransferFunction[i]->GetMTime();
        mTime = svtkMath::Max(mTime, time);
      }
    }

    // Opacity MTimes
    if (this->ScalarOpacity[i])
    {
      // time that Scalar opacity transfer function pointer was set
      time = this->ScalarOpacityMTime[i];
      mTime = svtkMath::Max(mTime, time);

      // time that Scalar opacity transfer function was last modified
      time = this->ScalarOpacity[i]->GetMTime();
      mTime = svtkMath::Max(mTime, time);
    }

    // 2D Transfer Function MTimes
    if (this->TransferFunction2D[i])
    {
      // time that the TransferFunction2D pointer was set
      time = this->TransferFunction2DMTime[i];
      mTime = svtkMath::Max(mTime, time);

      // time that the TransferFunction2D was last modified
      time = this->TransferFunction2D[i]->GetMTime();
      mTime = svtkMath::Max(mTime, time);
    }

    if (this->GradientOpacity[i])
    {
      // time that Gradient opacity transfer function pointer was set
      time = this->GradientOpacityMTime[i];
      mTime = svtkMath::Max(mTime, time);

      if (!this->DisableGradientOpacity[i])
      {
        // time that Gradient opacity transfer function was last modified
        time = this->GradientOpacity[i]->GetMTime();
        mTime = svtkMath::Max(mTime, time);
      }
    }
  }

  time = this->IsoSurfaceValues->GetMTime();
  mTime = svtkMath::Max(mTime, time);

  time = this->LabelColorMTime;
  mTime = svtkMath::Max(mTime, time);

  time = this->LabelScalarOpacityMTime;
  mTime = svtkMath::Max(mTime, time);

  time = this->LabelGradientOpacityMTime;
  mTime = svtkMath::Max(mTime, time);

  return mTime;
}

//-----------------------------------------------------------------------------
int svtkVolumeProperty::GetColorChannels(int index)
{
  if (index < 0 || index > 3)
  {
    svtkErrorMacro("Bad index - must be between 0 and 3");
    return 0;
  }

  return this->ColorChannels[index];
}

//-----------------------------------------------------------------------------
// Set the color of a volume to a gray transfer function
void svtkVolumeProperty::SetColor(int index, svtkPiecewiseFunction* function)
{
  if (this->GrayTransferFunction[index] != function)
  {
    if (this->GrayTransferFunction[index] != nullptr)
    {
      this->GrayTransferFunction[index]->UnRegister(this);
    }
    this->GrayTransferFunction[index] = function;
    if (this->GrayTransferFunction[index] != nullptr)
    {
      this->GrayTransferFunction[index]->Register(this);
    }

    this->GrayTransferFunctionMTime[index].Modified();
    this->Modified();
    this->TransferFunctionMode = svtkVolumeProperty::TF_1D;
  }

  if (this->ColorChannels[index] != 1)
  {
    this->ColorChannels[index] = 1;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
// Get the currently set gray transfer function. Create one if none set.
svtkPiecewiseFunction* svtkVolumeProperty::GetGrayTransferFunction(int index)
{
  if (this->GrayTransferFunction[index] == nullptr)
  {
    this->GrayTransferFunction[index] = svtkPiecewiseFunction::New();
    this->GrayTransferFunction[index]->Register(this);
    this->GrayTransferFunction[index]->Delete();
    this->GrayTransferFunction[index]->AddPoint(0, 0.0);
    this->GrayTransferFunction[index]->AddPoint(1024, 1.0);
    if (this->ColorChannels[index] != 1)
    {
      this->ColorChannels[index] = 1;
    }
    this->Modified();
  }

  return this->GrayTransferFunction[index];
}

//-----------------------------------------------------------------------------
// Set the color of a volume to an RGB transfer function
void svtkVolumeProperty::SetColor(int index, svtkColorTransferFunction* function)
{
  if (this->RGBTransferFunction[index] != function)
  {
    if (this->RGBTransferFunction[index] != nullptr)
    {
      this->RGBTransferFunction[index]->UnRegister(this);
    }
    this->RGBTransferFunction[index] = function;
    if (this->RGBTransferFunction[index] != nullptr)
    {
      this->RGBTransferFunction[index]->Register(this);
    }
    this->RGBTransferFunctionMTime[index].Modified();
    this->Modified();
    this->TransferFunctionMode = svtkVolumeProperty::TF_1D;
  }

  if (this->ColorChannels[index] != 3)
  {
    this->ColorChannels[index] = 3;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
// Get the currently set RGB transfer function. Create one if none set.
svtkColorTransferFunction* svtkVolumeProperty::GetRGBTransferFunction(int index)
{
  if (this->RGBTransferFunction[index] == nullptr)
  {
    this->RGBTransferFunction[index] = svtkColorTransferFunction::New();
    this->RGBTransferFunction[index]->Register(this);
    this->RGBTransferFunction[index]->Delete();
    this->RGBTransferFunction[index]->AddRGBPoint(0, 0.0, 0.0, 0.0);
    this->RGBTransferFunction[index]->AddRGBPoint(1024, 1.0, 1.0, 1.0);
    if (this->ColorChannels[index] != 3)
    {
      this->ColorChannels[index] = 3;
    }
    this->Modified();
  }

  return this->RGBTransferFunction[index];
}

//-----------------------------------------------------------------------------
// Set the scalar opacity of a volume to a transfer function
void svtkVolumeProperty::SetScalarOpacity(int index, svtkPiecewiseFunction* function)
{
  if (this->ScalarOpacity[index] != function)
  {
    if (this->ScalarOpacity[index] != nullptr)
    {
      this->ScalarOpacity[index]->UnRegister(this);
    }
    this->ScalarOpacity[index] = function;
    if (this->ScalarOpacity[index] != nullptr)
    {
      this->ScalarOpacity[index]->Register(this);
    }

    this->ScalarOpacityMTime[index].Modified();
    this->Modified();
    this->TransferFunctionMode = svtkVolumeProperty::TF_1D;
  }
}

//-----------------------------------------------------------------------------
// Get the scalar opacity transfer function. Create one if none set.
svtkPiecewiseFunction* svtkVolumeProperty::GetScalarOpacity(int index)
{
  if (this->ScalarOpacity[index] == nullptr)
  {
    this->ScalarOpacity[index] = svtkPiecewiseFunction::New();
    this->ScalarOpacity[index]->Register(this);
    this->ScalarOpacity[index]->Delete();
    this->ScalarOpacity[index]->AddPoint(0, 1.0);
    this->ScalarOpacity[index]->AddPoint(1024, 1.0);
  }

  return this->ScalarOpacity[index];
}

//-----------------------------------------------------------------------------
void svtkVolumeProperty::SetScalarOpacityUnitDistance(int index, double distance)
{
  if (index < 0 || index > 3)
  {
    svtkErrorMacro("Bad index - must be between 0 and 3");
    return;
  }

  if (this->ScalarOpacityUnitDistance[index] != distance)
  {
    this->ScalarOpacityUnitDistance[index] = distance;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
double svtkVolumeProperty::GetScalarOpacityUnitDistance(int index)
{
  if (index < 0 || index > 3)
  {
    svtkErrorMacro("Bad index - must be between 0 and 3");
    return 0;
  }

  return this->ScalarOpacityUnitDistance[index];
}

//-----------------------------------------------------------------------------
// Set the gradient opacity transfer function
void svtkVolumeProperty::SetGradientOpacity(int index, svtkPiecewiseFunction* function)
{
  if (this->GradientOpacity[index] != function)
  {
    if (this->GradientOpacity[index] != nullptr)
    {
      this->GradientOpacity[index]->UnRegister(this);
    }
    this->GradientOpacity[index] = function;
    if (this->GradientOpacity[index] != nullptr)
    {
      this->GradientOpacity[index]->Register(this);
    }

    this->GradientOpacityMTime[index].Modified();
    this->Modified();
    this->TransferFunctionMode = svtkVolumeProperty::TF_1D;
  }
}

//-----------------------------------------------------------------------------
void svtkVolumeProperty::CreateDefaultGradientOpacity(int index)
{
  if (this->DefaultGradientOpacity[index] == nullptr)
  {
    this->DefaultGradientOpacity[index] = svtkPiecewiseFunction::New();
    this->DefaultGradientOpacity[index]->Register(this);
    this->DefaultGradientOpacity[index]->Delete();
  }

  this->DefaultGradientOpacity[index]->RemoveAllPoints();
  this->DefaultGradientOpacity[index]->AddPoint(0, 1.0);
  this->DefaultGradientOpacity[index]->AddPoint(255, 1.0);
}

//-----------------------------------------------------------------------------
svtkPiecewiseFunction* svtkVolumeProperty::GetGradientOpacity(int index)
{
  if (this->DisableGradientOpacity[index])
  {
    if (this->DefaultGradientOpacity[index] == nullptr)
    {
      this->CreateDefaultGradientOpacity(index);
    }
    return this->DefaultGradientOpacity[index];
  }

  return this->GetStoredGradientOpacity(index);
}

//-----------------------------------------------------------------------------
void svtkVolumeProperty::SetTransferFunction2D(int index, svtkImageData* function)
{
  if (this->TransferFunction2D[index] != function)
  {
    svtkDataArray* dataArr = function->GetPointData()->GetScalars();
    const int* dims = function->GetDimensions();
    if (!dataArr || dataArr->GetNumberOfComponents() != 4 || dataArr->GetDataType() != SVTK_FLOAT ||
      dims[0] == 0)
    {
      if (dataArr)
      {
        const int type = dataArr->GetDataType();
        const int comp = dataArr->GetNumberOfComponents();
        svtkErrorMacro(<< "Invalid type (" << type << ") or number of components (" << comp
                      << ") or dimensions (" << dims[0] << ", " << dims[1]
                      << ")."
                         " Expected SVTK_FLOAT, 4 Components, dimensions > 0!");
        return;
      }

      svtkErrorMacro(<< "Invalid array!");
      return;
    }

    if (this->TransferFunction2D[index] != nullptr)
    {
      this->TransferFunction2D[index]->UnRegister(this);
    }

    this->TransferFunction2D[index] = function;
    if (this->TransferFunction2D[index] != nullptr)
    {
      this->TransferFunction2D[index]->Register(this);
    }

    this->TransferFunction2DMTime[index].Modified();
    this->Modified();
    this->TransferFunctionMode = svtkVolumeProperty::TF_2D;
  }
}

//-----------------------------------------------------------------------------
svtkImageData* svtkVolumeProperty::GetTransferFunction2D(int index)
{
  return this->TransferFunction2D[index];
}

//-----------------------------------------------------------------------------
// Get the gradient opacity transfer function. Create one if none set.
svtkPiecewiseFunction* svtkVolumeProperty::GetStoredGradientOpacity(int index)
{
  if (this->GradientOpacity[index] == nullptr)
  {
    this->GradientOpacity[index] = svtkPiecewiseFunction::New();
    this->GradientOpacity[index]->Register(this);
    this->GradientOpacity[index]->Delete();
    this->GradientOpacity[index]->AddPoint(0, 1.0);
    this->GradientOpacity[index]->AddPoint(255, 1.0);
  }

  return this->GradientOpacity[index];
}

//-----------------------------------------------------------------------------
void svtkVolumeProperty::SetDisableGradientOpacity(int index, int value)
{
  if (this->DisableGradientOpacity[index] == value)
  {
    return;
  }

  this->DisableGradientOpacity[index] = value;

  // Make sure the default function is up-to-date (since the user
  // could have modified the default function)

  if (value)
  {
    this->CreateDefaultGradientOpacity(index);
  }

  // Since this Ivar basically "sets" the gradient opacity function to be
  // either a default one or the user-specified one, update the MTime
  // accordingly

  this->GradientOpacityMTime[index].Modified();

  this->Modified();
}

//-----------------------------------------------------------------------------
int svtkVolumeProperty::GetDisableGradientOpacity(int index)
{
  return this->DisableGradientOpacity[index];
}

void svtkVolumeProperty::SetComponentWeight(int index, double value)
{
  if (index < 0 || index >= SVTK_MAX_VRCOMP)
  {
    svtkErrorMacro("Invalid index");
    return;
  }

  double val = value < 0.0 ? 0.0 : (value > 1.0 ? 1.0 : value);
  if (this->ComponentWeight[index] != val)
  {
    this->ComponentWeight[index] = val;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
double svtkVolumeProperty::GetComponentWeight(int index)
{
  if (index < 0 || index >= SVTK_MAX_VRCOMP)
  {
    svtkErrorMacro("Invalid index");
    return 0.0;
  }

  return this->ComponentWeight[index];
}

//-----------------------------------------------------------------------------
void svtkVolumeProperty::SetShade(int index, int value)
{
  if (value != 0 && value != 1)
  {
    svtkErrorMacro("SetShade accepts values 0 or 1");
    return;
  }

  if (this->Shade[index] != value)
  {
    this->Shade[index] = value;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void svtkVolumeProperty::ShadeOn(int index)
{
  this->SetShade(index, 1);
}

//-----------------------------------------------------------------------------
void svtkVolumeProperty::ShadeOff(int index)
{
  this->SetShade(index, 0);
}

//-----------------------------------------------------------------------------
int svtkVolumeProperty::GetShade(int index)
{
  return this->Shade[index];
}

//-----------------------------------------------------------------------------
void svtkVolumeProperty::SetAmbient(int index, double value)
{
  if (this->Ambient[index] != value)
  {
    this->Ambient[index] = value;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
double svtkVolumeProperty::GetAmbient(int index)
{
  return this->Ambient[index];
}

//-----------------------------------------------------------------------------
void svtkVolumeProperty::SetDiffuse(int index, double value)
{
  if (this->Diffuse[index] != value)
  {
    this->Diffuse[index] = value;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
double svtkVolumeProperty::GetDiffuse(int index)
{
  return this->Diffuse[index];
}

//-----------------------------------------------------------------------------
void svtkVolumeProperty::SetSpecular(int index, double value)
{
  if (this->Specular[index] != value)
  {
    this->Specular[index] = value;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
double svtkVolumeProperty::GetSpecular(int index)
{
  return this->Specular[index];
}

//-----------------------------------------------------------------------------
void svtkVolumeProperty::SetSpecularPower(int index, double value)
{
  if (this->SpecularPower[index] != value)
  {
    this->SpecularPower[index] = value;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
double svtkVolumeProperty::GetSpecularPower(int index)
{
  return this->SpecularPower[index];
}

//-----------------------------------------------------------------------------
svtkTimeStamp svtkVolumeProperty::GetScalarOpacityMTime(int index)
{
  return this->ScalarOpacityMTime[index];
}

//-----------------------------------------------------------------------------
svtkTimeStamp svtkVolumeProperty::GetGradientOpacityMTime(int index)
{
  return this->GradientOpacityMTime[index];
}

//-----------------------------------------------------------------------------
svtkTimeStamp svtkVolumeProperty::GetRGBTransferFunctionMTime(int index)
{
  return this->RGBTransferFunctionMTime[index];
}

//-----------------------------------------------------------------------------
svtkTimeStamp svtkVolumeProperty::GetTransferFunction2DMTime(int index)
{
  return this->TransferFunction2DMTime[index];
}

//-----------------------------------------------------------------------------
svtkTimeStamp svtkVolumeProperty::GetGrayTransferFunctionMTime(int index)
{
  return this->GrayTransferFunctionMTime[index];
}

//-----------------------------------------------------------------------------
svtkContourValues* svtkVolumeProperty::GetIsoSurfaceValues()
{
  return this->IsoSurfaceValues;
}

//-----------------------------------------------------------------------------
void svtkVolumeProperty::SetLabelColor(int label, svtkColorTransferFunction* color)
{
  if (label == 0)
  {
    svtkWarningMacro(<< "Ignoring attempt to set label map for label \"0\"");
    return;
  }
  if (this->LabelColor.count(label))
  {
    if (this->LabelColor[label] == color)
    {
      return;
    }
    if (this->LabelColor[label] != nullptr)
    {
      this->LabelColor[label]->UnRegister(this);
    }
  }
  this->LabelColor[label] = color;
  if (this->LabelColor[label] != nullptr)
  {
    this->LabelColor[label]->Register(this);
    this->LabelMapLabels.insert(label);
  }
  this->LabelColorMTime.Modified();
  this->Modified();
}

//-----------------------------------------------------------------------------
svtkColorTransferFunction* svtkVolumeProperty::GetLabelColor(int label)
{
  if (this->LabelColor.count(label) == 0)
  {
    return nullptr;
  }
  return this->LabelColor[label];
}

//-----------------------------------------------------------------------------
void svtkVolumeProperty::SetLabelScalarOpacity(int label, svtkPiecewiseFunction* function)
{
  if (label == 0)
  {
    svtkWarningMacro(<< "Ignoring attempt to set label map for label \"0\"");
    return;
  }
  if (this->LabelScalarOpacity.count(label))
  {
    if (this->LabelScalarOpacity[label] == function)
    {
      return;
    }
    if (this->LabelScalarOpacity[label] != nullptr)
    {
      this->LabelScalarOpacity[label]->UnRegister(this);
    }
  }
  this->LabelScalarOpacity[label] = function;
  if (this->LabelScalarOpacity[label] != nullptr)
  {
    this->LabelScalarOpacity[label]->Register(this);
    this->LabelMapLabels.insert(label);
  }
  this->LabelScalarOpacityMTime.Modified();
  this->Modified();
}

//-----------------------------------------------------------------------------
svtkPiecewiseFunction* svtkVolumeProperty::GetLabelScalarOpacity(int label)
{
  if (this->LabelScalarOpacity.count(label) == 0)
  {
    return nullptr;
  }
  return this->LabelScalarOpacity[label];
}

//-----------------------------------------------------------------------------
void svtkVolumeProperty::SetLabelGradientOpacity(int label, svtkPiecewiseFunction* function)
{
  if (label == 0)
  {
    svtkWarningMacro(<< "Ignoring attempt to set label map for label \"0\"");
    return;
  }
  if (this->LabelGradientOpacity.count(label))
  {
    if (this->LabelGradientOpacity[label] == function)
    {
      return;
    }
    if (this->LabelGradientOpacity[label] != nullptr)
    {
      this->LabelGradientOpacity[label]->UnRegister(this);
    }
  }
  this->LabelGradientOpacity[label] = function;
  if (this->LabelGradientOpacity[label] != nullptr)
  {
    this->LabelGradientOpacity[label]->Register(this);
    this->LabelMapLabels.insert(label);
  }
  this->LabelGradientOpacityMTime.Modified();
  this->Modified();
}

//-----------------------------------------------------------------------------
svtkPiecewiseFunction* svtkVolumeProperty::GetLabelGradientOpacity(int label)
{
  if (this->LabelGradientOpacity.count(label) == 0)
  {
    return nullptr;
  }
  return this->LabelGradientOpacity[label];
}

//-----------------------------------------------------------------------------
std::size_t svtkVolumeProperty::GetNumberOfLabels()
{
  return this->GetLabelMapLabels().size();
}

//-----------------------------------------------------------------------------
std::set<int> svtkVolumeProperty::GetLabelMapLabels()
{
  // Erase labels that were added re-assigned to null pointers
  for (auto it = this->LabelMapLabels.begin(); it != this->LabelMapLabels.end();)
  {
    if (!this->GetLabelColor(*it) && !this->GetLabelScalarOpacity(*it) &&
      !this->GetLabelGradientOpacity(*it))
    {
      it = this->LabelMapLabels.erase(it);
    }
    else
    {
      ++it;
    }
  }
  return this->LabelMapLabels;
}

//-----------------------------------------------------------------------------
// Print the state of the volume property.
void svtkVolumeProperty::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Independent Components: " << (this->IndependentComponents ? "On\n" : "Off\n");

  os << indent << "Interpolation Type: " << this->GetInterpolationTypeAsString() << "\n";

  os << indent
     << "Use Clipped Voxel Intensity: " << (this->UseClippedVoxelIntensity ? "On\n" : "Off\n");
  os << indent << "Clipped Voxel Intensity: " << this->GetClippedVoxelIntensity() << "\n";

  for (int i = 0; i < SVTK_MAX_VRCOMP; i++)
  {
    os << indent << "Properties for material " << i << endl;

    os << indent << "Color Channels: " << this->ColorChannels[i] << "\n";

    if (this->ColorChannels[i] == 1)
    {
      os << indent << "Gray Color Transfer Function: " << this->GrayTransferFunction[i] << "\n";
    }
    else if (this->ColorChannels[i] == 3)
    {
      os << indent << "RGB Color Transfer Function: " << this->RGBTransferFunction[i] << "\n";
    }

    os << indent << "Scalar Opacity Transfer Function: " << this->ScalarOpacity[i] << "\n";

    os << indent << "Gradient Opacity Transfer Function: " << this->GradientOpacity[i] << "\n";

    os << indent << "DisableGradientOpacity: " << (this->DisableGradientOpacity[i] ? "On" : "Off")
       << "\n";

    os << indent << "2D Transfer Function: " << this->TransferFunction2D[i] << "\n";

    os << indent << "ComponentWeight: " << this->ComponentWeight[i] << "\n";

    os << indent << "Shade: " << this->Shade[i] << "\n";
    os << indent << indent << "Ambient: " << this->Ambient[i] << "\n";
    os << indent << indent << "Diffuse: " << this->Diffuse[i] << "\n";
    os << indent << indent << "Specular: " << this->Specular[i] << "\n";
    os << indent << indent << "SpecularPower: " << this->SpecularPower[i] << "\n";
  }

  if (!this->LabelColor.empty())
  {
    os << indent << "Label Color Transfer Functions:"
       << "\n";
    for (auto it = this->LabelColor.begin(); it != LabelColor.end(); ++it)
    {
      os << indent.GetNextIndent() << "Label: " << it->first << " " << it->second;
    }
  }
  if (!this->LabelScalarOpacity.empty())
  {
    os << indent << "Label Scalar Opacity Transfer Functions:"
       << "\n";
    for (auto it = this->LabelScalarOpacity.begin(); it != LabelScalarOpacity.end(); ++it)
    {
      os << indent.GetNextIndent() << "Label: " << it->first << " " << it->second;
    }
  }
  if (!this->LabelGradientOpacity.empty())
  {
    os << indent << "Label Gradient Opacity Transfer Functions:"
       << "\n";
    for (auto it = this->LabelGradientOpacity.begin(); it != LabelGradientOpacity.end(); ++it)
    {
      os << indent.GetNextIndent() << "Label: " << it->first << " " << it->second;
    }
  }

  // These variables should not be printed to the user:
  // this->GradientOpacityMTime
  // this->GrayTransferFunctionMTime
  // this->RGBTransferFunctionMTime
  // this->ScalarOpacityMTime
  // this->LabelColorMTime
  // this->LabelScalarOpacityMTime
  // this->LabelGradientOpacityMTime
}
