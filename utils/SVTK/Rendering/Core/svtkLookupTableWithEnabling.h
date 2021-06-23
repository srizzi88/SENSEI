/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLookupTableWithEnabling.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLookupTableWithEnabling
 * @brief   A lookup table that allows for an
 * optional array to be provided that specifies which scalars to "enable" and
 * which to "disable".
 *
 *
 * svtkLookupTableWithEnabling "disables" or "grays out" output colors
 * based on whether the given value in EnabledArray is "0" or not.
 *
 *
 * @warning
 * You must set the EnabledArray before MapScalars() is called.
 * Indices of EnabledArray must map directly to those of the array passed
 * to MapScalars().
 *
 */

#ifndef svtkLookupTableWithEnabling_h
#define svtkLookupTableWithEnabling_h

#include "svtkLookupTable.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkDataArray;

class SVTKRENDERINGCORE_EXPORT svtkLookupTableWithEnabling : public svtkLookupTable
{
public:
  static svtkLookupTableWithEnabling* New();

  svtkTypeMacro(svtkLookupTableWithEnabling, svtkLookupTable);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * This must be set before MapScalars() is called.
   * Indices of this array must map directly to those in the scalars array
   * passed to MapScalars().
   * Values of 0 in the array indicate the color should be desaturatated.
   */
  svtkGetObjectMacro(EnabledArray, svtkDataArray);
  virtual void SetEnabledArray(svtkDataArray* enabledArray);
  //@}

  /**
   * Map a set of scalars through the lookup table.
   */
  void MapScalarsThroughTable2(void* input, unsigned char* output, int inputDataType,
    int numberOfValues, int inputIncrement, int outputIncrement) override;

  /**
   * A convenience method for taking a color and desaturating it.
   */
  virtual void DisableColor(unsigned char r, unsigned char g, unsigned char b, unsigned char* rd,
    unsigned char* gd, unsigned char* bd);

protected:
  svtkLookupTableWithEnabling(int sze = 256, int ext = 256);
  ~svtkLookupTableWithEnabling() override;

  svtkDataArray* EnabledArray;

private:
  svtkLookupTableWithEnabling(const svtkLookupTableWithEnabling&) = delete;
  void operator=(const svtkLookupTableWithEnabling&) = delete;
};

#endif
