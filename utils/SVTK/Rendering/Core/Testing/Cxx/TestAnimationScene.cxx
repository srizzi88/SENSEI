/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAnimationScene.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Animate a sphere source.
// Original code from
// http://www.svtk.org/pipermail/svtkusers/2005-April/079316.html

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkAnimationCue.h"
#include "svtkAnimationScene.h"
#include "svtkCommand.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"

class CueAnimator
{
public:
  CueAnimator()
  {
    this->SphereSource = nullptr;
    this->Mapper = nullptr;
    this->Actor = nullptr;
  }

  ~CueAnimator() { this->Cleanup(); }

  void StartCue(svtkAnimationCue::AnimationCueInfo* svtkNotUsed(info), svtkRenderer* ren)
  {
    cout << "*** IN StartCue " << endl;
    this->SphereSource = svtkSphereSource::New();
    this->SphereSource->SetRadius(0.5);

    this->Mapper = svtkPolyDataMapper::New();
    this->Mapper->SetInputConnection(this->SphereSource->GetOutputPort());

    this->Actor = svtkActor::New();
    this->Actor->SetMapper(this->Mapper);

    ren->AddActor(this->Actor);
    ren->ResetCamera();
    ren->Render();
  }

  void Tick(svtkAnimationCue::AnimationCueInfo* info, svtkRenderer* ren)
  {
    double newradius = 0.1 +
      (static_cast<double>(info->AnimationTime - info->StartTime) /
        static_cast<double>(info->EndTime - info->StartTime)) *
        1;
    this->SphereSource->SetRadius(newradius);
    this->SphereSource->Update();
    ren->Render();
  }

  void EndCue(svtkAnimationCue::AnimationCueInfo* svtkNotUsed(info), svtkRenderer* ren)
  {
    (void)ren;
    // don't remove the actor for the regression image.
    //      ren->RemoveActor(this->Actor);
    this->Cleanup();
  }

protected:
  svtkSphereSource* SphereSource;
  svtkPolyDataMapper* Mapper;
  svtkActor* Actor;

  void Cleanup()
  {
    if (this->SphereSource != nullptr)
    {
      this->SphereSource->Delete();
      this->SphereSource = nullptr;
    }

    if (this->Mapper != nullptr)
    {
      this->Mapper->Delete();
      this->Mapper = nullptr;
    }
    if (this->Actor != nullptr)
    {
      this->Actor->Delete();
      this->Actor = nullptr;
    }
  }
};

class svtkAnimationCueObserver : public svtkCommand
{
public:
  static svtkAnimationCueObserver* New() { return new svtkAnimationCueObserver; }

  void Execute(svtkObject* svtkNotUsed(caller), unsigned long event, void* calldata) override
  {
    if (this->Animator != nullptr && this->Renderer != nullptr)
    {
      svtkAnimationCue::AnimationCueInfo* info =
        static_cast<svtkAnimationCue::AnimationCueInfo*>(calldata);
      switch (event)
      {
        case svtkCommand::StartAnimationCueEvent:
          this->Animator->StartCue(info, this->Renderer);
          break;
        case svtkCommand::EndAnimationCueEvent:
          this->Animator->EndCue(info, this->Renderer);
          break;
        case svtkCommand::AnimationCueTickEvent:
          this->Animator->Tick(info, this->Renderer);
          break;
      }
    }
    if (this->RenWin != nullptr)
    {
      this->RenWin->Render();
    }
  }

  svtkRenderer* Renderer;
  svtkRenderWindow* RenWin;
  CueAnimator* Animator;

protected:
  svtkAnimationCueObserver()
  {
    this->Renderer = nullptr;
    this->Animator = nullptr;
    this->RenWin = nullptr;
  }
};

int TestAnimationScene(int argc, char* argv[])
{
  // Create the graphics structure. The renderer renders into the
  // render window.
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  svtkRenderer* ren1 = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->SetMultiSamples(0);
  iren->SetRenderWindow(renWin);
  renWin->AddRenderer(ren1);
  renWin->Render();

  // Create an Animation Scene
  svtkAnimationScene* scene = svtkAnimationScene::New();
  if (argc >= 2 && strcmp(argv[1], "-real") == 0)
  {
    cout << "real-time mode" << endl;
    scene->SetModeToRealTime();
  }
  else
  {
    cout << "sequence mode" << endl;
    scene->SetModeToSequence();
  }
  scene->SetLoop(0);
  scene->SetFrameRate(5);
  scene->SetStartTime(3);
  scene->SetEndTime(20);

  // Create an Animation Cue.
  svtkAnimationCue* cue1 = svtkAnimationCue::New();
  cue1->SetStartTime(5);
  cue1->SetEndTime(23);
  scene->AddCue(cue1);

  // Create cue animator;
  CueAnimator animator;

  // Create Cue observer.
  svtkAnimationCueObserver* observer = svtkAnimationCueObserver::New();
  observer->Renderer = ren1;
  observer->Animator = &animator;
  observer->RenWin = renWin;
  cue1->AddObserver(svtkCommand::StartAnimationCueEvent, observer);
  cue1->AddObserver(svtkCommand::EndAnimationCueEvent, observer);
  cue1->AddObserver(svtkCommand::AnimationCueTickEvent, observer);

  scene->Play();
  scene->Stop();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  iren->Delete();

  ren1->Delete();
  renWin->Delete();
  scene->Delete();
  cue1->Delete();
  observer->Delete();
  return !retVal;
}
