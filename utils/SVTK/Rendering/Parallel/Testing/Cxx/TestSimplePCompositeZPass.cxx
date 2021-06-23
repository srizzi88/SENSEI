/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPCompositeZPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// The scene consists of
// * 4 actors: a rectangle, a box, a cone and a sphere. The box, the cone and
// the sphere are above the rectangle.
// * 2 spotlights: one in the direction of the box, another one in the
// direction of the sphere. Both lights are above the box, the cone and
// the sphere.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkCompositeRenderManager.h"
#include "svtkMPICommunicator.h"
#include "svtkMPIController.h"
#include "svtkObjectFactory.h"
#include <svtk_mpi.h>

#include "svtkRegressionTestImage.h"
#include "svtkTestErrorObserver.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLRenderer.h"
#include "svtkRenderWindowInteractor.h"

#include "svtkCamera.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkImageData.h"
#include "svtkImageDataGeometryFilter.h"
#include "svtkImageSinusoidSource.h"
#include "svtkLookupTable.h"
#include "svtkPolyDataMapper.h"

#include "svtkActorCollection.h"
#include "svtkCameraPass.h"
#include "svtkCompositeZPass.h"
#include "svtkConeSource.h"
#include "svtkCubeSource.h"
#include "svtkDepthPeelingPass.h"
#include "svtkFrustumSource.h"
#include "svtkInformation.h"
#include "svtkLight.h"
#include "svtkLightCollection.h"
#include "svtkLightsPass.h"
#include "svtkMath.h"
#include "svtkOpaquePass.h"
#include "svtkOverlayPass.h"
#include "svtkPlaneSource.h"
#include "svtkPlanes.h"
#include "svtkPointData.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkRenderPassCollection.h"
#include "svtkSequencePass.h"
#include "svtkSphereSource.h"
#include "svtkTranslucentPass.h"
#include "svtkVolumetricPass.h"
#include <cassert>

#include "svtkLightActor.h"
#include "svtkProcess.h"

#include "svtkImageAppendComponents.h"
#include "svtkImageImport.h"
#include "svtkImageShiftScale.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

namespace
{

// Defined in TestLightActor.cxx
// For each spotlight, add a light frustum wireframe representation and a cone
// wireframe representation, colored with the light color.
void AddLightActors(svtkRenderer* r);

class MyProcess : public svtkProcess
{
public:
  static MyProcess* New();

  virtual void Execute();

  void SetArgs(int anArgc, char* anArgv[])
  {
    this->Argc = anArgc;
    this->Argv = anArgv;
  }

protected:
  MyProcess();

