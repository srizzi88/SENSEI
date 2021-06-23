/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPieChartActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPieChartActor
 * @brief   create a pie chart from an array
 *
 * svtkPieChartActor generates a pie chart from an array of numbers defined in
 * field data (a svtkDataObject). To use this class, you must specify an input
 * data object. You'll probably also want to specify the position of the plot
 * be setting the Position and Position2 instance variables, which define a
 * rectangle in which the plot lies.  There are also many other instance
 * variables that control the look of the plot includes its title,
 * and legend.
 *
 * Set the text property/attributes of the title and the labels through the
 * svtkTextProperty objects associated with these components.
 *
 * @sa
 * svtkParallelCoordinatesActor svtkXYPlotActor2D svtkSpiderPlotActor
 */

#ifndef svtkPieChartActor_h
#define svtkPieChartActor_h

#include "svtkActor2D.h"
#include "svtkRenderingAnnotationModule.h" // For export macro

class svtkAlgorithmOutput;
class svtkAxisActor2D;
class svtkDataObject;
class svtkPolyData;
class svtkPolyDataMapper2D;
class svtkTextMapper;
class svtkTextProperty;
class svtkLegendBoxActor;
class svtkGlyphSource2D;
class svtkPieChartActorConnection;
class svtkPieceLabelArray;

class SVTKRENDERINGANNOTATION_EXPORT svtkPieChartActor : public svtkActor2D
{
public:
  //@{
  /**
   * Standard methods for type information and printing.
   */
  svtkTypeMacro(svtkPieChartActor, svtkActor2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Instantiate this class.
   */
  static svtkPieChartActor* New();

  //@{
  /**
   * Set the input to the pie chart actor. SetInputData()
   * does not connect the pipeline whereas SetInputConnection()
   * does.
   */
  virtual void SetInputData(svtkDataObject*);
  virtual void SetInputConnection(svtkAlgorithmOutput*);
  //@}

  /**
   * Get the input data object to this actor.
   */
  virtual svtkDataObject* GetInput();

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
   * Set/Get the title of the pie chart.
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
   * Enable/Disable the display of pie piece labels.
   */
  svtkSetMacro(LabelVisibility, svtkTypeBool);
  svtkGetMacro(LabelVisibility, svtkTypeBool);
  svtkBooleanMacro(LabelVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the labels text property. This controls the appearance
   * of all pie piece labels.
   */
  virtual void SetLabelTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(LabelTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Specify colors for each piece of pie. If not specified, they are
   * automatically generated.
   */
  void SetPieceColor(int i, double r, double g, double b);
  void SetPieceColor(int i, const double color[3])
  {
    this->SetPieceColor(i, color[0], color[1], color[2]);
  }
  double* GetPieceColor(int i);
  //@}

  //@{
  /**
   * Specify the names for each piece of pie.  not specified, then an integer
   * number is automatically generated.
   */
  void SetPieceLabel(const int i, const char*);
  const char* GetPieceLabel(int i);
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
   * Draw the pie plot.
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
  svtkPieChartActor();
  ~svtkPieChartActor() override;

private:
  svtkPieChartActorConnection* ConnectionHolder;

  svtkIdType ArrayNumber;
  svtkIdType ComponentNumber;
  svtkTypeBool TitleVisibility; // Should I see the title?
  char* Title;                 // The title string
  svtkTextProperty* TitleTextProperty;
  svtkTypeBool LabelVisibility;
  svtkTextProperty* LabelTextProperty;
  svtkPieceLabelArray* Labels;
  svtkTypeBool LegendVisibility;
  svtkLegendBoxActor* LegendActor;
  svtkGlyphSource2D* GlyphSource;

  // Local variables needed to plot
  svtkIdType N;       // The number of values
  double Total;      // The total of all values in the data array
  double* Fractions; // The fraction of the pie

  svtkTextMapper** PieceMappers; // a label for each radial spoke
  svtkActor2D** PieceActors;

  svtkTextMapper* TitleMapper;
  svtkActor2D* TitleActor;

  svtkPolyData* WebData; // The web of the spider plot
  svtkPolyDataMapper2D* WebMapper;
  svtkActor2D* WebActor;

  svtkPolyData* PlotData; // The lines drawn within the axes
  svtkPolyDataMapper2D* PlotMapper;
  svtkActor2D* PlotActor;

  svtkTimeStamp BuildTime;

  double Center[3];
  double Radius;

  int LastPosition[2];
  int LastPosition2[2];
  double P1[3];
  double P2[3];

  void Initialize();
  int PlaceAxes(svtkViewport* viewport, const int* size);
  int BuildPlot(svtkViewport*);

private:
  svtkPieChartActor(const svtkPieChartActor&) = delete;
  void operator=(const svtkPieChartActor&) = delete;
};

#endif
