/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkViewTheme.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkViewTheme
 * @brief   Sets theme colors for a graphical view.
 *
 *
 * This may be set on any subclass of svtkView.  The view class will attempt
 * to use the values set in the theme to customize the view.  Views will not
 * generally use every aspect of the theme.
 * NOTICE: This class will be deprecated in favor of a more robust
 * solution based on style sheets.  Do not become overly-dependent on the
 * functionality of themes.
 */

#ifndef svtkViewTheme_h
#define svtkViewTheme_h

#include "svtkObject.h"
#include "svtkViewsCoreModule.h" // For export macro

class svtkScalarsToColors;
class svtkTextProperty;

class SVTKVIEWSCORE_EXPORT svtkViewTheme : public svtkObject
{
public:
  static svtkViewTheme* New();
  svtkTypeMacro(svtkViewTheme, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The size of points or vertices
   */
  svtkSetMacro(PointSize, double);
  svtkGetMacro(PointSize, double);
  //@}

  //@{
  /**
   * The width of lines or edges
   */
  svtkSetMacro(LineWidth, double);
  svtkGetMacro(LineWidth, double);
  //@}

  //@{
  /**
   * The color and opacity of points or vertices when not mapped through
   * a lookup table.
   */
  svtkSetVector3Macro(PointColor, double);
  svtkGetVector3Macro(PointColor, double);
  svtkSetMacro(PointOpacity, double);
  svtkGetMacro(PointOpacity, double);
  //@}

  //@{
  /**
   * The ranges to use in the point lookup table.
   * You may also do this by accessing the point lookup table directly
   * with GetPointLookupTable() and calling these methods.
   */
  virtual void SetPointHueRange(double mn, double mx);
  virtual void SetPointHueRange(double rng[2]);
  virtual double* GetPointHueRange();
  virtual void GetPointHueRange(double& mn, double& mx);
  virtual void GetPointHueRange(double rng[2]);
  //@}

  virtual void SetPointSaturationRange(double mn, double mx);
  virtual void SetPointSaturationRange(double rng[2]);
  virtual double* GetPointSaturationRange();
  virtual void GetPointSaturationRange(double& mn, double& mx);
  virtual void GetPointSaturationRange(double rng[2]);

  virtual void SetPointValueRange(double mn, double mx);
  virtual void SetPointValueRange(double rng[2]);
  virtual double* GetPointValueRange();
  virtual void GetPointValueRange(double& mn, double& mx);
  virtual void GetPointValueRange(double rng[2]);

  virtual void SetPointAlphaRange(double mn, double mx);
  virtual void SetPointAlphaRange(double rng[2]);
  virtual double* GetPointAlphaRange();
  virtual void GetPointAlphaRange(double& mn, double& mx);
  virtual void GetPointAlphaRange(double rng[2]);

  //@{
  /**
   * Set/Get the point lookup table.
   */
  svtkGetObjectMacro(PointLookupTable, svtkScalarsToColors);
  virtual void SetPointLookupTable(svtkScalarsToColors* lut);
  //@}

  //@{
  /**
   * Whether to scale the lookup table to fit the range of the data.
   */
  svtkSetMacro(ScalePointLookupTable, bool);
  svtkGetMacro(ScalePointLookupTable, bool);
  svtkBooleanMacro(ScalePointLookupTable, bool);
  //@}

  //@{
  /**
   * The color and opacity of cells or edges when not mapped through
   * a lookup table.
   */
  svtkSetVector3Macro(CellColor, double);
  svtkGetVector3Macro(CellColor, double);
  svtkSetMacro(CellOpacity, double);
  svtkGetMacro(CellOpacity, double);
  //@}

  //@{
  /**
   * The ranges to use in the cell lookup table.
   * You may also do this by accessing the cell lookup table directly
   * with GetCellLookupTable() and calling these methods.
   */
  virtual void SetCellHueRange(double mn, double mx);
  virtual void SetCellHueRange(double rng[2]);
  virtual double* GetCellHueRange();
  virtual void GetCellHueRange(double& mn, double& mx);
  virtual void GetCellHueRange(double rng[2]);
  //@}

  virtual void SetCellSaturationRange(double mn, double mx);
  virtual void SetCellSaturationRange(double rng[2]);
  virtual double* GetCellSaturationRange();
  virtual void GetCellSaturationRange(double& mn, double& mx);
  virtual void GetCellSaturationRange(double rng[2]);

  virtual void SetCellValueRange(double mn, double mx);
  virtual void SetCellValueRange(double rng[2]);
  virtual double* GetCellValueRange();
  virtual void GetCellValueRange(double& mn, double& mx);
  virtual void GetCellValueRange(double rng[2]);

  virtual void SetCellAlphaRange(double mn, double mx);
  virtual void SetCellAlphaRange(double rng[2]);
  virtual double* GetCellAlphaRange();
  virtual void GetCellAlphaRange(double& mn, double& mx);
  virtual void GetCellAlphaRange(double rng[2]);

  //@{
  /**
   * Set/Get the cell lookup table.
   */
  svtkGetObjectMacro(CellLookupTable, svtkScalarsToColors);
  virtual void SetCellLookupTable(svtkScalarsToColors* lut);
  //@}

  //@{
  /**
   * Whether to scale the lookup table to fit the range of the data.
   */
  svtkSetMacro(ScaleCellLookupTable, bool);
  svtkGetMacro(ScaleCellLookupTable, bool);
  svtkBooleanMacro(ScaleCellLookupTable, bool);
  //@}

  //@{
  /**
   * The color of any outlines in the view.
   */
  svtkSetVector3Macro(OutlineColor, double);
  svtkGetVector3Macro(OutlineColor, double);
  //@}

  //@{
  /**
   * The color of selected points or vertices.
   */
  svtkSetVector3Macro(SelectedPointColor, double);
  svtkGetVector3Macro(SelectedPointColor, double);
  svtkSetMacro(SelectedPointOpacity, double);
  svtkGetMacro(SelectedPointOpacity, double);
  //@}

  //@{
  /**
   * The color of selected cells or edges.
   */
  svtkSetVector3Macro(SelectedCellColor, double);
  svtkGetVector3Macro(SelectedCellColor, double);
  svtkSetMacro(SelectedCellOpacity, double);
  svtkGetMacro(SelectedCellOpacity, double);
  //@}

  //@{
  /**
   * The view background color.
   */
  svtkSetVector3Macro(BackgroundColor, double);
  svtkGetVector3Macro(BackgroundColor, double);
  //@}

  //@{
  /**
   * The second background color (for gradients).
   */
  svtkSetVector3Macro(BackgroundColor2, double);
  svtkGetVector3Macro(BackgroundColor2, double);
  //@}

  //@{
  /**
   * The text property to use for labeling points/vertices.
   */
  virtual void SetPointTextProperty(svtkTextProperty* tprop);
  svtkGetObjectMacro(PointTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * The text property to use for labeling edges/cells.
   */
  virtual void SetCellTextProperty(svtkTextProperty* tprop);
  svtkGetObjectMacro(CellTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * The color to use for labeling graph vertices.
   * This is deprecated. Use GetPointTextProperty()->SetColor() instead.
   */
  virtual void SetVertexLabelColor(double r, double g, double b);
  virtual void SetVertexLabelColor(double c[3]) { this->SetVertexLabelColor(c[0], c[1], c[2]); }
  virtual double* GetVertexLabelColor();
  virtual void GetVertexLabelColor(double& r, double& g, double& b)
  {
    double* c = this->GetVertexLabelColor();
    if (c)
    {
      r = c[0];
      g = c[1];
      b = c[2];
    }
  }
  virtual void GetVertexLabelColor(double c[3]) { this->GetVertexLabelColor(c[0], c[1], c[2]); }
  //@}

  //@{
  /**
   * The color to use for labeling graph edges.
   * This is deprecated. Use GetCellTextProperty()->SetColor() instead.
   */
  virtual void SetEdgeLabelColor(double r, double g, double b);
  virtual void SetEdgeLabelColor(double c[3]) { this->SetEdgeLabelColor(c[0], c[1], c[2]); }
  virtual double* GetEdgeLabelColor();
  virtual void GetEdgeLabelColor(double& r, double& g, double& b)
  {
    double* c = this->GetEdgeLabelColor();
    if (c)
    {
      r = c[0];
      g = c[1];
      b = c[2];
    }
  }
  virtual void GetEdgeLabelColor(double c[3]) { this->GetEdgeLabelColor(c[0], c[1], c[2]); }
  //@}

  //@{
  /**
   * Convenience methods for creating some default view themes.
   * The return reference is reference-counted, so you will have to call
   * Delete() on the reference when you are finished with it.
   */
  static svtkViewTheme* CreateOceanTheme();
  static svtkViewTheme* CreateMellowTheme();
  static svtkViewTheme* CreateNeonTheme();
  //@}

  //@{
  /**
   * Whether a given lookup table matches the point or cell theme of this
   * theme.
   */
  bool LookupMatchesPointTheme(svtkScalarsToColors* s2c);
  bool LookupMatchesCellTheme(svtkScalarsToColors* s2c);
  //@}

protected:
  svtkViewTheme();
  ~svtkViewTheme() override;

  double PointSize;
  double LineWidth;

  double PointColor[3];
  double PointOpacity;

  double CellColor[3];
  double CellOpacity;

  double OutlineColor[3];

  double SelectedPointColor[3];
  double SelectedPointOpacity;
  double SelectedCellColor[3];
  double SelectedCellOpacity;

  double BackgroundColor[3];
  double BackgroundColor2[3];

  svtkScalarsToColors* PointLookupTable;
  svtkScalarsToColors* CellLookupTable;

  bool ScalePointLookupTable;
  bool ScaleCellLookupTable;

  svtkTextProperty* PointTextProperty;
  svtkTextProperty* CellTextProperty;

private:
  svtkViewTheme(const svtkViewTheme&) = delete;
  void operator=(const svtkViewTheme&) = delete;
};

#endif
