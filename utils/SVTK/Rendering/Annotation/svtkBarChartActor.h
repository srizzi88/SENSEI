/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBarChartActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBarChartActor
 * @brief   create a bar chart from an array
 *
 * svtkBarChartActor generates a bar chart from an array of numbers defined in
 * field data (a svtkDataObject). To use this class, you must specify an input
 * data object. You'll probably also want to specify the position of the plot
 * be setting the Position and Position2 instance variables, which define a
 * rectangle in which the plot lies.  There are also many other instance
 * variables that control the look of the plot includes its title and legend.
 *
 * Set the text property/attributes of the title and the labels through the
 * svtkTextProperty objects associated with these components.
 *
 * @sa
 * svtkParallelCoordinatesActor svtkXYPlotActor svtkSpiderPlotActor
 * svtkPieChartActor
 */

#ifndef svtkBarChartActor_h
#define svtkBarChartActor_h

#include "svtkActor2D.h"
#include "svtkRenderingAnnotationModule.h" // For export macro

class svtkAxisActor2D;
class svtkDataObject;
class svtkPolyData;
class svtkPolyDataMapper2D;
class svtkTextMapper;
class svtkTextProperty;
class svtkLegendBoxActor;
class svtkGlyphSource2D;
class svtkBarLabelArray;

class SVTKRENDERINGANNOTATION_EXPORT svtkBarChartActor : public svtkActor2D
{
public:
  //@{
  /**
   * Standard methods for type information and printing.
   */
  svtkTypeMacro(svtkBarChartActor, svtkActor2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Instantiate this class.
   */
  static svtkBarChartActor* New();

  /**
   * Set the input to the bar chart actor.
   */
  virtual void SetInput(svtkDataObject*);

  //@{
  /**
   * Get the input data object to this actor.
   */
  svtkGetObjectMacro(Input, svtkDataObject);
  //@}

  //@{
  /**
   * Enable/Disable the display of a plot title.
   */
  svtkSetMacro(TitleVisibility, svtkTypeBool);
  svtkGetMacro(TitleVisibility, svtkTypeBool);
  svtkBooleanMacro(TitleVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the title of the bar chart.
   */
  svtkSetStringMacro(Title);
  svtkGetStringMacro(Title);
  //@}

  //@{
  /**
   * Set/Get the title text property. The property controls the
   * appearance of the plot title.
   */
  virtual void SetTitleTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(TitleTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Enable/Disable the display of bar labels.
   */
  svtkSetMacro(LabelVisibility, svtkTypeBool);
  svtkGetMacro(LabelVisibility, svtkTypeBool);
  svtkBooleanMacro(LabelVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the labels text property. This controls the appearance
   * of all bar bar labels.
   */
  virtual void SetLabelTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(LabelTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Specify colors for each bar. If not specified, they are
   * automatically generated.
   */
  void SetBarColor(int i, double r, double g, double b);
  void SetBarColor(int i, const double color[3])
  {
    this->SetBarColor(i, color[0], color[1], color[2]);
  }
  double* GetBarColor(int i);
  //@}

  //@{
  /**
   * Specify the names of each bar. If
   * not specified, then an integer number is automatically generated.
   */
  void SetBarLabel(const int i, const char*);
  const char* GetBarLabel(int i);
  //@}

  //@{
  /**
   * Specify the title of the y-axis.
   */
  svtkSetStringMacro(YTitle);
  svtkGetStringMacro(YTitle);
  //@}

  //@{
  /**
   * Enable/Disable the creation of a legend. If on, the legend labels will
   * be created automatically unless the per plot legend symbol has been
   * set.
   */
  svtkSetMacro(LegendVisibility, svtkTypeBool);
  svtkGetMacro(LegendVisibility, svtkTypeBool);
  svtkBooleanMacro(LegendVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Retrieve handles to the legend box. This is useful if you would like
   * to manually control the legend appearance.
   */
  svtkGetObjectMacro(LegendActor, svtkLegendBoxActor);
  //@}

  //@{
  /**
   * Draw the bar plot.
   */
  int RenderOverlay(svtkViewport*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override { return 0; }
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

protected:
  svtkBarChartActor();
  ~svtkBarChartActor() override;

private:
  svtkDataObject* Input; // List of data sets to plot
  svtkIdType ArrayNumber;
  svtkIdType ComponentNumber;
  svtkTypeBool TitleVisibility; // Should I see the title?
  char* Title;                 // The title string
  svtkTextProperty* TitleTextProperty;
  svtkTypeBool LabelVisibility;
  svtkTextProperty* LabelTextProperty;
  svtkBarLabelArray* Labels;
  svtkTypeBool LegendVisibility;
  svtkLegendBoxActor* LegendActor;
  svtkGlyphSource2D* GlyphSource;

  // Local variables needed to plot
  svtkIdType N;      // The number of values
  double* Heights;  // The heights of each bar
  double MinHeight; // The maximum and minimum height
  double MaxHeight;
  double LowerLeft[2];
  double UpperRight[2];

  svtkTextMapper** BarMappers; // a label for each bar
  svtkActor2D** BarActors;

  svtkTextMapper* TitleMapper;
  svtkActor2D* TitleActor;

  svtkPolyData* PlotData; // The actual bars plus the x-axis
  svtkPolyDataMapper2D* PlotMapper;
  svtkActor2D* PlotActor;

  svtkAxisActor2D* YAxis; // The y-axis
  char* YTitle;

  svtkTimeStamp BuildTime;

  int LastPosition[2];
  int LastPosition2[2];
  double P1[3];
  double P2[3];

  void Initialize();
  int PlaceAxes(svtkViewport* viewport, const int* size);
  int BuildPlot(svtkViewport*);

private:
  svtkBarChartActor(const svtkBarChartActor&) = delete;
  void operator=(const svtkBarChartActor&) = delete;
};

#endif
