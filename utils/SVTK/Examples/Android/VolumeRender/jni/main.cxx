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

#define SYNTHETIC 1
#ifdef SYNTHETIC
#include "svtkImageCast.h"
#include "svtkRTAnalyticSource.h"
#else
#include "svtkNrrdReader.h"
#endif

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkConeSource.h"
#include "svtkDebugLeaks.h"
#include "svtkGlyph3D.h"
#include "svtkImageData.h"
#include "svtkOpenGLGPUVolumeRayCastMapper.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTextActor.h"
#include "svtkTextProperty.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

#include "svtkAndroidRenderWindowInteractor.h"
#include "svtkCommand.h"

#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "NativeSVTK", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "NativeSVTK", __VA_ARGS__))

extern "C"
{
  JNIEXPORT jlong JNICALL Java_com_kitware_VolumeRender_VolumeRenderLib_init(
    JNIEnv* env, jobject obj, jint width, jint height);
  JNIEXPORT void JNICALL Java_com_kitware_VolumeRender_VolumeRenderLib_render(
    JNIEnv* env, jobject obj, jlong renWinP);
  JNIEXPORT void JNICALL Java_com_kitware_VolumeRender_VolumeRenderLib_onKeyEvent(JNIEnv* env,
    jobject obj, jlong udp, jboolean down, jint keyCode, jint metaState, jint repeatCount);
  JNIEXPORT void JNICALL Java_com_kitware_VolumeRender_VolumeRenderLib_onMotionEvent(JNIEnv* env,
    jobject obj, jlong udp, jint action, jint eventPointer, jint numPtrs, jfloatArray xPos,
    jfloatArray yPos, jintArray ids, jint metaState);
};

struct userData
{
  svtkRenderWindow* RenderWindow;
  svtkRenderer* Renderer;
  svtkAndroidRenderWindowInteractor* Interactor;
};

/*
 * Here is where you would setup your pipeline and other normal SVTK logic
 */
JNIEXPORT jlong JNICALL Java_com_kitware_VolumeRender_VolumeRenderLib_init(
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

  svtkNew<svtkOpenGLGPUVolumeRayCastMapper> volumeMapper;

  svtkNew<svtkPiecewiseFunction> pwf;

#ifdef SYNTHETIC
  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-63, 64, -63, 64, -63, 64);
  wavelet->SetCenter(0.0, 0.0, 0.0);

  svtkNew<svtkImageCast> ic;
  ic->SetInputConnection(wavelet->GetOutputPort());
  ic->SetOutputScalarTypeToUnsignedChar();
  volumeMapper->SetInputConnection(ic->GetOutputPort());

  pwf->AddPoint(0, 0);
  pwf->AddPoint(255, 0.1);
#else
  svtkNew<svtkNrrdReader> mi;
  mi->SetFileName("/sdcard/CT-chest-quantized.nrrd");
  mi->Update();

  double range[2];
  mi->GetOutput()->GetPointData()->GetScalars()->GetRange(range);
  LOGI("Min %f Max %f type %s", range[0], range[1], mi->GetOutput()->GetScalarTypeAsString());

  volumeMapper->SetInputConnection(mi->GetOutputPort());

  double tweak = 80.0;
  pwf->AddPoint(0, 0);
  pwf->AddPoint(255 * (67.0106 + tweak) / 3150.0, 0);
  pwf->AddPoint(255 * (251.105 + tweak) / 3150.0, 0.3);
  pwf->AddPoint(255 * (439.291 + tweak) / 3150.0, 0.5);
  pwf->AddPoint(255 * 3071 / 3150.0, 0.616071);
#endif

  volumeMapper->SetAutoAdjustSampleDistances(1);
  volumeMapper->SetSampleDistance(0.5);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->SetShade(1);
  volumeProperty->SetInterpolationTypeToLinear();

  svtkNew<svtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0, 0, 0, 0);
  ctf->AddRGBPoint(255 * 67.0106 / 3150.0, 0.54902, 0.25098, 0.14902);
  ctf->AddRGBPoint(255 * 251.105 / 3150.0, 0.882353, 0.603922, 0.290196);
  ctf->AddRGBPoint(255 * 439.291 / 3150.0, 1, 0.937033, 0.954531);
  ctf->AddRGBPoint(255 * 3071 / 3150.0, 0.827451, 0.658824, 1);

  volumeProperty->SetColor(ctf.GetPointer());
  volumeProperty->SetScalarOpacity(pwf.GetPointer());

  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  renderer->SetBackground2(0.2, 0.3, 0.4);
  renderer->SetBackground(0.1, 0.1, 0.1);
  renderer->GradientBackgroundOn();
  renderer->AddVolume(volume.GetPointer());
  renderer->ResetCamera();
  //  renderer->GetActiveCamera()->Zoom(1.4);
  renderer->GetActiveCamera()->Zoom(0.7);

  struct userData* foo = new struct userData();
  foo->RenderWindow = renWin;
  foo->Renderer = renderer.Get();
  foo->Interactor = iren.Get();

  return (jlong)foo;
}

JNIEXPORT void JNICALL Java_com_kitware_VolumeRender_VolumeRenderLib_render(
  JNIEnv* env, jobject obj, jlong udp)
{
  struct userData* foo = (userData*)(udp);
  foo->RenderWindow->SwapBuffersOff(); // android does it
  foo->RenderWindow->Render();
  foo->RenderWindow->SwapBuffersOn(); // reset
}

JNIEXPORT void JNICALL Java_com_kitware_VolumeRender_VolumeRenderLib_onKeyEvent(JNIEnv* env,
  jobject obj, jlong udp, jboolean down, jint keyCode, jint metaState, jint repeatCount)
{
  struct userData* foo = (userData*)(udp);
  foo->Interactor->HandleKeyEvent(down, keyCode, metaState, repeatCount);
}

JNIEXPORT void JNICALL Java_com_kitware_VolumeRender_VolumeRenderLib_onMotionEvent(JNIEnv* env,
  jobject obj, jlong udp, jint action, jint eventPointer, jint numPtrs, jfloatArray xPos,
  jfloatArray yPos, jintArray ids, jint metaState)
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
