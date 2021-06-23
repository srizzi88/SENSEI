/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCategoryLegend.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkCategoryLegend
 * @brief   Legend item to display categorical data.
 *
 * svtkCategoryLegend will display a label and color patch for each value
 * in a categorical data set.  To use this class, you must first populate
 * a svtkScalarsToColors by using the SetAnnotation() method.  The other
 * input to this class is a svtkVariantArray.  This should contain the
 * annotated values from the svtkScalarsToColors that you wish to include
 * within the legend.
 */

#ifndef svtkCategoryLegend_h
#define svtkCategoryLegend_h

#include "svtkChartLegend.h"
#include "svtkChartsCoreModule.h" // For export macro
#include "svtkNew.h"              // For svtkNew ivars
#include "svtkStdString.h"        // For svtkStdString ivars
#include "svtkVector.h"           // For svtkRectf

class svtkScalarsToColors;
class svtkTextProperty;
class svtkVariantArray;

class SVTKCHARTSCORE_EXPORT svtkCategoryLegend : public svtkChartLegend
{
public:
  svtkTypeMacro(svtkCategoryLegend, svtkChartLegend);
  static svtkCategoryLegend* New();

  /**
   * Enum of legend orientation types
   */
  enum
  {
    VERTICAL = 0,
    HORIZONTAL
  };

  /**
   * Paint the legend into a rectangle defined by the bounds.
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Compute and return the lower left corner of this legend, along
   * with its width and height.
   */
  svtkRectf GetBoundingRect(svtkContext2D* painter) override;

  //@{
  /**
   * Get/Set the svtkScalarsToColors used to draw this legend.
   * Since this legend represents categorical data, this
   * svtkScalarsToColors must have been populated using SetAnnotation().
   */
  virtual void SetScalarsToColors(svtkScalarsToColors* stc);
  virtual svtkScalarsToColors* GetScalarsToColors();
  //@}

  //@{
  /**
   * Get/Set the array of values that will be represented by this legend.
   * This array must contain distinct annotated values from the ScalarsToColors.
   * Each value in this array will be drawn as a separate entry within this
   * legend.
   */
  svtkGetMacro(Values, svtkVariantArray*);
  svtkSetMacro(Values, svtkVariantArray*);
  //@}

  //@{
  /**
   * Get/set the title text of the legend.
   */
  virtual void SetTitle(const svtkStdString& title);
  virtual svtkStdString GetTitle();
  //@}

  //@{
  /**
   * Get/set the label to use for outlier data.
   */
  svtkGetMacro(OutlierLabel, svtkStdString);
  svtkSetMacro(OutlierLabel, svtkStdString);
  //@}

protected:
  svtkCategoryLegend();
  ~svtkCategoryLegend() override;

  bool HasOutliers;
  float TitleWidthOffset;
  svtkScalarsToColors* ScalarsToColors;
  svtkStdString OutlierLabel;
  svtkStdString Title;
  svtkNew<svtkTextProperty> TitleProperties;
  svtkVariantArray* Values;

private:
  svtkCategoryLegend(const svtkCategoryLegend&) = delete;
  void operator=(const svtkCategoryLegend&) = delete;
};

#endif
