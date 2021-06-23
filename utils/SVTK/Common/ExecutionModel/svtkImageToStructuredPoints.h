/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageToStructuredPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageToStructuredPoints
 * @brief   Attaches image pipeline to SVTK.
 *
 * svtkImageToStructuredPoints changes an image cache format to
 * a structured points dataset.  It takes an Input plus an optional
 * VectorInput. The VectorInput converts the RGB scalar components
 * of the VectorInput to vector pointdata attributes. This filter
 * will try to reference count the data but in some cases it must
 * make a copy.
 */

#ifndef svtkImageToStructuredPoints_h
#define svtkImageToStructuredPoints_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkImageAlgorithm.h"

class svtkImageData;
class svtkStructuredPoints;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkImageToStructuredPoints : public svtkImageAlgorithm
{
public:
  static svtkImageToStructuredPoints* New();
  svtkTypeMacro(svtkImageToStructuredPoints, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the input object from the image pipeline.
   */
  void SetVectorInputData(svtkImageData* input);
  svtkImageData* GetVectorInput();
  //@}

  /**
   * Get the output of the filter.
   */
  svtkStructuredPoints* GetStructuredPointsOutput();

protected:
  svtkImageToStructuredPoints();
  ~svtkImageToStructuredPoints() override;

  // to translate the wholeExtent to have min 0 ( I do not like this hack).
  int Translate[3];

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillOutputPortInformation(int, svtkInformation*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

private:
  svtkImageToStructuredPoints(const svtkImageToStructuredPoints&) = delete;
  void operator=(const svtkImageToStructuredPoints&) = delete;
};

#endif
