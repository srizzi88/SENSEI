/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QChartTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "QSVTKOpenGLStereoWidget.h"
#include "svtkChartXY.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkFloatArray.h"
#include "svtkGenericOpenGLRenderWindow.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkPlot.h"
#include "svtkQtTableView.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"
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
  QMainWindow mainWindow;
  mainWindow.setGeometry(0, 0, 1150, 600);

  QSVTKOpenGLStereoWidget* qsvtkWidget = new QSVTKOpenGLStereoWidget(&mainWindow);

  svtkNew<svtkGenericOpenGLRenderWindow> renderWindow;
  qsvtkWidget->setRenderWindow(renderWindow);

  // Set up my 2D world...
  svtkNew<svtkContextView> view; // This contains a chart object
  view->SetRenderWindow(renderWindow);
  view->SetInteractor(renderWindow->GetInteractor());

  // Create a table with some points in it...
  svtkNew<svtkTable> table;
  svtkNew<svtkFloatArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX);
  svtkNew<svtkFloatArray> arrC;
  arrC->SetName("Cosine");
  table->AddColumn(arrC);
  svtkNew<svtkFloatArray> arrS;
  arrS->SetName("Sine");
  table->AddColumn(arrS);

  // Make a timer object - need to get some frame rates/render times
  svtkNew<svtkTimerLog> timer;

  // Test charting with a few more points...
  int numPoints = 29;
  float inc = 7.0 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + 0.0);
    table->SetValue(i, 2, sin(i * inc) + 0.0);
  }

  //   table->Update();

  // Add multiple line plots, setting the colors etc
  svtkNew<svtkChartXY> chart;
  view->GetScene()->AddItem(chart);
  svtkPlot* line = chart->AddPlot(svtkChart::LINE);
  line->SetInputData(table, 0, 1);
  line->SetColor(255, 0, 0, 255);
  line = chart->AddPlot(svtkChart::LINE);
  line->SetInputData(table, 0, 2);
  line->SetColor(0, 255, 0, 255);
  line->SetWidth(2.0);

  // Instantiate a svtkQtChart and use that too
  /*  svtkQtChart *qtChart = new svtkQtChart;
    chart = qtChart->chart();
    line = chart->AddPlot(svtkChart::LINE);
    line->SetTable(table, 0, 1);
    line->SetColor(255, 0, 0, 255);
    line = chart->AddPlot(svtkChart::LINE);
    line->SetTable(table, 0, 2);
    line->SetColor(0, 255, 0, 255);
    line->SetWidth(2.0);
  */
  // Now lets try to add a table view
  QWidget* widget = new QWidget(&mainWindow);
  QHBoxLayout* layout = new QHBoxLayout(widget);
  svtkNew<svtkQtTableView> tableView;
  tableView->SetSplitMultiComponentColumns(true);
  tableView->AddRepresentationFromInput(table);
  tableView->Update();
  layout->addWidget(qsvtkWidget, 2);
  // layout->addWidget(qtChart, 2);
  layout->addWidget(tableView->GetWidget());
  mainWindow.setCentralWidget(widget);

  // Now show the application and start the event loop
  mainWindow.show();

  return app.exec();
}
