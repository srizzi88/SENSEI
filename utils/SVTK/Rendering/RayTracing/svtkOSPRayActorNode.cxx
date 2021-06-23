/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayActorNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOSPRayActorNode.h"

#include "svtkActor.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationDoubleKey.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationObjectBaseKey.h"
#include "svtkInformationStringKey.h"
#include "svtkMapper.h"
#include "svtkObjectFactory.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPolyData.h"
#include "svtkProperty.h"
#include "svtkTexture.h"
#include "svtkViewNodeCollection.h"

#include "RTWrapper/RTWrapper.h"

svtkInformationKeyMacro(svtkOSPRayActorNode, LUMINOSITY, Double);
svtkInformationKeyMacro(svtkOSPRayActorNode, ENABLE_SCALING, Integer);
svtkInformationKeyMacro(svtkOSPRayActorNode, SCALE_ARRAY_NAME, String);
svtkInformationKeyMacro(svtkOSPRayActorNode, SCALE_FUNCTION, ObjectBase);

//============================================================================
svtkStandardNewMacro(svtkOSPRayActorNode);

//----------------------------------------------------------------------------
svtkOSPRayActorNode::svtkOSPRayActorNode()
{
  this->LastMapper = nullptr;
}

//----------------------------------------------------------------------------
svtkOSPRayActorNode::~svtkOSPRayActorNode() {}

//----------------------------------------------------------------------------
void svtkOSPRayActorNode::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkOSPRayActorNode::SetEnableScaling(int value, svtkActor* actor)
{
  if (!actor)
  {
    return;
  }
  svtkMapper* mapper = actor->GetMapper();
  if (mapper)
  {
    svtkInformation* info = mapper->GetInformation();
    info->Set(svtkOSPRayActorNode::ENABLE_SCALING(), value);
  }
}

//----------------------------------------------------------------------------
int svtkOSPRayActorNode::GetEnableScaling(svtkActor* actor)
{
  if (!actor)
  {
    return 0;
  }
  svtkMapper* mapper = actor->GetMapper();
  if (mapper)
  {
    svtkInformation* info = mapper->GetInformation();
    if (info && info->Has(svtkOSPRayActorNode::ENABLE_SCALING()))
    {
      return (info->Get(svtkOSPRayActorNode::ENABLE_SCALING()));
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkOSPRayActorNode::SetScaleArrayName(const char* arrayName, svtkActor* actor)
{
  if (!actor)
  {
    return;
  }
  svtkMapper* mapper = actor->GetMapper();
  if (mapper)
  {
    svtkInformation* mapperInfo = mapper->GetInformation();
    mapperInfo->Set(svtkOSPRayActorNode::SCALE_ARRAY_NAME(), arrayName);
  }
}

//----------------------------------------------------------------------------
void svtkOSPRayActorNode::SetScaleFunction(svtkPiecewiseFunction* scaleFunction, svtkActor* actor)
{
  if (!actor)
  {
    return;
  }
  svtkMapper* mapper = actor->GetMapper();
  if (mapper)
  {
    svtkInformation* mapperInfo = mapper->GetInformation();
    mapperInfo->Set(svtkOSPRayActorNode::SCALE_FUNCTION(), scaleFunction);
  }
}

//----------------------------------------------------------------------------
void svtkOSPRayActorNode::SetLuminosity(double value, svtkProperty* property)
{
  if (!property)
  {
    return;
  }
  svtkInformation* info = property->GetInformation();
  info->Set(svtkOSPRayActorNode::LUMINOSITY(), value);
}

//----------------------------------------------------------------------------
double svtkOSPRayActorNode::GetLuminosity(svtkProperty* property)
{
  if (!property)
  {
    return 0.0;
  }
  svtkInformation* info = property->GetInformation();
  if (info && info->Has(svtkOSPRayActorNode::LUMINOSITY()))
  {
    double retval = info->Get(svtkOSPRayActorNode::LUMINOSITY());
    return retval;
  }
  return 0.0;
}

//----------------------------------------------------------------------------
svtkMTimeType svtkOSPRayActorNode::GetMTime()
{
  svtkMTimeType mtime = this->Superclass::GetMTime();
  svtkActor* act = (svtkActor*)this->GetRenderable();
  if (act->GetMTime() > mtime)
  {
    mtime = act->GetMTime();
  }
  if (svtkProperty* prop = act->GetProperty())
  {
    if (prop->GetMTime() > mtime)
    {
      mtime = prop->GetMTime();
    }
    if (prop->GetInformation()->GetMTime() > mtime)
    {
      mtime = prop->GetInformation()->GetMTime();
    }
  }
  svtkDataObject* dobj = nullptr;
  svtkPolyData* poly = nullptr;
  svtkMapper* mapper = act->GetMapper();
  svtkTexture* texture = act->GetTexture();
  if (mapper)
  {
    // if (act->GetRedrawMTime() > mtime)
    //  {
    //  mtime = act->GetRedrawMTime();
    // }
    if (mapper->GetMTime() > mtime)
    {
      mtime = mapper->GetMTime();
    }
    if (mapper->GetInformation()->GetMTime() > mtime)
    {
      mtime = mapper->GetInformation()->GetMTime();
    }
    if (mapper != this->LastMapper)
    {
      this->MapperChangedTime.Modified();
      mtime = this->MapperChangedTime;
      this->LastMapper = mapper;
    }
    svtkPiecewiseFunction* pwf = svtkPiecewiseFunction::SafeDownCast(
      mapper->GetInformation()->Get(svtkOSPRayActorNode::SCALE_FUNCTION()));
    if (pwf)
    {
      if (pwf->GetMTime() > mtime)
      {
        mtime = pwf->GetMTime();
      }
    }
    if (mapper->GetNumberOfInputPorts() > 0)
    {
      dobj = mapper->GetInputDataObject(0, 0);
      poly = svtkPolyData::SafeDownCast(dobj);
    }
  }
  if (poly)
  {
    if (poly->GetMTime() > mtime)
    {
      mtime = poly->GetMTime();
    }
  }
  else if (dobj)
  {
    svtkCompositeDataSet* comp = svtkCompositeDataSet::SafeDownCast(dobj);
    if (comp)
    {
      svtkCompositeDataIterator* dit = comp->NewIterator();
      dit->SkipEmptyNodesOn();
      while (!dit->IsDoneWithTraversal())
      {
        poly = svtkPolyData::SafeDownCast(comp->GetDataSet(dit));
        if (poly)
        {
          if (poly->GetMTime() > mtime)
          {
            mtime = poly->GetMTime();
          }
        }
        dit->GoToNextItem();
      }
      dit->Delete();
    }
  }
  if (texture)
  {
    if (texture->GetMTime() > mtime)
    {
      mtime = texture->GetMTime();
    }
    if (texture->GetInput() && texture->GetInput()->GetMTime() > mtime)
    {
      mtime = texture->GetInput()->GetMTime();
    }
  }
  return mtime;
}
