/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLinePlot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkAxis.h"
#include "svtkChartLegend.h"
#include "svtkChartXY.h"
#include "svtkColorSeries.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkDoubleArray.h"
#include "svtkIntArray.h"
#include "svtkPlotBar.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTextProperty.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

const int num_months = 12;

static const int month[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };

static const int book_2008[] = { 5675, 5902, 6388, 5990, 5575, 7393, 9878, 8082, 6417, 5946, 5526,
  5166 };
static const int new_popular_2008[] = { 701, 687, 736, 696, 750, 814, 923, 860, 786, 735, 680,
  741 };
static const int periodical_2008[] = { 184, 176, 166, 131, 171, 191, 231, 166, 197, 162, 152, 143 };
static const int audiobook_2008[] = { 903, 1038, 987, 1073, 1144, 1203, 1173, 1196, 1213, 1076, 926,
  874 };
static const int video_2008[] = { 1524, 1565, 1627, 1445, 1179, 1816, 2293, 1811, 1588, 1561, 1542,
  1563 };

static const int book_2009[] = { 6388, 5990, 5575, 9878, 8082, 5675, 7393, 5902, 5526, 5166, 5946,
  6417 };
static const int new_popular_2009[] = { 696, 735, 786, 814, 736, 860, 750, 687, 923, 680, 741,
  701 };
static const int periodical_2009[] = { 197, 166, 176, 231, 171, 152, 166, 131, 184, 191, 143, 162 };
static const int audiobook_2009[] = { 1213, 1076, 926, 987, 903, 1196, 1073, 1144, 1203, 1038, 874,
  1173 };
static const int video_2009[] = { 2293, 1561, 1542, 1627, 1588, 1179, 1563, 1445, 1811, 1565, 1524,
  1816 };

static void build_array(const char* name, svtkIntArray* array, const int c_array[])
{
  array->SetName(name);
  for (int i = 0; i < num_months; ++i)
  {
    array->InsertNextValue(c_array[i]);
  }
}

