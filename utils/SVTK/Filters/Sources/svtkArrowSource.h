/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkArrowSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkArrowSource
 * @brief   Appends a cylinder to a cone to form an arrow.
 *
 * svtkArrowSource was intended to be used as the source for a glyph.
 * The shaft base is always at (0,0,0). The arrow tip is always at (1,0,0). If
 * "Invert" is true, then the ends are flipped i.e. tip is at (0,0,0) while
 * base is at (1, 0, 0).
 * The resolution of the cone and shaft can be set and default to 6.
 * The radius of the cone and shaft can be set and default to 0.03 and 0.1.
 * The length of the tip can also be set, and defaults to 0.35.
 */

#ifndef svtkArrowSource_h
#define svtkArrowSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSSOURCES_EXPORT svtkArrowSource : public svtkPolyDataAlgorithm
{
public:
  /**
   * Construct cone with angle of 45 degrees.
   */
  static svtkArrowSource* New();

  svtkTypeMacro(svtkArrowSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the length, and radius of the tip.  They default to 0.35 and 0.1
   */
  svtkSetClampMacro(TipLength, double, 0.0, 1.0);
  svtkGetMacro(TipLength, double);
  svtkSetClampMacro(TipRadius, double, 0.0, 10.0);
  svtkGetMacro(TipRadius, double);
  //@}

  //@{
  /**
   * Set the resolution of the tip.  The tip behaves the same as a cone.
   * Resoultion 1 gives a single triangle, 2 gives two crossed triangles.
   */
  svtkSetClampMacro(TipResolution, int, 1, 128);
  svtkGetMacro(TipResolution, int);
  //@}

  //@{
  /**
   * Set the radius of the shaft.  Defaults to 0.03.
   */
  svtkSetClampMacro(ShaftRadius, double, 0.0, 5.0);
  svtkGetMacro(ShaftRadius, double);
  //@}

  //@{
  /**
   * Set the resolution of the shaft.  2 gives a rectangle.
   * I would like to extend the cone to produce a line,
   * but this is not an option now.
   */
  svtkSetClampMacro(ShaftResolution, int, 0, 128);
  svtkGetMacro(ShaftResolution, int);
  //@}

  //@{
  /**
   * Inverts the arrow direction. When set to true, base is at (1, 0, 0) while the
   * tip is at (0, 0, 0). The default is false, i.e. base at (0, 0, 0) and the tip
   * at (1, 0, 0).
   */
  svtkBooleanMacro(Invert, bool);
  svtkSetMacro(Invert, bool);
  svtkGetMacro(Invert, bool);
  //@}

protected:
  svtkArrowSource();
  ~svtkArrowSource() override {}

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int TipResolution;
  double TipLength;
  double TipRadius;

  int ShaftResolution;
  double ShaftRadius;
  bool Invert;

private:
  svtkArrowSource(const svtkArrowSource&) = delete;
  void operator=(const svtkArrowSource&) = delete;
};

#endif
