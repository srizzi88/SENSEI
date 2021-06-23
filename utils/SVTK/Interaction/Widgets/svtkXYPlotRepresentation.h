/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkScalarBarRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkXYPlotRepresentation
 * @brief   represent XY plot for svtkXYPlotWidget
 *
 *
 *
 * This class represents a XY plot for a svtkXYPlotWidget.  This class
 * provides support for interactively placing a XY plot on the 2D overlay
 * plane.  The XY plot is defined by an instance of svtkXYPlotActor.
 *
 * @sa
 * svtkXYPlotWidget svtkWidgetRepresentation svtkXYPlotActor
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Kitware SAS 2012
 */

#ifndef svtkXYPlotRepresentation_h
#define svtkXYPlotRepresentation_h

#include "svtkBorderRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkXYPlotActor;

class SVTKINTERACTIONWIDGETS_EXPORT svtkXYPlotRepresentation : public svtkBorderRepresentation
{
public:
  svtkTypeMacro(svtkXYPlotRepresentation, svtkBorderRepresentation);
  virtual void PrintSelf(ostream& os, svtkIndent indent);
  static svtkXYPlotRepresentation* New();

  //@{
  /**
   * The prop that is placed in the renderer.
   */
  svtkGetObjectMacro(XYPlotActor, svtkXYPlotActor);
  virtual void SetXYPlotActor(svtkXYPlotActor*);
  //@}

  //@{
  /**
   * Satisfy the superclass' API.
   */
  void BuildRepresentation() override;
  void WidgetInteraction(double eventPos[2]) override;
  void GetSize(double size[2]) override
  {
    size[0] = 2.0;
    size[1] = 2.0;
  }
  //@}

  //@{
  /**
   * These methods are necessary to make this representation behave as
   * a svtkProp.
   */
  virtual int GetVisibility();
  virtual void SetVisibility(int);
  virtual void GetActors2D(svtkPropCollection* collection);
  virtual void ReleaseGraphicsResources(svtkWindow* window);
  virtual int RenderOverlay(svtkViewport*);
  virtual int RenderOpaqueGeometry(svtkViewport*);
  virtual int RenderTranslucentPolygonalGeometry(svtkViewport*);
  virtual svtkTypeBool HasTranslucentPolygonalGeometry();
  //@}

  //@{
  /**
   * Set glyph properties
   */
  void SetGlyphSize(double x);
  void SetPlotGlyphType(int curve, int glyph);
  //@}

  //@{
  /**
   * Set title properties
   */
  void SetTitle(const char* title);
  void SetTitleColor(double r, double g, double b);
  void SetTitleFontFamily(int x);
  void SetTitleBold(int x);
  void SetTitleItalic(int x);
  void SetTitleShadow(int x);
  void SetTitleFontSize(int x);
  void SetTitleJustification(int x);
  void SetTitleVerticalJustification(int x);
  void SetAdjustTitlePosition(int x);
  void SetTitlePosition(double x, double y);
  //@}

  //@{
  /**
   * Set/Get axis properties
   */
  void SetXAxisColor(double r, double g, double b);
  void SetYAxisColor(double r, double g, double b);
  void SetXTitle(const char* ytitle);
  char* GetXTitle();
  void SetXRange(double min, double max);
  void SetYTitle(const char* ytitle);
  char* GetYTitle();
  void SetYRange(double min, double max);
  void SetYTitlePosition(int pos);
  int GetYTitlePosition() const;
  void SetXValues(int x);
  //@}

  //@{
  /**
   * Set axis title properties
   */
  void SetAxisTitleColor(double r, double g, double b);
  void SetAxisTitleFontFamily(int x);
  void SetAxisTitleBold(int x);
  void SetAxisTitleItalic(int x);
  void SetAxisTitleShadow(int x);
  void SetAxisTitleFontSize(int x);
  void SetAxisTitleJustification(int x);
  void SetAxisTitleVerticalJustification(int x);
  //@}

  //@{
  /**
   * Set axis label properties
   */
  void SetAxisLabelColor(double r, double g, double b);
  void SetAxisLabelFontFamily(int x);
  void SetAxisLabelBold(int x);
  void SetAxisLabelItalic(int x);
  void SetAxisLabelShadow(int x);
  void SetAxisLabelFontSize(int x);
  void SetAxisLabelJustification(int x);
  void SetAxisLabelVerticalJustification(int x);
  void SetXLabelFormat(const char* _arg);
  void SetYLabelFormat(const char* _arg);
  //@}

  //@{
  /**
   * Set various properties
   */
  void SetBorder(int x);
  void RemoveAllActiveCurves();
  void AddUserCurvesPoint(double c, double x, double y);
  void SetLegend(int x);
  void SetLegendBorder(int b);
  void SetLegendBox(int b);
  void SetLegendBoxColor(double r, double g, double b);
  void SetLegendPosition(double x, double y);
  void SetLegendPosition2(double x, double y);
  void SetLineWidth(double w);
  void SetPlotColor(int i, int r, int g, int b);
  void SetPlotLines(int i);
  void SetPlotPoints(int i);
  void SetPlotLabel(int i, const char* label);
  //@}

protected:
  svtkXYPlotRepresentation();
  ~svtkXYPlotRepresentation() override;

  svtkXYPlotActor* XYPlotActor;

private:
  svtkXYPlotRepresentation(const svtkXYPlotRepresentation&) = delete;
  void operator=(const svtkXYPlotRepresentation&) = delete;
};

#endif // svtkXYPlotRepresentation_h