//----------------------------------------------------------------------------
int TestStackedBarGraph(int, char*[])
{
  // Set up a 2D scene, add an XY chart to it
  SVTK_CREATE(svtkContextView, view);
  view->GetRenderWindow()->SetSize(500, 350);
  SVTK_CREATE(svtkChartXY, chart);
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  SVTK_CREATE(svtkTable, table);

  SVTK_CREATE(svtkIntArray, arrMonth);
  build_array("Month", arrMonth, month);
  table->AddColumn(arrMonth);

  SVTK_CREATE(svtkIntArray, arrBooks2008);
  build_array("Books 2008", arrBooks2008, book_2008);
  table->AddColumn(arrBooks2008);
  SVTK_CREATE(svtkIntArray, arrNewPopular2008);
  build_array("New / Popular 2008", arrNewPopular2008, new_popular_2008);
  table->AddColumn(arrNewPopular2008);
  SVTK_CREATE(svtkIntArray, arrPeriodical2008);
  build_array("Periodical 2008", arrPeriodical2008, periodical_2008);
  table->AddColumn(arrPeriodical2008);
  SVTK_CREATE(svtkIntArray, arrAudiobook2008);
  build_array("Audiobook 2008", arrAudiobook2008, audiobook_2008);
  table->AddColumn(arrAudiobook2008);
  SVTK_CREATE(svtkIntArray, arrVideo2008);
  build_array("Video 2008", arrVideo2008, video_2008);
  table->AddColumn(arrVideo2008);

  SVTK_CREATE(svtkIntArray, arrBooks2009);
  build_array("Books 2009", arrBooks2009, book_2009);
  table->AddColumn(arrBooks2009);
  SVTK_CREATE(svtkIntArray, arrNewPopular2009);
  build_array("New / Popular 2009", arrNewPopular2009, new_popular_2009);
  table->AddColumn(arrNewPopular2009);
  SVTK_CREATE(svtkIntArray, arrPeriodical2009);
  build_array("Periodical 2009", arrPeriodical2009, periodical_2009);
  table->AddColumn(arrPeriodical2009);
  SVTK_CREATE(svtkIntArray, arrAudiobook2009);
  build_array("Audiobook 2009", arrAudiobook2009, audiobook_2009);
  table->AddColumn(arrAudiobook2009);
  SVTK_CREATE(svtkIntArray, arrVideo2009);
  build_array("Video 2009", arrVideo2009, video_2009);
  table->AddColumn(arrVideo2009);

  // Create a color series to use with our stacks.
  SVTK_CREATE(svtkColorSeries, colorSeries1);
  colorSeries1->SetColorScheme(svtkColorSeries::WILD_FLOWER);

  // Add multiple line plots, setting the colors etc
  svtkPlotBar* bar = nullptr;

  bar = svtkPlotBar::SafeDownCast(chart->AddPlot(svtkChart::BAR));
  bar->SetColorSeries(colorSeries1);
  bar->SetInputData(table, "Month", "Books 2008");
  bar->SetInputArray(2, "New / Popular 2008");
  bar->SetInputArray(3, "Periodical 2008");
  bar->SetInputArray(4, "Audiobook 2008");
  bar->SetInputArray(5, "Video 2008");

  SVTK_CREATE(svtkColorSeries, colorSeries2);
  colorSeries2->SetColorScheme(svtkColorSeries::WILD_FLOWER);

  bar = svtkPlotBar::SafeDownCast(chart->AddPlot(svtkChart::BAR));
  bar->SetColorSeries(colorSeries2);
  bar->SetInputData(table, "Month", "Books 2009");
  bar->SetInputArray(2, "New / Popular 2009");
  bar->SetInputArray(3, "Periodical 2009");
  bar->SetInputArray(4, "Audiobook 2009");
  bar->SetInputArray(5, "Video 2009");

  chart->SetShowLegend(true);
  svtkAxis* axis = chart->GetAxis(svtkAxis::BOTTOM);
  axis->SetBehavior(1);
  axis->SetMaximum(13.0);
  axis->SetTitle("Month");
  chart->GetAxis(svtkAxis::LEFT)->SetTitle("");
  chart->SetTitle("Circulation 2008, 2009");

  // Set up the legend to be off to the top right of the viewport.
  chart->GetLegend()->SetInline(false);
  chart->GetLegend()->SetHorizontalAlignment(svtkChartLegend::RIGHT);
  chart->GetLegend()->SetVerticalAlignment(svtkChartLegend::TOP);

  // Set up some custom labels for months.
  svtkSmartPointer<svtkDoubleArray> dates = svtkSmartPointer<svtkDoubleArray>::New();
  svtkSmartPointer<svtkStringArray> strings = svtkSmartPointer<svtkStringArray>::New();
  dates->InsertNextValue(1);
  strings->InsertNextValue("January");
  dates->InsertNextValue(2);
  strings->InsertNextValue("February");
  dates->InsertNextValue(3);
  strings->InsertNextValue("March");
  dates->InsertNextValue(4);
  strings->InsertNextValue("April");
  dates->InsertNextValue(5);
  strings->InsertNextValue("May");
  dates->InsertNextValue(6);
  strings->InsertNextValue("June");
  dates->InsertNextValue(7);
  strings->InsertNextValue("July");
  dates->InsertNextValue(8);
  strings->InsertNextValue("August");
  dates->InsertNextValue(9);
  strings->InsertNextValue("September");
  dates->InsertNextValue(10);
  strings->InsertNextValue("October");
  dates->InsertNextValue(11);
  strings->InsertNextValue("November");
  dates->InsertNextValue(12);
  strings->InsertNextValue("December");
  axis->SetCustomTickPositions(dates, strings);
  axis->GetLabelProperties()->SetOrientation(90);
  axis->GetLabelProperties()->SetVerticalJustification(SVTK_TEXT_CENTERED);
  axis->GetLabelProperties()->SetJustification(SVTK_TEXT_RIGHT);

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
