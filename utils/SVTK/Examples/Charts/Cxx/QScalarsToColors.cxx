/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QScalarsToColors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "QSVTKOpenGLStereoWidget.h"
#include "svtkChartXY.h"
#include "svtkColorTransferFunction.h"
#include "svtkCompositeTransferFunctionItem.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkFloatArray.h"
#include "svtkGenericOpenGLRenderWindow.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPlot.h"
#include "svtkQtTableView.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTimerLog.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QSurfaceFormat>
#include <QWidget>

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  // needed to ensure appropriate OpenGL context is created for SVTK rendering.
  QSurfaceFormat::setDefaultFormat(QSVTKOpenGLStereoWidget::defaultFormat());

  // Qt initialization
  QApplication app(argc, argv);

  // QSVTK set up and initialization
  QSVTKOpenGLStereoWidget qsvtkWidget;

  svtkNew<svtkGenericOpenGLRenderWindow> renderWindow;
  qsvtkWidget.setRenderWindow(renderWindow);

  // Set up my 2D world...
  svtkNew<svtkContextView> view;
  ; // This contains a chart object
  view->SetRenderWindow(qsvtkWidget.renderWindow());
  view->SetInteractor(qsvtkWidget.interactor());

  svtkNew<svtkChartXY> chart;
  chart->SetTitle("Chart");
  view->GetScene()->AddItem(chart);

  svtkNew<svtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->AddHSVSegment(0., 0., 1., 1., 0.3333, 0.3333, 1., 1.);
  colorTransferFunction->AddHSVSegment(0.3333, 0.3333, 1., 1., 0.6666, 0.6666, 1., 1.);
  colorTransferFunction->AddHSVSegment(0.6666, 0.6666, 1., 1., 1., 0., 1., 1.);
  colorTransferFunction->Build();

  svtkNew<svtkPiecewiseFunction> opacityFunction;
  opacityFunction->AddPoint(0., 0.);
  opacityFunction->AddPoint(0.5, 0.5);
  opacityFunction->AddPoint(1., 1.);

  svtkNew<svtkCompositeTransferFunctionItem> item3;
  item3->SetColorTransferFunction(colorTransferFunction);
  item3->SetOpacityFunction(opacityFunction);
  item3->SetOpacity(0.2);
  item3->SetMaskAboveCurve(true);
  chart->AddPlot(item3);

  // Now show the application and start the event loop
  qsvtkWidget.show();
  return app.exec();
}
