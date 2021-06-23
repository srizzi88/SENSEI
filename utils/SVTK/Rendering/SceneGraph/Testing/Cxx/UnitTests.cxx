/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Mace.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkActorNode.h"
#include "svtkCamera.h"
#include "svtkCameraNode.h"
#include "svtkLight.h"
#include "svtkLightNode.h"
#include "svtkMapperNode.h"
#include "svtkObjectFactory.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkRendererNode.h"
#include "svtkSphereSource.h"
#include "svtkViewNodeCollection.h"
#include "svtkViewNodeFactory.h"
#include "svtkWindowNode.h"

#include <string>
namespace
{
std::string resultS = "";
}

//-----------------------------------------------------------------------
// ViewNode subclasses specialized for this test
class svtkMyActorNode : public svtkActorNode
{
public:
  static svtkMyActorNode* New();
  svtkTypeMacro(svtkMyActorNode, svtkActorNode);
  virtual void Render(bool prepass) override
  {
    if (prepass)
    {
      cerr << "Render " << this << " " << this->GetClassName() << endl;
      resultS += "Render ";
      resultS += this->GetClassName();
      resultS += "\n";
    }
  }
  svtkMyActorNode() {}
  ~svtkMyActorNode() override {}
};
svtkStandardNewMacro(svtkMyActorNode);

class svtkMyCameraNode : public svtkCameraNode
{
public:
  static svtkMyCameraNode* New();
  svtkTypeMacro(svtkMyCameraNode, svtkCameraNode);
  virtual void Render(bool prepass) override
  {
    if (prepass)
    {
      cerr << "Render " << this << " " << this->GetClassName() << endl;
      resultS += "Render ";
      resultS += this->GetClassName();
      resultS += "\n";
    }
  }
  svtkMyCameraNode() {}
  ~svtkMyCameraNode() override {}
};
svtkStandardNewMacro(svtkMyCameraNode);

class svtkMyLightNode : public svtkLightNode
{
public:
  static svtkMyLightNode* New();
  svtkTypeMacro(svtkMyLightNode, svtkLightNode);
  virtual void Render(bool prepass) override
  {
    if (prepass)
    {
      cerr << "Render " << this << " " << this->GetClassName() << endl;
      resultS += "Render ";
      resultS += this->GetClassName();
      resultS += "\n";
    }
  }
  svtkMyLightNode() {}
  ~svtkMyLightNode() override {}
};
svtkStandardNewMacro(svtkMyLightNode);

class svtkMyMapperNode : public svtkMapperNode
{
public:
  static svtkMyMapperNode* New();
  svtkTypeMacro(svtkMyMapperNode, svtkMapperNode);
  virtual void Render(bool prepass) override
  {
    if (prepass)
    {
      cerr << "Render " << this << " " << this->GetClassName() << endl;
      resultS += "Render ";
      resultS += this->GetClassName();
      resultS += "\n";
    }
  }
  svtkMyMapperNode(){};
  ~svtkMyMapperNode() override{};
};
svtkStandardNewMacro(svtkMyMapperNode);

class svtkMyRendererNode : public svtkRendererNode
{
public:
  static svtkMyRendererNode* New();
  svtkTypeMacro(svtkMyRendererNode, svtkRendererNode);
  virtual void Render(bool prepass) override
  {
    if (prepass)
    {
      cerr << "Render " << this << " " << this->GetClassName() << endl;
      resultS += "Render ";
      resultS += this->GetClassName();
      resultS += "\n";
    }
  }
  svtkMyRendererNode() {}
  ~svtkMyRendererNode() override {}
};
svtkStandardNewMacro(svtkMyRendererNode);

class svtkMyWindowNode : public svtkWindowNode
{
public:
  static svtkMyWindowNode* New();
  svtkTypeMacro(svtkMyWindowNode, svtkWindowNode);
  virtual void Render(bool prepass) override
  {
    if (prepass)
    {
      cerr << "Render " << this << " " << this->GetClassName() << endl;
      resultS += "Render ";
      resultS += this->GetClassName();
      resultS += "\n";
    }
  }
  svtkMyWindowNode() {}
  ~svtkMyWindowNode() override {}
};
svtkStandardNewMacro(svtkMyWindowNode);

//------------------------------------------------------------------------------

// builders that produce the specialized ViewNodes
svtkViewNode* act_maker()
{
  svtkMyActorNode* vn = svtkMyActorNode::New();
  cerr << "make actor node " << vn << endl;
  resultS += "make actor\n";
  return vn;
}

