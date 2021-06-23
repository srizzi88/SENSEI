/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWarpLens.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWarpLens
 * @brief   deform geometry by applying lens distortion
 *
 * svtkWarpLens is a filter that modifies point coordinates by moving
 * in accord with a lens distortion model.
 */

#ifndef svtkWarpLens_h
#define svtkWarpLens_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPointSetAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkWarpLens : public svtkPointSetAlgorithm
{
public:
  static svtkWarpLens* New();
  svtkTypeMacro(svtkWarpLens, svtkPointSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify second order symmetric radial lens distortion parameter.
   * This is obsoleted by newer instance variables.
   */
  void SetKappa(double kappa);
  double GetKappa();
  //@}

  //@{
  /**
   * Specify the center of radial distortion in pixels.
   * This is obsoleted by newer instance variables.
   */
  void SetCenter(double centerX, double centerY);
  double* GetCenter() SVTK_SIZEHINT(2);
  //@}

  //@{
  /**
   * Specify the calibrated principal point of the camera/lens
   */
  svtkSetVector2Macro(PrincipalPoint, double);
  svtkGetVectorMacro(PrincipalPoint, double, 2);
  //@}

  //@{
  /**
   * Specify the symmetric radial distortion parameters for the lens
   */
  svtkSetMacro(K1, double);
  svtkGetMacro(K1, double);
  svtkSetMacro(K2, double);
  svtkGetMacro(K2, double);
  //@}

  //@{
  /**
   * Specify the decentering distortion parameters for the lens
   */
  svtkSetMacro(P1, double);
  svtkGetMacro(P1, double);
  svtkSetMacro(P2, double);
  svtkGetMacro(P2, double);
  //@}

  //@{
  /**
   * Specify the imager format width / height in mm
   */
  svtkSetMacro(FormatWidth, double);
  svtkGetMacro(FormatWidth, double);
  svtkSetMacro(FormatHeight, double);
  svtkGetMacro(FormatHeight, double);
  //@}

  //@{
  /**
   * Specify the image width / height in pixels
   */
  svtkSetMacro(ImageWidth, int);
  svtkGetMacro(ImageWidth, int);
  svtkSetMacro(ImageHeight, int);
  svtkGetMacro(ImageHeight, int);
  //@}

  int FillInputPortInformation(int port, svtkInformation* info) override;

protected:
  svtkWarpLens();
  ~svtkWarpLens() override {}

  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double PrincipalPoint[2]; // The calibrated principal point of camera/lens in mm
  double K1;                // Symmetric radial distortion parameters
  double K2;
  double P1; // Decentering distortion parameters
  double P2;
  double FormatWidth;  // imager format width in mm
  double FormatHeight; // imager format height in mm
  int ImageWidth;      // image width in pixels
  int ImageHeight;     // image height in pixels
private:
  svtkWarpLens(const svtkWarpLens&) = delete;
  void operator=(const svtkWarpLens&) = delete;
};

#endif
