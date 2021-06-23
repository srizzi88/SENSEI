
#include "QtSVTKRenderWindows.h"
#include "ui_QtSVTKRenderWindows.h"

#include "svtkBoundedPlanePointPlacer.h"
#include "svtkCellPicker.h"
#include "svtkCommand.h"
#include "svtkDICOMImageReader.h"
#include "svtkDistanceRepresentation.h"
#include "svtkDistanceRepresentation2D.h"
#include "svtkDistanceWidget.h"
#include "svtkHandleRepresentation.h"
#include "svtkImageData.h"
#include "svtkImageMapToWindowLevelColors.h"
#include "svtkImageSlabReslice.h"
#include "svtkInteractorStyleImage.h"
#include "svtkLookupTable.h"
#include "svtkPlane.h"
#include "svtkPlaneSource.h"
#include "svtkPointHandleRepresentation2D.h"
#include "svtkPointHandleRepresentation3D.h"
#include "svtkProperty.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkResliceCursor.h"
#include "svtkResliceCursorActor.h"
#include "svtkResliceCursorLineRepresentation.h"
#include "svtkResliceCursorPolyDataAlgorithm.h"
#include "svtkResliceCursorThickLineRepresentation.h"
#include "svtkResliceCursorWidget.h"
#include "svtkResliceImageViewer.h"
#include "svtkResliceImageViewerMeasurements.h"
#include <svtkGenericOpenGLRenderWindow.h>
#include <svtkRenderWindow.h>
#include <svtkRenderer.h>

//----------------------------------------------------------------------------
class svtkResliceCursorCallback : public svtkCommand
{
public:
  static svtkResliceCursorCallback* New() { return new svtkResliceCursorCallback; }

  void Execute(svtkObject* caller, unsigned long ev, void* callData) override
  {

    if (ev == svtkResliceCursorWidget::WindowLevelEvent || ev == svtkCommand::WindowLevelEvent ||
      ev == svtkResliceCursorWidget::ResliceThicknessChangedEvent)
    {
      // Render everything
      for (int i = 0; i < 3; i++)
      {
        this->RCW[i]->Render();
      }
      this->IPW[0]->GetInteractor()->GetRenderWindow()->Render();
      return;
    }

    svtkImagePlaneWidget* ipw = dynamic_cast<svtkImagePlaneWidget*>(caller);
    if (ipw)
    {
      double* wl = static_cast<double*>(callData);

      if (ipw == this->IPW[0])
      {
        this->IPW[1]->SetWindowLevel(wl[0], wl[1], 1);
        this->IPW[2]->SetWindowLevel(wl[0], wl[1], 1);
      }
      else if (ipw == this->IPW[1])
      {
        this->IPW[0]->SetWindowLevel(wl[0], wl[1], 1);
        this->IPW[2]->SetWindowLevel(wl[0], wl[1], 1);
      }
      else if (ipw == this->IPW[2])
      {
        this->IPW[0]->SetWindowLevel(wl[0], wl[1], 1);
        this->IPW[1]->SetWindowLevel(wl[0], wl[1], 1);
      }
    }

    svtkResliceCursorWidget* rcw = dynamic_cast<svtkResliceCursorWidget*>(caller);
    if (rcw)
    {
      svtkResliceCursorLineRepresentation* rep =
        dynamic_cast<svtkResliceCursorLineRepresentation*>(rcw->GetRepresentation());
      // Although the return value is not used, we keep the get calls
      // in case they had side-effects
      rep->GetResliceCursorActor()->GetCursorAlgorithm()->GetResliceCursor();
      for (int i = 0; i < 3; i++)
      {
        svtkPlaneSource* ps = static_cast<svtkPlaneSource*>(this->IPW[i]->GetPolyDataAlgorithm());
        ps->SetOrigin(
          this->RCW[i]->GetResliceCursorRepresentation()->GetPlaneSource()->GetOrigin());
        ps->SetPoint1(
          this->RCW[i]->GetResliceCursorRepresentation()->GetPlaneSource()->GetPoint1());
        ps->SetPoint2(
          this->RCW[i]->GetResliceCursorRepresentation()->GetPlaneSource()->GetPoint2());

        // If the reslice plane has modified, update it on the 3D widget
        this->IPW[i]->UpdatePlacement();
      }
    }

    // Render everything
    for (int i = 0; i < 3; i++)
    {
      this->RCW[i]->Render();
    }
    this->IPW[0]->GetInteractor()->GetRenderWindow()->Render();
  }

