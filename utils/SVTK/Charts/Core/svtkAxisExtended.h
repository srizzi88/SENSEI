/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCellLocator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAxisExtended
 * @brief   extended axis tick positioning
 *
 * This implements the optimization based tick position calculating algorithm in the paper "An
 * Extension of Wilkinson's Algorithm for Positioning Tick Labels on Axes" by Junstin Talbot, Sharon
 * Lin and Pat Hanrahan
 *
 *
 * @sa
 * svtkAxis
 */

#ifndef svtkAxisExtended_h
#define svtkAxisExtended_h
#endif

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkObject.h"
#include "svtkVector.h" // Needed for svtkVector

class SVTKCHARTSCORE_EXPORT svtkAxisExtended : public svtkObject
{
public:
  svtkTypeMacro(svtkAxisExtended, svtkObject);
  static svtkAxisExtended* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * This method return a value to make step sizes corresponding to low q
   * and j values more preferable
   */
  static double Simplicity(int qIndex, int qLength, int j, double lmin, double lmax, double lstep);

  /**
   * This method returns the maximum possible value of simplicity value given
   * q and j
   */
  static double SimplicityMax(int qIndex, int qLength, int j);

  /**
   * This method makes the data range approximately same as the labeling
   * range more preferable
   */
  static double Coverage(double dmin, double dmax, double lmin, double lmax);

  /**
   * This gives the maximum possible value of coverage given the step size
   */
  static double CoverageMax(double dmin, double dmax, double span);

  /**
   * This method return a value to make the density of the labels close to
   * the user given value
   */
  static double Density(int k, double m, double dmin, double dmax, double lmin, double lmax);

  /**
   * Derives the maximum values for density given k (number of ticks) and
   * m (user given)
   */
  static double DensityMax(int k, double m);

  /**
   * This methods return the legibility score of different formats
   */
  static double FormatLegibilityScore(double n, int format);

  /**
   * This method returns the string length of different format notations.
   */
  static int FormatStringLength(int format, double n, int precision);

  /**
   * This method implements the algorithm given in the paper
   * The method return the minimum tick position, maximum tick position and
   * the tick spacing
   */
  svtkVector3d GenerateExtendedTickLabels(double dmin, double dmax, double m, double scaling);

  //@{
  /**
   * Set/Get methods for variables
   */
  svtkGetMacro(FontSize, int);
  svtkSetMacro(FontSize, int);
  //@}

  svtkGetMacro(DesiredFontSize, int);
  svtkSetMacro(DesiredFontSize, int);

  svtkGetMacro(Precision, int);
  svtkSetMacro(Precision, int);
  svtkGetMacro(LabelFormat, int);
  svtkSetMacro(LabelFormat, int);

  svtkGetMacro(Orientation, int);
  svtkSetMacro(Orientation, int);

  svtkGetMacro(IsAxisVertical, bool);
  svtkSetMacro(IsAxisVertical, bool);

protected:
  svtkAxisExtended();
  ~svtkAxisExtended() override;

  /**
   * This method implements an exhaustive search of the legibilty parameters.
   */
  double Legibility(
    double lmin, double lmax, double lstep, double scaling, svtkVector<int, 3>& parameters);

  int Orientation;
  int FontSize;
  int DesiredFontSize;
  int Precision;
  int LabelFormat;
  bool LabelLegibilityChanged;
  bool IsAxisVertical;

private:
  svtkAxisExtended(const svtkAxisExtended&) = delete;
  void operator=(const svtkAxisExtended&) = delete;
};
