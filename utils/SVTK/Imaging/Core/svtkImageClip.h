/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageClip.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageClip
 * @brief   Reduces the image extent of the input.
 *
 * svtkImageClip  will make an image smaller.  The output must have
 * an image extent which is the subset of the input.  The filter has two
 * modes of operation:
 * 1: By default, the data is not copied in this filter.
 * Only the whole extent is modified.
 * 2: If ClipDataOn is set, then you will get no more that the clipped
 * extent.
 */

#ifndef svtkImageClip_h
#define svtkImageClip_h

// I did not make this a subclass of in place filter because
// the references on the data do not matter. I make no modifications
// to the data.
#include "svtkImageAlgorithm.h"
#include "svtkImagingCoreModule.h" // For export macro

class SVTKIMAGINGCORE_EXPORT svtkImageClip : public svtkImageAlgorithm
{
public:
  static svtkImageClip* New();
  svtkTypeMacro(svtkImageClip, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The whole extent of the output has to be set explicitly.
   */
  void SetOutputWholeExtent(int extent[6], svtkInformation* outInfo = nullptr);
  void SetOutputWholeExtent(int minX, int maxX, int minY, int maxY, int minZ, int maxZ);
  void GetOutputWholeExtent(int extent[6]);
  int* GetOutputWholeExtent() SVTK_SIZEHINT(6) { return this->OutputWholeExtent; }
  //@}

  void ResetOutputWholeExtent();

  //@{
  /**
   * By default, ClipData is off, and only the WholeExtent is modified.
   * the data's extent may actually be larger.  When this flag is on,
   * the data extent will be no more than the OutputWholeExtent.
   */
  svtkSetMacro(ClipData, svtkTypeBool);
  svtkGetMacro(ClipData, svtkTypeBool);
  svtkBooleanMacro(ClipData, svtkTypeBool);
  //@}

protected:
  svtkImageClip();
  ~svtkImageClip() override {}

  // Time when OutputImageExtent was computed.
  svtkTimeStamp CTime;
  int Initialized; // Set the OutputImageExtent for the first time.
  int OutputWholeExtent[6];

  svtkTypeBool ClipData;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void CopyData(svtkImageData* inData, svtkImageData* outData, int* ext);

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkImageClip(const svtkImageClip&) = delete;
  void operator=(const svtkImageClip&) = delete;
};

#endif