  svtkResliceCursorCallback() {}
  svtkImagePlaneWidget* IPW[3];
  svtkResliceCursorWidget* RCW[3];
};

QtSVTKRenderWindows::QtSVTKRenderWindows(int svtkNotUsed(argc), char* argv[])
{
  this->ui = new Ui_QtSVTKRenderWindows;
  this->ui->setupUi(this);

  svtkSmartPointer<svtkDICOMImageReader> reader = svtkSmartPointer<svtkDICOMImageReader>::New();
  reader->SetDirectoryName(argv[1]);
  reader->Update();
  int imageDims[3];
  reader->GetOutput()->GetDimensions(imageDims);

  for (int i = 0; i < 3; i++)
  {
    riw[i] = svtkSmartPointer<svtkResliceImageViewer>::New();
    svtkNew<svtkGenericOpenGLRenderWindow> renderWindow;
    riw[i]->SetRenderWindow(renderWindow);
  }

  this->ui->view1->setRenderWindow(riw[0]->GetRenderWindow());
  riw[0]->SetupInteractor(this->ui->view1->renderWindow()->GetInteractor());

  this->ui->view2->setRenderWindow(riw[1]->GetRenderWindow());
  riw[1]->SetupInteractor(this->ui->view2->renderWindow()->GetInteractor());

  this->ui->view3->setRenderWindow(riw[2]->GetRenderWindow());
  riw[2]->SetupInteractor(this->ui->view3->renderWindow()->GetInteractor());

  for (int i = 0; i < 3; i++)
  {
    // make them all share the same reslice cursor object.
    svtkResliceCursorLineRepresentation* rep = svtkResliceCursorLineRepresentation::SafeDownCast(
      riw[i]->GetResliceCursorWidget()->GetRepresentation());
    riw[i]->SetResliceCursor(riw[0]->GetResliceCursor());

    rep->GetResliceCursorActor()->GetCursorAlgorithm()->SetReslicePlaneNormal(i);

    riw[i]->SetInputData(reader->GetOutput());
    riw[i]->SetSliceOrientation(i);
    riw[i]->SetResliceModeToAxisAligned();
  }

  svtkSmartPointer<svtkCellPicker> picker = svtkSmartPointer<svtkCellPicker>::New();
  picker->SetTolerance(0.005);

  svtkSmartPointer<svtkProperty> ipwProp = svtkSmartPointer<svtkProperty>::New();

  svtkSmartPointer<svtkRenderer> ren = svtkSmartPointer<svtkRenderer>::New();

  svtkNew<svtkGenericOpenGLRenderWindow> renderWindow;
  this->ui->view4->setRenderWindow(renderWindow);
  this->ui->view4->renderWindow()->AddRenderer(ren);
  svtkRenderWindowInteractor* iren = this->ui->view4->interactor();

  for (int i = 0; i < 3; i++)
  {
    planeWidget[i] = svtkSmartPointer<svtkImagePlaneWidget>::New();
    planeWidget[i]->SetInteractor(iren);
    planeWidget[i]->SetPicker(picker);
    planeWidget[i]->RestrictPlaneToVolumeOn();
    double color[3] = { 0, 0, 0 };
    color[i] = 1;
    planeWidget[i]->GetPlaneProperty()->SetColor(color);

    color[0] /= 4.0;
    color[1] /= 4.0;
    color[2] /= 4.0;
    riw[i]->GetRenderer()->SetBackground(color);

    planeWidget[i]->SetTexturePlaneProperty(ipwProp);
    planeWidget[i]->TextureInterpolateOff();
    planeWidget[i]->SetResliceInterpolateToLinear();
    planeWidget[i]->SetInputConnection(reader->GetOutputPort());
    planeWidget[i]->SetPlaneOrientation(i);
    planeWidget[i]->SetSliceIndex(imageDims[i] / 2);
    planeWidget[i]->DisplayTextOn();
    planeWidget[i]->SetDefaultRenderer(ren);
    planeWidget[i]->SetWindowLevel(1358, -27);
    planeWidget[i]->On();
    planeWidget[i]->InteractionOn();
  }

  svtkSmartPointer<svtkResliceCursorCallback> cbk = svtkSmartPointer<svtkResliceCursorCallback>::New();

  for (int i = 0; i < 3; i++)
  {
    cbk->IPW[i] = planeWidget[i];
    cbk->RCW[i] = riw[i]->GetResliceCursorWidget();
    riw[i]->GetResliceCursorWidget()->AddObserver(
      svtkResliceCursorWidget::ResliceAxesChangedEvent, cbk);
    riw[i]->GetResliceCursorWidget()->AddObserver(svtkResliceCursorWidget::WindowLevelEvent, cbk);
    riw[i]->GetResliceCursorWidget()->AddObserver(
      svtkResliceCursorWidget::ResliceThicknessChangedEvent, cbk);
    riw[i]->GetResliceCursorWidget()->AddObserver(svtkResliceCursorWidget::ResetCursorEvent, cbk);
    riw[i]->GetInteractorStyle()->AddObserver(svtkCommand::WindowLevelEvent, cbk);

    // Make them all share the same color map.
    riw[i]->SetLookupTable(riw[0]->GetLookupTable());
    planeWidget[i]->GetColorMap()->SetLookupTable(riw[0]->GetLookupTable());
    // planeWidget[i]->GetColorMap()->SetInput(riw[i]->GetResliceCursorWidget()->GetResliceCursorRepresentation()->GetColorMap()->GetInput());
    planeWidget[i]->SetColorMap(
      riw[i]->GetResliceCursorWidget()->GetResliceCursorRepresentation()->GetColorMap());
  }

  this->ui->view1->show();
  this->ui->view2->show();
  this->ui->view3->show();

  // Set up action signals and slots
  connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));
  connect(this->ui->resliceModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(resliceMode(int)));
  connect(this->ui->thickModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(thickMode(int)));
  this->ui->thickModeCheckBox->setEnabled(0);

  connect(this->ui->radioButton_Max, SIGNAL(pressed()), this, SLOT(SetBlendModeToMaxIP()));
  connect(this->ui->radioButton_Min, SIGNAL(pressed()), this, SLOT(SetBlendModeToMinIP()));
  connect(this->ui->radioButton_Mean, SIGNAL(pressed()), this, SLOT(SetBlendModeToMeanIP()));
  this->ui->blendModeGroupBox->setEnabled(0);

  connect(this->ui->resetButton, SIGNAL(pressed()), this, SLOT(ResetViews()));
  connect(
    this->ui->AddDistance1Button, SIGNAL(pressed()), this, SLOT(AddDistanceMeasurementToView1()));
};

