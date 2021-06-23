/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageDataStreamer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageDataStreamer
 * @brief   Initiates streaming on image data.
 *
 * To satisfy a request, this filter calls update on its input
 * many times with smaller update extents.  All processing up stream
 * streams smaller pieces.
 */

#ifndef svtkImageDataStreamer_h
#define svtkImageDataStreamer_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingCoreModule.h" // For export macro

class svtkExtentTranslator;

class SVTKIMAGINGCORE_EXPORT svtkImageDataStreamer : public svtkImageAlgorithm
{
public:
  static svtkImageDataStreamer* New();
  svtkTypeMacro(svtkImageDataStreamer, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set how many pieces to divide the input into.
   * void SetNumberOfStreamDivisions(int num);
   * int GetNumberOfStreamDivisions();
   */
  svtkSetMacro(NumberOfStreamDivisions, int);
  svtkGetMacro(NumberOfStreamDivisions, int);
  //@}

  //@{
  /**
   * Get the extent translator that will be used to split the requests
   */
  virtual void SetExtentTranslator(svtkExtentTranslator*);
  svtkGetObjectMacro(ExtentTranslator, svtkExtentTranslator);
  //@}

  // See the svtkAlgorithm for a description of what these do
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkImageDataStreamer();
  ~svtkImageDataStreamer() override;

  svtkExtentTranslator* ExtentTranslator;
  int NumberOfStreamDivisions;
  int CurrentDivision;

private:
  svtkImageDataStreamer(const svtkImageDataStreamer&) = delete;
  void operator=(const svtkImageDataStreamer&) = delete;
};

#endif