svtkViewNode* cam_maker()
{
  svtkMyCameraNode* vn = svtkMyCameraNode::New();
  cerr << "make camera node " << vn << endl;
  resultS += "make camera\n";
  return vn;
}

svtkViewNode* light_maker()
{
  svtkMyLightNode* vn = svtkMyLightNode::New();
  cerr << "make light node " << vn << endl;
  resultS += "make light\n";
  return vn;
}

svtkViewNode* mapper_maker()
{
  svtkMyMapperNode* vn = svtkMyMapperNode::New();
  cerr << "make mapper node " << vn << endl;
  resultS += "make mapper\n";
  return vn;
}

svtkViewNode* ren_maker()
{
  svtkMyRendererNode* vn = svtkMyRendererNode::New();
  cerr << "make renderer node " << vn << endl;
  resultS += "make renderer\n";
  return vn;
}

svtkViewNode* win_maker()
{
  svtkMyWindowNode* vn = svtkMyWindowNode::New();
  cerr << "make window node " << vn << endl;
  resultS += "make window\n";
  return vn;
}

// exercises the scene graph related classes
int UnitTests(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkWindowNode* wvn = svtkWindowNode::New();
  cerr << "made " << wvn << endl;

  svtkViewNodeCollection* vnc = svtkViewNodeCollection::New();
  cerr << "made " << vnc << endl;
  vnc->AddItem(wvn);
  vnc->PrintSelf(cerr, svtkIndent(0));
  wvn->Delete();
  vnc->Delete();

  svtkViewNode* vn = nullptr;
  svtkViewNodeFactory* vnf = svtkViewNodeFactory::New();
  cerr << "CREATE pre override" << endl;
  vnc = nullptr;
  vn = vnf->CreateNode(vnc);
  if (vn)
  {
    cerr << "Shouldn't have made anything" << endl;
    return 1;
  }
  cerr << "factory made nothing as it should have" << endl;

  svtkRenderWindow* rwin = svtkRenderWindow::New();
  vnf->RegisterOverride(rwin->GetClassName(), win_maker);
  cerr << "CREATE node for renderwindow" << endl;
  vn = vnf->CreateNode(rwin);

  cerr << "factory makes" << endl;
  cerr << vn << endl;
  cerr << "BUILD [" << endl;
  vn->Traverse(svtkViewNode::build);
  cerr << "]" << endl;

  cerr << "add renderer" << endl;
  svtkRenderer* ren = svtkRenderer::New();
  vnf->RegisterOverride(ren->GetClassName(), ren_maker);
  rwin->AddRenderer(ren);

  svtkLight* light = svtkLight::New();
  vnf->RegisterOverride(light->GetClassName(), light_maker);
  ren->AddLight(light);
  light->Delete();

  vnf->RegisterOverride("svtkMapper", mapper_maker);

  svtkCamera* cam = svtkCamera::New();
  vnf->RegisterOverride(cam->GetClassName(), cam_maker);
  cam->Delete();

  svtkActor* actor = svtkActor::New();
  vnf->RegisterOverride(actor->GetClassName(), act_maker);
  ren->AddActor(actor);
  actor->Delete();

  svtkSphereSource* sphere = svtkSphereSource::New();
  svtkPolyDataMapper* pmap = svtkPolyDataMapper::New();
  pmap->SetInputConnection(sphere->GetOutputPort());
  actor->SetMapper(pmap);
  rwin->Render();
  sphere->Delete();
  pmap->Delete();

  cerr << "BUILD [" << endl;
  vn->Traverse(svtkViewNode::build);
  cerr << "]" << endl;
  cerr << "SYNCHRONIZE [" << endl;
  vn->Traverse(svtkViewNode::synchronize);
  cerr << "]" << endl;
  cerr << "RENDER [" << endl;
  vn->Traverse(svtkViewNode::render);
  cerr << "]" << endl;

  vn->Delete();
  ren->Delete();
  rwin->Delete();

  vnf->Delete();

  cerr << "Results is [" << endl;
  cerr << resultS << "]" << endl;
  std::string ok_res = "make window\nmake renderer\nmake light\nmake actor\nmake camera\nmake "
                       "mapper\nRender svtkMyWindowNode\nRender svtkMyRendererNode\nRender "
                       "svtkMyLightNode\nRender svtkMyActorNode\nRender svtkMyMapperNode\nRender "
                       "svtkMyCameraNode\n";
  if (resultS != ok_res)
  {
    cerr << "Which does not match [" << endl;
    cerr << ok_res << "]" << endl;
    return 1;
  }
  return 0;
}
