/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHedgeHog.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHedgeHog
 * @brief   create oriented lines from vector data
 *
 * svtkHedgeHog creates oriented lines from the input data set. Line
 * length is controlled by vector (or normal) magnitude times scale
 * factor. If VectorMode is UseNormal, normals determine the orientation
 * of the lines. Lines are colored by scalar data, if available.
 */

#ifndef svtkHedgeHog_h
#define svtkHedgeHog_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_USE_VECTOR 0
#define SVTK_USE_NORMAL 1

class SVTKFILTERSCORE_EXPORT svtkHedgeHog : public svtkPolyDataAlgorithm
{
public:
  static svtkHedgeHog* New();
  svtkTypeMacro(svtkHedgeHog, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set scale factor to control size of oriented lines.
   */
  svtkSetMacro(ScaleFactor, double);
  svtkGetMacro(ScaleFactor, double);
  //@}

  //@{
  /**
   * Specify whether to use vector or normal to perform vector operations.
   */
  svtkSetMacro(VectorMode, int);
  svtkGetMacro(VectorMode, int);
  void SetVectorModeToUseVector() { this->SetVectorMode(SVTK_USE_VECTOR); }
  void SetVectorModeToUseNormal() { this->SetVectorMode(SVTK_USE_NORMAL); }
  const char* GetVectorModeAsString();
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkHedgeHog();
  ~svtkHedgeHog() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  double ScaleFactor;
  int VectorMode; // Orient/scale via normal or via vector data
  int OutputPointsPrecision;

private:
  svtkHedgeHog(const svtkHedgeHog&) = delete;
  void operator=(const svtkHedgeHog&) = delete;
};

//@{
/**
 * Return the vector mode as a character string.
 */
inline const char* svtkHedgeHog::GetVectorModeAsString(void)
{
  if (this->VectorMode == SVTK_USE_VECTOR)
  {
    return "UseVector";
  }
  else if (this->VectorMode == SVTK_USE_NORMAL)
  {
    return "UseNormal";
  }
  else
  {
    return "Unknown";
  }
}
#endif
//@}
