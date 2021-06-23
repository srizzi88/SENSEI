/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <errno.h>
#include <jni.h>
#include <sstream>

#include "svtkNew.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkConeSource.h"
#include "svtkDebugLeaks.h"
#include "svtkGlyph3D.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTextActor.h"
#include "svtkTextProperty.h"

#include "svtkAndroidRenderWindowInteractor.h"
#include "svtkCommand.h"

#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "NativeSVTK", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "NativeSVTK", __VA_ARGS__))

extern "C"
{
  JNIEXPORT jlong JNICALL Java_com_kitware_JavaSVTK_JavaSVTKLib_init(
    JNIEnv* env, jobject obj, jint width, jint height);
  JNIEXPORT void JNICALL Java_com_kitware_JavaSVTK_JavaSVTKLib_render(
    JNIEnv* env, jobject obj, jlong renWinP);
  JNIEXPORT void JNICALL Java_com_kitware_JavaSVTK_JavaSVTKLib_onKeyEvent(JNIEnv* env, jobject obj,
    jlong udp, jboolean down, jint keyCode, jint metaState, jint repeatCount);
  JNIEXPORT void JNICALL Java_com_kitware_JavaSVTK_JavaSVTKLib_onMotionEvent(JNIEnv* env, jobject obj,
    jlong udp, jint action, jint eventPointer, jint numPtrs, jfloatArray xPos, jfloatArray yPos,
    jintArray ids, jint metaState);
};

struct userData
{
  svtkRenderWindow* RenderWindow;
  svtkRenderer* Renderer;
  svtkAndroidRenderWindowInteractor* Interactor;
};

// Example of updating text as we go
class svtkExampleCallback : public svtkCommand
{
public:
  static svtkExampleCallback* New() { return new svtkExampleCallback; }
  virtual void Execute(svtkObject* caller, unsigned long, void*)
  {
    // Update cardinality of selection
    double* pos = this->Camera->GetPosition();
    std::ostringstream txt;
    txt << "Camera positioned at: " << std::fixed << std::setprecision(2) << std::setw(6) << pos[0]
        << ", " << std::setw(6) << pos[1] << ", " << std::setw(6) << pos[2];
    this->Text->SetInput(txt.str().c_str());
  }

  svtkExampleCallback()
  {
    this->Camera = 0;
    this->Text = 0;
  }

  svtkCamera* Camera;
  svtkTextActor* Text;
};

/*
 * Here is where you would setup your pipeline and other normal SVTK logic
 */
JNIEXPORT jlong JNICALL Java_com_kitware_JavaSVTK_JavaSVTKLib_init(
  JNIEnv* env, jobject obj, jint width, jint height)
{
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  char jniS[4] = { 'j', 'n', 'i', 0 };
  renWin->SetWindowInfo(jniS); // tell the system that jni owns the window not us
  renWin->SetSize(width, height);
  svtkNew<svtkRenderer> renderer;
  renWin->AddRenderer(renderer.Get());

  svtkNew<svtkAndroidRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkSphereSource> sphere;
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);

  svtkNew<svtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  svtkNew<svtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper.Get());

  svtkNew<svtkConeSource> cone;
  cone->SetResolution(6);

  svtkNew<svtkGlyph3D> glyph;
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  svtkNew<svtkPolyDataMapper> spikeMapper;
  spikeMapper->SetInputConnection(glyph->GetOutputPort());

  svtkNew<svtkActor> spikeActor;
  spikeActor->SetMapper(spikeMapper.Get());

  renderer->AddActor(sphereActor.Get());
  renderer->AddActor(spikeActor.Get());
  renderer->SetBackground(0.4, 0.5, 0.6);

  svtkNew<svtkTextActor> ta;
  ta->SetInput("Droids Rock");
  ta->GetTextProperty()->SetColor(0.5, 1.0, 0.0);
  ta->SetDisplayPosition(50, 50);
  ta->GetTextProperty()->SetFontSize(32);
  renderer->AddActor(ta.Get());

  svtkNew<svtkExampleCallback> cb;
  cb->Camera = renderer->GetActiveCamera();
  cb->Text = ta.Get();
  iren->AddObserver(svtkCommand::InteractionEvent, cb.Get());

  struct userData* foo = new struct userData();
  foo->RenderWindow = renWin;
  foo->Renderer = renderer.Get();
  foo->Interactor = iren.Get();

  return (jlong)foo;
}

JNIEXPORT void JNICALL Java_com_kitware_JavaSVTK_JavaSVTKLib_render(
  JNIEnv* env, jobject obj, jlong udp)
{
  struct userData* foo = (userData*)(udp);
  foo->RenderWindow->SwapBuffersOff(); // android does it
  foo->RenderWindow->Render();
  foo->RenderWindow->SwapBuffersOn(); // reset
}

JNIEXPORT void JNICALL Java_com_kitware_JavaSVTK_JavaSVTKLib_onKeyEvent(JNIEnv* env, jobject obj,
  jlong udp, jboolean down, jint keyCode, jint metaState, jint repeatCount)
{
  struct userData* foo = (userData*)(udp);
  foo->Interactor->HandleKeyEvent(down, keyCode, metaState, repeatCount);
}

JNIEXPORT void JNICALL Java_com_kitware_JavaSVTK_JavaSVTKLib_onMotionEvent(JNIEnv* env, jobject obj,
  jlong udp, jint action, jint eventPointer, jint numPtrs, jfloatArray xPos, jfloatArray yPos,
  jintArray ids, jint metaState)
{
  struct userData* foo = (userData*)(udp);

  int xPtr[SVTKI_MAX_POINTERS];
  int yPtr[SVTKI_MAX_POINTERS];
  int idPtr[SVTKI_MAX_POINTERS];

  // only allow SVTKI_MAX_POINTERS touches right now
  if (numPtrs > SVTKI_MAX_POINTERS)
  {
    numPtrs = SVTKI_MAX_POINTERS;
  }

  // fill in the arrays
  jfloat* xJPtr = env->GetFloatArrayElements(xPos, 0);
  jfloat* yJPtr = env->GetFloatArrayElements(yPos, 0);
  jint* idJPtr = env->GetIntArrayElements(ids, 0);
  for (int i = 0; i < numPtrs; ++i)
  {
    xPtr[i] = (int)xJPtr[i];
    yPtr[i] = (int)yJPtr[i];
    idPtr[i] = idJPtr[i];
  }
  env->ReleaseIntArrayElements(ids, idJPtr, 0);
  env->ReleaseFloatArrayElements(xPos, xJPtr, 0);
  env->ReleaseFloatArrayElements(yPos, yJPtr, 0);

  foo->Interactor->HandleMotionEvent(action, eventPointer, numPtrs, xPtr, yPtr, idPtr, metaState);
}
