/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStackedPlotGL2PS.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkAxis.h"
#include "svtkChartXY.h"
#include "svtkColor.h"
#include "svtkColorSeries.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkDoubleArray.h"
#include "svtkGL2PSExporter.h"
#include "svtkIntArray.h"
#include "svtkNew.h"
#include "svtkPlot.h"
#include "svtkPlotStacked.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTestingInteractor.h"

// Monthly checkout data
static const char* month_labels[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep",
  "Oct", "Nov", "Dec" };
static int book[] = { 5675, 5902, 6388, 5990, 5575, 7393, 9878, 8082, 6417, 5946, 5526, 5166 };
static int new_popular[] = { 701, 687, 736, 696, 750, 814, 923, 860, 786, 735, 680, 741 };
static int periodical[] = { 184, 176, 166, 131, 171, 191, 231, 166, 197, 162, 152, 143 };
static int audiobook[] = { 903, 1038, 987, 1073, 1144, 1203, 1173, 1196, 1213, 1076, 926, 874 };
static int video[] = { 1524, 1565, 1627, 1445, 1179, 1816, 2293, 1811, 1588, 1561, 1542, 1563 };

//----------------------------------------------------------------------------
int TestStackedPlotGL2PS(int, char*[])
{
  // Set up a 2D scene, add an XY chart to it
  svtkNew<svtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(400, 300);
  svtkNew<svtkChartXY> chart;
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  svtkNew<svtkTable> table;

  svtkNew<svtkStringArray> arrMonthLabel;
  arrMonthLabel->SetNumberOfValues(12);

  svtkNew<svtkDoubleArray> arrXTickPositions;
  arrXTickPositions->SetNumberOfValues(12);

  svtkNew<svtkIntArray> arrMonth;
  arrMonth->SetName("Month");
  table->AddColumn(arrMonth);

  svtkNew<svtkIntArray> arrBook;
  arrBook->SetName("Books");
  table->AddColumn(arrBook);

  svtkNew<svtkIntArray> arrNewPopularBook;
  arrNewPopularBook->SetName("New / Popular");
  table->AddColumn(arrNewPopularBook);

  svtkNew<svtkIntArray> arrPeriodical;
  arrPeriodical->SetName("Periodical");
  table->AddColumn(arrPeriodical);

  svtkNew<svtkIntArray> arrAudiobook;
  arrAudiobook->SetName("Audiobook");
  table->AddColumn(arrAudiobook);

  svtkNew<svtkIntArray> arrVideo;
  arrVideo->SetName("Video");
  table->AddColumn(arrVideo);

  table->SetNumberOfRows(12);
  for (int i = 0; i < 12; i++)
  {
    arrMonthLabel->SetValue(i, month_labels[i]);
    arrXTickPositions->SetValue(i, i);

    arrBook->SetValue(i, book[i]);
    arrNewPopularBook->SetValue(i, new_popular[i]);
    arrPeriodical->SetValue(i, periodical[i]);
    arrAudiobook->SetValue(i, audiobook[i]);
    arrVideo->SetValue(i, video[i]);
  }

  // Set the Month Labels
  chart->GetAxis(1)->SetCustomTickPositions(arrXTickPositions, arrMonthLabel);
  chart->GetAxis(1)->SetRange(0, 11);
  chart->GetAxis(1)->SetBehavior(svtkAxis::FIXED);

  // Add multiple line plots, setting the colors etc
  svtkPlotStacked* stack = nullptr;

  // Books
  stack = svtkPlotStacked::SafeDownCast(chart->AddPlot(svtkChart::STACKED));
  stack->SetUseIndexForXSeries(true);
  stack->SetInputData(table);
  stack->SetInputArray(1, "Books");
  stack->SetInputArray(2, "New / Popular");
  stack->SetInputArray(3, "Periodical");
  stack->SetInputArray(4, "Audiobook");
  stack->SetInputArray(5, "Video");

  svtkNew<svtkColorSeries> colorSeries;
  colorSeries->SetColorScheme(svtkColorSeries::COOL);
  stack->SetColorSeries(colorSeries);

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderWindow()->Render();

  svtkNew<svtkGL2PSExporter> exp;
  exp->SetRenderWindow(view->GetRenderWindow());
  exp->SetFileFormatToPS();
  exp->UsePainterSettings();
  exp->CompressOff();
  exp->DrawBackgroundOn();

  std::string fileprefix =
    svtkTestingInteractor::TempDirectory + std::string("/TestStackedPlotGL2PS");

  exp->SetFilePrefix(fileprefix.c_str());
  exp->Write();

  // Finally render the scene and compare the image to a reference image
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
