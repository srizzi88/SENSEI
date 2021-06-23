/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageToPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageToPoints
 * @brief   Extract all image voxels as points.
 *
 * This filter takes an input image and an optional stencil, and creates
 * a svtkPolyData that contains the points and the point attributes but no
 * cells.  If a stencil is provided, only the points inside the stencil
 * are included.
 * @par Thanks:
 * Thanks to David Gobbi, Calgary Image Processing and Analysis Centre,
 * University of Calgary, for providing this class.
 */

#ifndef svtkImageToPoints_h
#define svtkImageToPoints_h

#include "svtkImagingHybridModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkImageStencilData;

class SVTKIMAGINGHYBRID_EXPORT svtkImageToPoints : public svtkPolyDataAlgorithm
{
public:
  static svtkImageToPoints* New();
  svtkTypeMacro(svtkImageToPoints, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Only extract the points that lie within the stencil.
   */
  void SetStencilConnection(svtkAlgorithmOutput* port);
  svtkAlgorithmOutput* GetStencilConnection();
  void SetStencilData(svtkImageStencilData* stencil);
  //@}

  //@{
  /**
   * Set the desired precision for the output points.
   * See svtkAlgorithm::DesiredOutputPrecision for the available choices.
   * The default is double precision.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkImageToPoints();
  ~svtkImageToPoints() override;

  int RequestInformation(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo) override;

  int RequestUpdateExtent(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo) override;

  int RequestData(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  int OutputPointsPrecision;

private:
  svtkImageToPoints(const svtkImageToPoints&) = delete;
  void operator=(const svtkImageToPoints&) = delete;
};

#endif
