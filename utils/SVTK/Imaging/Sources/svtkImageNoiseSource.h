/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageNoiseSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageNoiseSource
 * @brief   Create an image filled with noise.
 *
 * svtkImageNoiseSource just produces images filled with noise.  The only
 * option now is uniform noise specified by a min and a max.  There is one
 * major problem with this source. Every time it executes, it will output
 * different pixel values.  This has important implications when a stream
 * requests overlapping regions.  The same pixels will have different values
 * on different updates.
 */

#ifndef svtkImageNoiseSource_h
#define svtkImageNoiseSource_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingSourcesModule.h" // For export macro

class SVTKIMAGINGSOURCES_EXPORT svtkImageNoiseSource : public svtkImageAlgorithm
{
public:
  static svtkImageNoiseSource* New();
  svtkTypeMacro(svtkImageNoiseSource, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the minimum and maximum values for the generated noise.
   */
  svtkSetMacro(Minimum, double);
  svtkGetMacro(Minimum, double);
  svtkSetMacro(Maximum, double);
  svtkGetMacro(Maximum, double);
  //@}

  //@{
  /**
   * Set how large of an image to generate.
   */
  void SetWholeExtent(int xMinx, int xMax, int yMin, int yMax, int zMin, int zMax);
  void SetWholeExtent(const int ext[6])
  {
    this->SetWholeExtent(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
  }
  //@}

protected:
  svtkImageNoiseSource();
  ~svtkImageNoiseSource() override {}

  double Minimum;
  double Maximum;
  int WholeExtent[6];

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void ExecuteDataWithInformation(svtkDataObject* data, svtkInformation* outInfo) override;

private:
  svtkImageNoiseSource(const svtkImageNoiseSource&) = delete;
  void operator=(const svtkImageNoiseSource&) = delete;
};

#endif