void QtSVTKRenderWindows::slotExit()
{
  qApp->exit();
}

void QtSVTKRenderWindows::resliceMode(int mode)
{
  this->ui->thickModeCheckBox->setEnabled(mode ? 1 : 0);
  this->ui->blendModeGroupBox->setEnabled(mode ? 1 : 0);

  for (int i = 0; i < 3; i++)
  {
    riw[i]->SetResliceMode(mode ? 1 : 0);
    riw[i]->GetRenderer()->ResetCamera();
    riw[i]->Render();
  }
}

void QtSVTKRenderWindows::thickMode(int mode)
{
  for (int i = 0; i < 3; i++)
  {
    riw[i]->SetThickMode(mode ? 1 : 0);
    riw[i]->Render();
  }
}

void QtSVTKRenderWindows::SetBlendMode(int m)
{
  for (int i = 0; i < 3; i++)
  {
    svtkImageSlabReslice* thickSlabReslice =
      svtkImageSlabReslice::SafeDownCast(svtkResliceCursorThickLineRepresentation::SafeDownCast(
        riw[i]->GetResliceCursorWidget()->GetRepresentation())
                                          ->GetReslice());
    thickSlabReslice->SetBlendMode(m);
    riw[i]->Render();
  }
}

void QtSVTKRenderWindows::SetBlendModeToMaxIP()
{
  this->SetBlendMode(SVTK_IMAGE_SLAB_MAX);
}

