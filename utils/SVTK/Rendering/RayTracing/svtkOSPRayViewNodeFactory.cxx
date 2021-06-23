/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayViewNodeFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOSPRayViewNodeFactory.h"
#include "svtkObjectFactory.h"

#include "svtkOSPRayAMRVolumeMapperNode.h"
#include "svtkOSPRayActorNode.h"
#include "svtkOSPRayCameraNode.h"
#include "svtkOSPRayCompositePolyDataMapper2Node.h"
#include "svtkOSPRayLightNode.h"
#include "svtkOSPRayMoleculeMapperNode.h"
#include "svtkOSPRayPolyDataMapperNode.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkOSPRayTetrahedraMapperNode.h"
#include "svtkOSPRayVolumeMapperNode.h"
#include "svtkOSPRayVolumeNode.h"

svtkViewNode* ren_maker()
{
  svtkOSPRayRendererNode* vn = svtkOSPRayRendererNode::New();
  return vn;
}

svtkViewNode* amrm_maker()
{
  svtkOSPRayAMRVolumeMapperNode* vn = svtkOSPRayAMRVolumeMapperNode::New();
  return vn;
}

svtkViewNode* act_maker()
{
  svtkOSPRayActorNode* vn = svtkOSPRayActorNode::New();
  return vn;
}

svtkViewNode* vol_maker()
{
  return svtkOSPRayVolumeNode::New();
}

svtkViewNode* cam_maker()
{
  svtkOSPRayCameraNode* vn = svtkOSPRayCameraNode::New();
  return vn;
}

svtkViewNode* light_maker()
{
  svtkOSPRayLightNode* vn = svtkOSPRayLightNode::New();
  return vn;
}

svtkViewNode* pd_maker()
{
  svtkOSPRayPolyDataMapperNode* vn = svtkOSPRayPolyDataMapperNode::New();
  return vn;
}

svtkViewNode* molecule_maker()
{
  svtkOSPRayMoleculeMapperNode* vn = svtkOSPRayMoleculeMapperNode::New();
  return vn;
}

svtkViewNode* vm_maker()
{
  svtkOSPRayVolumeMapperNode* vn = svtkOSPRayVolumeMapperNode::New();
  return vn;
}

svtkViewNode* cpd_maker()
{
  svtkOSPRayCompositePolyDataMapper2Node* vn = svtkOSPRayCompositePolyDataMapper2Node::New();
  return vn;
}

svtkViewNode* tetm_maker()
{
  svtkOSPRayTetrahedraMapperNode* vn = svtkOSPRayTetrahedraMapperNode::New();
  return vn;
}

//============================================================================
svtkStandardNewMacro(svtkOSPRayViewNodeFactory);

//----------------------------------------------------------------------------
svtkOSPRayViewNodeFactory::svtkOSPRayViewNodeFactory()
{
  // see svtkRenderWindow::GetRenderLibrary
  this->RegisterOverride("svtkOpenGLRenderer", ren_maker);
  this->RegisterOverride("svtkOpenGLActor", act_maker);
  this->RegisterOverride("svtkPVLODActor", act_maker);
  this->RegisterOverride("svtkPVLODVolume", vol_maker);
  this->RegisterOverride("svtkVolume", vol_maker);
  this->RegisterOverride("svtkOpenGLCamera", cam_maker);
  this->RegisterOverride("svtkPVCamera", cam_maker);
  this->RegisterOverride("svtkOpenGLLight", light_maker);
  this->RegisterOverride("svtkPVLight", light_maker);
  this->RegisterOverride("svtkPainterPolyDataMapper", pd_maker);
  this->RegisterOverride("svtkOpenGLPolyDataMapper", pd_maker);
  this->RegisterOverride("svtkSmartVolumeMapper", vm_maker);
  this->RegisterOverride("svtkOSPRayVolumeMapper", vm_maker);
  this->RegisterOverride("svtkOpenGLGPUVolumeRayCastMapper", vm_maker);
  this->RegisterOverride("svtkCompositePolyDataMapper2", cpd_maker);
  this->RegisterOverride("svtkOpenGLProjectedTetrahedraMapper", tetm_maker);
  this->RegisterOverride("svtkUnstructuredGridVolumeZSweepMapper", tetm_maker);
  this->RegisterOverride("svtkUnstructuredGridVolumeRayCastMapper", tetm_maker);
  this->RegisterOverride("svtkAMRVolumeMapper", amrm_maker);
  this->RegisterOverride("svtkMoleculeMapper", molecule_maker);
}

//----------------------------------------------------------------------------
svtkOSPRayViewNodeFactory::~svtkOSPRayViewNodeFactory() {}

//----------------------------------------------------------------------------
void svtkOSPRayViewNodeFactory::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
