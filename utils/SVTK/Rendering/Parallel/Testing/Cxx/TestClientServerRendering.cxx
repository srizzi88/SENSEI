/*=========================================================================

  Program:   ParaView
  Module:    TestClientServerRendering.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Tests client-server rendering using the svtkClientServerCompositePass.

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCameraPass.h"
#include "svtkClearZPass.h"
#include "svtkClientServerCompositePass.h"
#include "svtkCompositeRenderManager.h"
#include "svtkDataSetReader.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkDepthPeelingPass.h"
#include "svtkDistributedDataFilter.h"
#include "svtkImageRenderManager.h"
#include "svtkLightsPass.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOpaquePass.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOverlayPass.h"
#include "svtkPKdTree.h"
#include "svtkPieceScalars.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderPassCollection.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkSequencePass.h"
#include "svtkSmartPointer.h"
#include "svtkSocketController.h"
#include "svtkSphereSource.h"
#include "svtkSynchronizedRenderWindows.h"
#include "svtkSynchronizedRenderers.h"
#include "svtkTestUtilities.h"
#include "svtkTesting.h"
#include "svtkTranslucentPass.h"
#include "svtkUnstructuredGrid.h"
#include "svtkVolumetricPass.h"

#include <svtksys/CommandLineArguments.hxx>

namespace
{

class MyProcess : public svtkObject
{
public:
  static MyProcess* New();
  svtkSetMacro(ImageReductionFactor, int);
  // Returns true on success.
  bool Execute(int argc, char** argv);

  // Get/Set the controller.
  svtkSetObjectMacro(Controller, svtkMultiProcessController);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);

  bool IsServer;

private:
  // Creates the visualization pipeline and adds it to the renderer.
  void CreatePipeline(svtkRenderer* renderer);

  // Setups render passes.
  void SetupRenderPasses(svtkRenderer* renderer);

protected:
  MyProcess();
  ~MyProcess();
  int ImageReductionFactor;
  svtkMultiProcessController* Controller;
};

svtkStandardNewMacro(MyProcess);

//-----------------------------------------------------------------------------
MyProcess::MyProcess()
{
  this->ImageReductionFactor = 1;
  this->Controller = nullptr;
}

//-----------------------------------------------------------------------------
MyProcess::~MyProcess()
{
  this->SetController(nullptr);
}

//-----------------------------------------------------------------------------
void MyProcess::CreatePipeline(svtkRenderer* renderer)
{
  double bounds[] = { -0.5, .5, -0.5, .5, -0.5, 0.5 };
  renderer->ResetCamera(bounds);
  if (!this->IsServer)
  {
    return;
  }

  svtkSphereSource* sphere = svtkSphereSource::New();
  // sphere->SetPhiResolution(100);
  // sphere->SetThetaResolution(100);

  svtkDataSetSurfaceFilter* surface = svtkDataSetSurfaceFilter::New();
  surface->SetInputConnection(sphere->GetOutputPort());

  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetInputConnection(surface->GetOutputPort());

  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  actor->Delete();
  mapper->Delete();
  surface->Delete();
  sphere->Delete();
}

//-----------------------------------------------------------------------------
void MyProcess::SetupRenderPasses(svtkRenderer* renderer)
{
  // the rendering passes
  svtkCameraPass* cameraP = svtkCameraPass::New();
  svtkSequencePass* seq = svtkSequencePass::New();
  svtkOpaquePass* opaque = svtkOpaquePass::New();

  svtkTranslucentPass* translucent = svtkTranslucentPass::New();

  svtkVolumetricPass* volume = svtkVolumetricPass::New();
  svtkOverlayPass* overlay = svtkOverlayPass::New();
  svtkLightsPass* lights = svtkLightsPass::New();

  svtkClearZPass* clearZ = svtkClearZPass::New();
  clearZ->SetDepth(0.9);

  svtkRenderPassCollection* passes = svtkRenderPassCollection::New();
  passes->AddItem(lights);
  passes->AddItem(opaque);
  //  passes->AddItem(clearZ);
  passes->AddItem(translucent);
  passes->AddItem(volume);
  passes->AddItem(overlay);
  seq->SetPasses(passes);

  svtkClientServerCompositePass* csPass = svtkClientServerCompositePass::New();
  csPass->SetRenderPass(seq);
  csPass->SetProcessIsServer(this->IsServer);
  csPass->ServerSideRenderingOn();
  csPass->SetController(this->Controller);

  svtkOpenGLRenderer* glrenderer = svtkOpenGLRenderer::SafeDownCast(renderer);
  cameraP->SetDelegatePass(csPass);
  glrenderer->SetPass(cameraP);

  // setting viewport doesn't work in tile-display mode correctly yet.
  // renderer->SetViewport(0, 0, 0.75, 1);

  opaque->Delete();
  translucent->Delete();
  volume->Delete();
  overlay->Delete();
  seq->Delete();
  passes->Delete();
  cameraP->Delete();
  lights->Delete();
  clearZ->Delete();
  csPass->Delete();
}

//-----------------------------------------------------------------------------
bool MyProcess::Execute(int argc, char** argv)
{
  svtkRenderWindow* renWin = svtkRenderWindow::New();

  renWin->SetWindowName(this->IsServer ? "Server Window" : "Client Window");

  // enable alpha bit-planes.
  renWin->AlphaBitPlanesOn();

  // use double bufferring.
  renWin->DoubleBufferOn();

  // don't waste time swapping buffers unless needed.
  renWin->SwapBuffersOff();

  svtkRenderer* renderer = svtkRenderer::New();
  renWin->AddRenderer(renderer);

  svtkSynchronizedRenderWindows* syncWindows = svtkSynchronizedRenderWindows::New();
  syncWindows->SetRenderWindow(renWin);
  syncWindows->SetParallelController(this->Controller);
  syncWindows->SetIdentifier(2);
  syncWindows->SetRootProcessId(this->IsServer ? 1 : 0);

  svtkSynchronizedRenderers* syncRenderers = svtkSynchronizedRenderers::New();
  syncRenderers->SetRenderer(renderer);
  syncRenderers->SetParallelController(this->Controller);
  syncRenderers->SetRootProcessId(this->IsServer ? 1 : 0);
  syncRenderers->SetImageReductionFactor(this->ImageReductionFactor);

  this->CreatePipeline(renderer);
  this->SetupRenderPasses(renderer);

  bool success = true;
  if (!this->IsServer)
  {
    // CLIENT
    svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);
    renWin->SwapBuffersOn();
    renWin->Render();

    // regression test is done on the client since the data is on the server.
    int result = svtkTesting::Test(argc, argv, renWin, 15);
    success = (result == svtkTesting::PASSED);
    if (result == svtkTesting::DO_INTERACTOR)
    {
      iren->Start();
    }
    iren->Delete();
    this->Controller->TriggerBreakRMIs();
  }
  else
  {
    // SERVER
    this->Controller->ProcessRMIs();
  }

  renderer->Delete();
  renWin->Delete();
  syncWindows->Delete();
  syncRenderers->Delete();
  return success;
}

}

//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  int image_reduction_factor = 1;
  int is_server = 0;
  int port = 11111;

  svtksys::CommandLineArguments args;
  args.Initialize(argc, argv);
  args.StoreUnusedArguments(true);
  args.AddArgument("--image-reduction-factor", svtksys::CommandLineArguments::SPACE_ARGUMENT,
    &image_reduction_factor, "Image reduction factor");
  args.AddArgument("-irf", svtksys::CommandLineArguments::SPACE_ARGUMENT, &image_reduction_factor,
    "Image reduction factor (shorthand)");
  args.AddArgument(
    "--server", svtksys::CommandLineArguments::NO_ARGUMENT, &is_server, "process is a server");
  args.AddArgument("--port", svtksys::CommandLineArguments::SPACE_ARGUMENT, &port,
    "Port number (default is 11111)");
  if (!args.Parse())
  {
    return 1;
  }

  svtkSmartPointer<svtkSocketController> contr = svtkSmartPointer<svtkSocketController>::New();
  contr->Initialize(&argc, &argv);
  if (is_server)
  {
    cout << "Waiting for client on " << port << endl;
    contr->WaitForConnection(port);
  }
  else
  {
    if (!contr->ConnectTo(const_cast<char*>("localhost"), port))
    {
      return 1;
    }
  }

  MyProcess* p = MyProcess::New();
  p->IsServer = is_server != 0;
  p->SetImageReductionFactor(image_reduction_factor);
  p->SetController(contr);
  bool success = p->Execute(argc, argv);
  p->Delete();
  contr->Finalize();
  contr = nullptr;
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
