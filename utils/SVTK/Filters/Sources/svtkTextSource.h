/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTextSource
 * @brief   create polygonal text
 *
 * svtkTextSource converts a text string into polygons.  This way you can
 * insert text into your renderings. It uses the 9x15 font from X Windows.
 * You can specify if you want the background to be drawn or not. The
 * characters are formed by scan converting the raster font into
 * quadrilaterals. Colors are assigned to the letters using scalar data.
 * To set the color of the characters with the source's actor property, set
 * BackingOff on the text source and ScalarVisibilityOff on the associated
 * svtkPolyDataMapper. Then, the color can be set using the associated actor's
 * property.
 *
 * svtkVectorText generates higher quality polygonal representations of
 * characters.
 *
 * @sa
 * svtkVectorText
 */

#ifndef svtkTextSource_h
#define svtkTextSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSSOURCES_EXPORT svtkTextSource : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkTextSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with no string set and backing enabled.
   */
  static svtkTextSource* New();

  //@{
  /**
   * Set/Get the text to be drawn.
   */
  svtkSetStringMacro(Text);
  svtkGetStringMacro(Text);
  //@}

  //@{
  /**
   * Controls whether or not a background is drawn with the text.
   */
  svtkSetMacro(Backing, svtkTypeBool);
  svtkGetMacro(Backing, svtkTypeBool);
  svtkBooleanMacro(Backing, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the foreground color. Default is white (1,1,1). ALpha is always 1.
   */
  svtkSetVector3Macro(ForegroundColor, double);
  svtkGetVectorMacro(ForegroundColor, double, 3);
  //@}

  //@{
  /**
   * Set/Get the background color. Default is black (0,0,0). Alpha is always 1.
   */
  svtkSetVector3Macro(BackgroundColor, double);
  svtkGetVectorMacro(BackgroundColor, double, 3);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * svtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * svtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkTextSource();
  ~svtkTextSource() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  char* Text;
  svtkTypeBool Backing;
  double ForegroundColor[4];
  double BackgroundColor[4];
  int OutputPointsPrecision;

private:
  svtkTextSource(const svtkTextSource&) = delete;
  void operator=(const svtkTextSource&) = delete;
};

#endif
