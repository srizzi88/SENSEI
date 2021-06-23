#ifndef TestAxisActorInternal_h
#define TestAxisActorInternal_h

#include "svtkAxisActor.h"
#include "svtkCamera.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkStringArray.h"
#include "svtkTextProperty.h"

inline int TestAxisActorInternal(int use2dMode, int use3dProp)
{
  svtkNew<svtkStringArray> labels;
  labels->SetNumberOfTuples(6);
  labels->SetValue(0, "0");
  labels->SetValue(1, "2");
  labels->SetValue(2, "4");
  labels->SetValue(3, "6");
  labels->SetValue(4, "8");
  labels->SetValue(5, "10");

  svtkNew<svtkTextProperty> textProp1;
  textProp1->SetColor(0., 0., 1.);
  textProp1->SetOpacity(0.9);

  svtkNew<svtkTextProperty> textProp2;
  textProp2->SetColor(1., 0., 0.);
  textProp2->SetOpacity(0.6);

  svtkNew<svtkTextProperty> textProp3;
  textProp3->SetColor(0., 1., 0.);
  textProp3->SetOpacity(1);

  svtkNew<svtkProperty> prop1;
  prop1->SetColor(1., 0., 1.);

  svtkNew<svtkProperty> prop2;
  prop2->SetColor(1., 1., 0.);

  svtkNew<svtkProperty> prop3;
  prop3->SetColor(0., 1., 1.);

  //-------------  X Axis -------------
  svtkNew<svtkAxisActor> axisXActor;
  axisXActor->SetUse2DMode(use2dMode);
  axisXActor->SetUseTextActor3D(use3dProp);
  axisXActor->GetProperty()->SetAmbient(1);
  axisXActor->GetProperty()->SetDiffuse(0);
  axisXActor->SetPoint1(0, 0, 0);
  axisXActor->SetPoint2(10, 0, 0);
  axisXActor->SetTitle("X Axis");
  axisXActor->SetBounds(0, 10, 0, 0, 0, 0);
  axisXActor->SetTickLocationToBoth();
  axisXActor->SetAxisTypeToX();
  axisXActor->SetRange(0, 10);
  axisXActor->SetLabels(labels);
  axisXActor->SetDeltaRangeMajor(2);
  axisXActor->SetDeltaRangeMinor(0.5);
  axisXActor->SetExponent("+00");
  axisXActor->SetExponentVisibility(true);
  axisXActor->SetTitleScale(0.8);
  axisXActor->SetLabelScale(0.5);
  axisXActor->SetTitleOffset(3);
  axisXActor->SetExponentOffset(3);
  axisXActor->SetLabelOffset(5);
  axisXActor->SetTitleTextProperty(textProp1);
  axisXActor->SetLabelTextProperty(textProp2);
  axisXActor->SetAxisMainLineProperty(prop1);
  axisXActor->SetAxisMajorTicksProperty(prop2);
  axisXActor->SetAxisMinorTicksProperty(prop3);

  //-------------  Y Axis -------------
  svtkNew<svtkAxisActor> axisYActor;
  axisYActor->SetUse2DMode(use2dMode);
  axisYActor->SetUseTextActor3D(use3dProp);
  axisYActor->GetProperty()->SetAmbient(1);
  axisYActor->GetProperty()->SetDiffuse(0);
  axisYActor->SetPoint1(0, 0, 0);
  axisYActor->SetPoint2(0, 10, 0);
  axisYActor->SetTitle("Y Axis");
  axisYActor->SetBounds(0, 0, 0, 10, 0, 0);
  axisYActor->SetTickLocationToInside();
  axisYActor->SetAxisTypeToY();
  axisYActor->SetRange(0.1, 500);
  axisYActor->SetMajorRangeStart(0.1);
  axisYActor->SetMinorRangeStart(0.1);
  axisYActor->SetMinorTicksVisible(true);
  axisYActor->SetTitleAlignLocation(svtkAxisActor::SVTK_ALIGN_TOP);
  axisYActor->SetExponent("+00");
  axisYActor->SetExponentVisibility(true);
  axisYActor->SetExponentLocation(svtkAxisActor::SVTK_ALIGN_TOP);
  axisYActor->SetTitleScale(0.8);
  axisYActor->SetLabelScale(0.5);
  axisYActor->SetTitleOffset(3);
  axisYActor->SetExponentOffset(5);
  axisYActor->SetLabelOffset(5);
  axisYActor->SetTitleTextProperty(textProp2);
  axisYActor->SetLog(true);
  axisYActor->SetAxisLinesProperty(prop1);

  //-------------  Z Axis -------------
  svtkNew<svtkAxisActor> axisZActor;
  axisZActor->SetUse2DMode(use2dMode);
  axisZActor->SetUseTextActor3D(use3dProp);
  axisZActor->GetProperty()->SetAmbient(1);
  axisZActor->GetProperty()->SetDiffuse(0);
  axisZActor->SetPoint1(0, 0, 0);
  axisZActor->SetPoint2(0, 0, 10);
  axisZActor->SetTitle("Z Axis");
  axisZActor->SetBounds(0, 0, 0, 0, 0, 10);
  axisZActor->SetTickLocationToOutside();
  axisZActor->SetAxisTypeToZ();
  axisZActor->SetRange(0, 10);
  axisZActor->SetTitleAlignLocation(svtkAxisActor::SVTK_ALIGN_POINT2);
  axisZActor->SetExponent("+00");
  axisZActor->SetExponentVisibility(true);
  axisZActor->SetExponentLocation(svtkAxisActor::SVTK_ALIGN_POINT1);
  axisZActor->SetTitleScale(0.8);
  axisZActor->SetLabelScale(0.5);
  axisZActor->SetTitleOffset(3);
  axisZActor->SetExponentOffset(3);
  axisZActor->SetLabelOffset(5);
  axisZActor->SetTitleTextProperty(textProp3);
  axisZActor->SetMajorTickSize(3);
  axisZActor->SetMinorTickSize(1);
  axisZActor->SetDeltaRangeMajor(2);
  axisZActor->SetDeltaRangeMinor(0.1);

  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderer->AddActor(axisXActor);
  renderer->AddActor(axisYActor);
  renderer->AddActor(axisZActor);
  renderer->SetBackground(.5, .5, .5);

  svtkCamera* camera = renderer->GetActiveCamera();
  axisXActor->SetCamera(camera);
  axisYActor->SetCamera(camera);
  axisZActor->SetCamera(camera);
  renderWindow->SetSize(300, 300);

  camera->SetPosition(-10.0, 22.0, -29);
  camera->SetFocalPoint(-2, 8.5, -9.);

  renderWindow->SetMultiSamples(0);
  renderWindow->Render();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}

#endif
