/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayVolumeNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOSPRayVolumeNode.h"

#include "svtkAbstractVolumeMapper.h"
#include "svtkActor.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataArray.h"
#include "svtkInformation.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationObjectBaseKey.h"
#include "svtkInformationStringKey.h"
#include "svtkMapper.h"
#include "svtkObjectFactory.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPolyData.h"
#include "svtkViewNodeCollection.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

#include "RTWrapper/RTWrapper.h"

//============================================================================
svtkStandardNewMacro(svtkOSPRayVolumeNode);

//----------------------------------------------------------------------------
svtkOSPRayVolumeNode::svtkOSPRayVolumeNode() {}

//----------------------------------------------------------------------------
svtkOSPRayVolumeNode::~svtkOSPRayVolumeNode() {}

//----------------------------------------------------------------------------
void svtkOSPRayVolumeNode::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkMTimeType svtkOSPRayVolumeNode::GetMTime()
{
  svtkMTimeType mtime = this->Superclass::GetMTime();
  svtkVolume* vol = (svtkVolume*)this->GetRenderable();
  if (vol->GetMTime() > mtime)
  {
    mtime = vol->GetMTime();
  }
  if (vol->GetProperty())
  {
    mtime = std::max(mtime, vol->GetProperty()->GetMTime());
  }
  svtkAbstractVolumeMapper* mapper = vol->GetMapper();

  if (mapper)
  {
    svtkDataObject* dobj = mapper->GetDataSetInput();
    if (dobj)
    {
      mtime = std::max(mtime, dobj->GetMTime());
    }
    if (mapper->GetMTime() > mtime)
    {
      mtime = mapper->GetMTime();
    }
    if (mapper->GetInformation()->GetMTime() > mtime)
    {
      mtime = mapper->GetInformation()->GetMTime();
    }
  }
  return mtime;
}