void QtSVTKRenderWindows::SetBlendModeToMinIP()
{
  this->SetBlendMode(SVTK_IMAGE_SLAB_MIN);
}

void QtSVTKRenderWindows::SetBlendModeToMeanIP()
{
  this->SetBlendMode(SVTK_IMAGE_SLAB_MEAN);
}

void QtSVTKRenderWindows::ResetViews()
{
  // Reset the reslice image views
  for (int i = 0; i < 3; i++)
  {
    riw[i]->Reset();
  }

  // Also sync the Image plane widget on the 3D top right view with any
  // changes to the reslice cursor.
  for (int i = 0; i < 3; i++)
  {
    svtkPlaneSource* ps = static_cast<svtkPlaneSource*>(planeWidget[i]->GetPolyDataAlgorithm());
    ps->SetNormal(riw[0]->GetResliceCursor()->GetPlane(i)->GetNormal());
    ps->SetCenter(riw[0]->GetResliceCursor()->GetPlane(i)->GetOrigin());

    // If the reslice plane has modified, update it on the 3D widget
    this->planeWidget[i]->UpdatePlacement();
  }

  // Render in response to changes.
  this->Render();
}

void QtSVTKRenderWindows::Render()
{
  for (int i = 0; i < 3; i++)
  {
    riw[i]->Render();
  }
  this->ui->view3->renderWindow()->Render();
}

void QtSVTKRenderWindows::AddDistanceMeasurementToView1()
{
  this->AddDistanceMeasurementToView(1);
}

void QtSVTKRenderWindows::AddDistanceMeasurementToView(int i)
{
  // remove existing widgets.
  if (this->DistanceWidget[i])
  {
    this->DistanceWidget[i]->SetEnabled(0);
    this->DistanceWidget[i] = nullptr;
  }

  // add new widget
  this->DistanceWidget[i] = svtkSmartPointer<svtkDistanceWidget>::New();
  this->DistanceWidget[i]->SetInteractor(this->riw[i]->GetResliceCursorWidget()->GetInteractor());

  // Set a priority higher than our reslice cursor widget
  this->DistanceWidget[i]->SetPriority(
    this->riw[i]->GetResliceCursorWidget()->GetPriority() + 0.01);

  svtkSmartPointer<svtkPointHandleRepresentation2D> handleRep =
    svtkSmartPointer<svtkPointHandleRepresentation2D>::New();
  svtkSmartPointer<svtkDistanceRepresentation2D> distanceRep =
    svtkSmartPointer<svtkDistanceRepresentation2D>::New();
  distanceRep->SetHandleRepresentation(handleRep);
  this->DistanceWidget[i]->SetRepresentation(distanceRep);
  distanceRep->InstantiateHandleRepresentation();
  distanceRep->GetPoint1Representation()->SetPointPlacer(riw[i]->GetPointPlacer());
  distanceRep->GetPoint2Representation()->SetPointPlacer(riw[i]->GetPointPlacer());

  // Add the distance to the list of widgets whose visibility is managed based
  // on the reslice plane by the ResliceImageViewerMeasurements class
  this->riw[i]->GetMeasurements()->AddItem(this->DistanceWidget[i]);

  this->DistanceWidget[i]->CreateDefaultRepresentation();
  this->DistanceWidget[i]->EnabledOn();
}