  int Argc;
  char** Argv;
};

svtkStandardNewMacro(MyProcess);

MyProcess::MyProcess()
{
  this->Argc = 0;
  this->Argv = nullptr;
}

void MyProcess::Execute()
{
  // multiprocesss logic
  int numProcs = this->Controller->GetNumberOfProcesses();
  int me = this->Controller->GetLocalProcessId();

  svtkCompositeRenderManager* prm = svtkCompositeRenderManager::New();

  svtkRenderWindowInteractor* iren = nullptr;

  if (me == 0)
  {
    iren = svtkRenderWindowInteractor::New();
  }

  svtkRenderWindow* renWin = prm->MakeRenderWindow();
  renWin->SetMultiSamples(0);

  renWin->SetAlphaBitPlanes(1);

  if (me == 0)
  {
    iren->SetRenderWindow(renWin);
  }

  svtkRenderer* renderer = prm->MakeRenderer();
  renWin->AddRenderer(renderer);
  renderer->Delete();

  svtkCameraPass* cameraP = svtkCameraPass::New();

  svtkOpaquePass* opaque = svtkOpaquePass::New();

  svtkLightsPass* lights = svtkLightsPass::New();

  SVTK_CREATE(svtkTest::ErrorObserver, errorObserver);
  svtkCompositeZPass* compositeZPass = svtkCompositeZPass::New();
  compositeZPass->SetController(this->Controller);
  compositeZPass->AddObserver(svtkCommand::ErrorEvent, errorObserver);

  svtkSequencePass* seq = svtkSequencePass::New();
  svtkRenderPassCollection* passes = svtkRenderPassCollection::New();
  passes->AddItem(lights);
  passes->AddItem(opaque);
  passes->AddItem(compositeZPass);
  compositeZPass->Delete();

  seq->SetPasses(passes);
  cameraP->SetDelegatePass(seq);

  svtkOpenGLRenderer* glrenderer = svtkOpenGLRenderer::SafeDownCast(renderer);
  glrenderer->SetPass(cameraP);

  svtkPlaneSource* rectangleSource = svtkPlaneSource::New();
  rectangleSource->SetOrigin(-5.0, 0.0, 5.0);
  rectangleSource->SetPoint1(5.0, 0.0, 5.0);
  rectangleSource->SetPoint2(-5.0, 0.0, -5.0);
  rectangleSource->SetResolution(100, 100);

  svtkPolyDataMapper* rectangleMapper = svtkPolyDataMapper::New();
  rectangleMapper->SetInputConnection(rectangleSource->GetOutputPort());
  rectangleSource->Delete();
  rectangleMapper->SetScalarVisibility(0);

  svtkActor* rectangleActor = svtkActor::New();
  rectangleActor->SetMapper(rectangleMapper);
  rectangleMapper->Delete();
  rectangleActor->SetVisibility(1);
  rectangleActor->GetProperty()->SetColor(1.0, 1.0, 1.0);

  svtkCubeSource* boxSource = svtkCubeSource::New();
  boxSource->SetXLength(2.0);
  svtkPolyDataNormals* boxNormals = svtkPolyDataNormals::New();
  boxNormals->SetInputConnection(boxSource->GetOutputPort());
  boxNormals->SetComputePointNormals(0);
  boxNormals->SetComputeCellNormals(1);
  boxNormals->Update();
  boxNormals->GetOutput()->GetPointData()->SetNormals(nullptr);

  svtkPolyDataMapper* boxMapper = svtkPolyDataMapper::New();
  boxMapper->SetInputConnection(boxNormals->GetOutputPort());
  boxNormals->Delete();
  boxSource->Delete();
  boxMapper->SetScalarVisibility(0);

  svtkActor* boxActor = svtkActor::New();

  boxActor->SetMapper(boxMapper);
  boxMapper->Delete();
  boxActor->SetVisibility(1);
  boxActor->SetPosition(-2.0, 2.0, 0.0);
  boxActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  svtkConeSource* coneSource = svtkConeSource::New();
  coneSource->SetResolution(24);
  coneSource->SetDirection(1.0, 1.0, 1.0);
  svtkPolyDataMapper* coneMapper = svtkPolyDataMapper::New();
  coneMapper->SetInputConnection(coneSource->GetOutputPort());
  coneSource->Delete();
  coneMapper->SetScalarVisibility(0);

  svtkActor* coneActor = svtkActor::New();
  coneActor->SetMapper(coneMapper);
  coneMapper->Delete();
  coneActor->SetVisibility(1);
  coneActor->SetPosition(0.0, 1.0, 1.0);
  coneActor->GetProperty()->SetColor(0.0, 0.0, 1.0);
  //  coneActor->GetProperty()->SetLighting(false);

  svtkSphereSource* sphereSource = svtkSphereSource::New();
  sphereSource->SetThetaResolution(32);
  sphereSource->SetPhiResolution(32);
  svtkPolyDataMapper* sphereMapper = svtkPolyDataMapper::New();
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
  sphereSource->Delete();
  sphereMapper->SetScalarVisibility(0);

  svtkActor* sphereActor = svtkActor::New();
  sphereActor->SetMapper(sphereMapper);
  sphereMapper->Delete();
  sphereActor->SetVisibility(1);
  sphereActor->SetPosition(2.0, 2.0, -1.0);
  sphereActor->GetProperty()->SetColor(1.0, 1.0, 0.0);

  renderer->AddViewProp(rectangleActor);
  rectangleActor->Delete();
  renderer->AddViewProp(boxActor);
  boxActor->Delete();
  renderer->AddViewProp(coneActor);
  coneActor->Delete();
  renderer->AddViewProp(sphereActor);
  sphereActor->Delete();

  // Spotlights.

  // lighting the box.
  svtkLight* l1 = svtkLight::New();
  l1->SetPosition(-4.0, 4.0, -1.0);
  l1->SetFocalPoint(boxActor->GetPosition());
  l1->SetColor(1.0, 1.0, 1.0);
  l1->SetPositional(1);
  renderer->AddLight(l1);
  l1->SetSwitch(1);
  l1->Delete();

  // lighting the sphere
  svtkLight* l2 = svtkLight::New();
  l2->SetPosition(4.0, 5.0, 1.0);
  l2->SetFocalPoint(sphereActor->GetPosition());
  l2->SetColor(1.0, 0.0, 1.0);
  //  l2->SetColor(1.0,1.0,1.0);
  l2->SetPositional(1);
  renderer->AddLight(l2);
  l2->SetSwitch(1);
  l2->Delete();

  AddLightActors(renderer);

  renderer->SetBackground(0.66, 0.66, 0.66);
  renderer->SetBackground2(157.0 / 255.0 * 0.66, 186 / 255.0 * 0.66, 192.0 / 255.0 * 0.66);
  renderer->SetGradientBackground(true);
  renWin->SetSize(400, 400);        // 400,400
  renWin->SetPosition(0, 460 * me); // translate the window
  prm->SetRenderWindow(renWin);
  prm->SetController(this->Controller);

  if (me == 0)
  {
    rectangleActor->SetVisibility(false);
    boxActor->SetVisibility(false);
  }
  else
  {
    coneActor->SetVisibility(false);
    sphereActor->SetVisibility(false);
  }

  int retVal;
  const int MY_RETURN_VALUE_MESSAGE = 0x518113;

  if (me > 0)
  {
    // satellite nodes
    prm->StartServices(); // start listening other processes (blocking call).
    // receive return value from root process.
    this->Controller->Receive(&retVal, 1, 0, MY_RETURN_VALUE_MESSAGE);
  }
  else
  {
    // root node
    renWin->Render();
    svtkCamera* camera = renderer->GetActiveCamera();
    camera->Azimuth(40.0);
    camera->Elevation(10.0);
    renderer->ResetCamera();
    // testing code
    double thresh = 10;
    int i;
    SVTK_CREATE(svtkTesting, testing);
    for (i = 0; i < this->Argc; ++i)
    {
      testing->AddArgument(this->Argv[i]);
    }

    if (testing->IsInteractiveModeSpecified())
    {
      retVal = svtkTesting::DO_INTERACTOR;
    }
    else
    {
      testing->FrontBufferOff();
      for (i = 0; i < this->Argc; i++)
      {
        if (strcmp("-FrontBuffer", this->Argv[i]) == 0)
        {
          testing->FrontBufferOn();
        }
      }

      if (testing->IsValidImageSpecified())
      {
        renWin->Render();
        if (compositeZPass->IsSupported(static_cast<svtkOpenGLRenderWindow*>(renWin)))
        {
          int* dims;
          dims = renWin->GetSize();
          float* zBuffer = new float[dims[0] * dims[1]];
          renWin->GetZbufferData(0, 0, dims[0] - 1, dims[1] - 1, zBuffer);

          svtkImageImport* importer = svtkImageImport::New();
          size_t byteSize = static_cast<size_t>(dims[0] * dims[1]);
          byteSize = byteSize * sizeof(float);
          importer->CopyImportVoidPointer(zBuffer, static_cast<int>(byteSize));
          importer->SetDataScalarTypeToFloat();
          importer->SetNumberOfScalarComponents(1);
          importer->SetWholeExtent(0, dims[0] - 1, 0, dims[1] - 1, 0, 0);
          importer->SetDataExtentToWholeExtent();

          svtkImageShiftScale* converter = svtkImageShiftScale::New();
          converter->SetInputConnection(importer->GetOutputPort());
          converter->SetOutputScalarTypeToUnsignedChar();
          converter->SetShift(0.0);
          converter->SetScale(255.0);

          // svtkImageDifference requires 3 components.
          svtkImageAppendComponents* luminanceToRGB = svtkImageAppendComponents::New();
          luminanceToRGB->SetInputConnection(0, converter->GetOutputPort());
          luminanceToRGB->AddInputConnection(0, converter->GetOutputPort());
          luminanceToRGB->AddInputConnection(0, converter->GetOutputPort());
          luminanceToRGB->Update();

          retVal = testing->RegressionTest(luminanceToRGB, thresh);

          luminanceToRGB->Delete();
          converter->Delete();
          importer->Delete();
          delete[] zBuffer;
        }
        else
        {
          retVal = svtkTesting::PASSED; // not supported.
        }
      }
      else
      {
        retVal = svtkTesting::NOT_RUN;
      }
    }

    if (retVal == svtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
    prm->StopServices(); // tells satellites to stop listening.

    // send the return value to the satellites
    i = 1;
    while (i < numProcs)
    {
      this->Controller->Send(&retVal, 1, i, MY_RETURN_VALUE_MESSAGE);
      ++i;
    }
    iren->Delete();
  }

  renWin->Delete();
  opaque->Delete();
  seq->Delete();
  passes->Delete();
  cameraP->Delete();
  lights->Delete();
  prm->Delete();
  this->ReturnValue = retVal;
}

// DUPLICATE for SVTK/Rendering/Testing/Cxx/TestLightActor.cxx

// For each spotlight, add a light frustum wireframe representation and a cone
// wireframe representation, colored with the light color.
void AddLightActors(svtkRenderer* r)
{
  assert("pre: r_exists" && r != nullptr);

  svtkLightCollection* lights = r->GetLights();

  lights->InitTraversal();
  svtkLight* l = lights->GetNextItem();
  while (l != nullptr)
  {
    double angle = l->GetConeAngle();
    if (l->LightTypeIsSceneLight() && l->GetPositional() && angle < 90.0) // spotlight
    {
      svtkLightActor* la = svtkLightActor::New();
      la->SetLight(l);
      r->AddViewProp(la);
      la->Delete();
    }
    l = lights->GetNextItem();
  }
}

}

int TestSimplePCompositeZPass(int argc, char* argv[])
{
  // This is here to avoid false leak messages from svtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  // Note that this will create a svtkMPIController if MPI
  // is configured, svtkThreadedController otherwise.
  svtkMPIController* contr = svtkMPIController::New();
  contr->Initialize(&argc, &argv, 1);

  int retVal = 1; // 1==failed

  svtkMultiProcessController::SetGlobalController(contr);

  int numProcs = contr->GetNumberOfProcesses();
  int me = contr->GetLocalProcessId();

  if (numProcs != 2)
  {
    if (me == 0)
    {
      cout << "TestSimplePCompositeZPass test requires 2 processes" << endl;
    }
    contr->Delete();
    return retVal;
  }

  if (!contr->IsA("svtkMPIController"))
  {
    if (me == 0)
    {
      cout << "DistributedData test requires MPI" << endl;
    }
    contr->Delete();
    return retVal;
  }

  MyProcess* p = MyProcess::New();
  p->SetArgs(argc, argv);

  contr->SetSingleProcessObject(p);
  contr->SingleMethodExecute();

  retVal = p->GetReturnValue();
  p->Delete();
  contr->Finalize();
  contr->Delete();

  return !retVal;
}
