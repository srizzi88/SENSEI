/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractVOI.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkmExtractVOI
 * @brief   select piece (e.g., volume of interest) and/or subsample structured points dataset
 *
 * svtkmExtractVOI is a filter that selects a portion of an input structured
 * points dataset, or subsamples an input dataset. (The selected portion of
 * interested is referred to as the Volume Of Interest, or VOI.) The output of
 * this filter is a structured points dataset. The filter treats input data
 * of any topological dimension (i.e., point, line, image, or volume) and can
 * generate output data of any topological dimension.
 *
 * To use this filter set the VOI ivar which are i-j-k min/max indices that
 * specify a rectangular region in the data. (Note that these are 0-offset.)
 * You can also specify a sampling rate to subsample the data.
 *
 * Typical applications of this filter are to extract a slice from a volume
 * for image processing, subsampling large volumes to reduce data size, or
 * extracting regions of a volume with interesting data.
 *
 */
#ifndef svtkmExtractVOI_h
#define svtkmExtractVOI_h

#include "svtkAcceleratorsSVTKmModule.h" // for export macro
#include "svtkExtractVOI.h"

class SVTKACCELERATORSSVTKM_EXPORT svtkmExtractVOI : public svtkExtractVOI
{
public:
  svtkTypeMacro(svtkmExtractVOI, svtkExtractVOI);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkmExtractVOI* New();

protected:
  svtkmExtractVOI();
  ~svtkmExtractVOI() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkmExtractVOI(const svtkmExtractVOI&) = delete;
  void operator=(const svtkmExtractVOI&) = delete;
};

#endif // svtkmExtractVOI_h
// SVTK-HeaderTest-Exclude: svtkmExtractVOI.h
