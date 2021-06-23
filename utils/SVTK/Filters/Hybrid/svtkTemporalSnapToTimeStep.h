/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTemporalSnapToTimeStep.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTemporalSnapToTimeStep
 * @brief   modify the time range/steps of temporal data
 *
 * svtkTemporalSnapToTimeStep  modify the time range or time steps of
 * the data without changing the data itself. The data is not resampled
 * by this filter, only the information accompanying the data is modified.
 *
 * @par Thanks:
 * John Bidiscombe of CSCS - Swiss National Supercomputing Centre
 * for creating and contributing this class.
 * For related material, please refer to :
 * John Biddiscombe, Berk Geveci, Ken Martin, Kenneth Moreland, David Thompson,
 * "Time Dependent Processing in a Parallel Pipeline Architecture",
 * IEEE Visualization 2007.
 */

#ifndef svtkTemporalSnapToTimeStep_h
#define svtkTemporalSnapToTimeStep_h

#include "svtkFiltersHybridModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"

#include <vector> // used because I am a bad boy. So there.

class SVTKFILTERSHYBRID_EXPORT svtkTemporalSnapToTimeStep : public svtkPassInputTypeAlgorithm
{
public:
  static svtkTemporalSnapToTimeStep* New();
  svtkTypeMacro(svtkTemporalSnapToTimeStep, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum
  {
    SVTK_SNAP_NEAREST = 0,
    SVTK_SNAP_NEXTBELOW_OR_EQUAL,
    SVTK_SNAP_NEXTABOVE_OR_EQUAL
  };

  svtkSetMacro(SnapMode, int);
  svtkGetMacro(SnapMode, int);
  void SetSnapModeToNearest() { this->SetSnapMode(SVTK_SNAP_NEAREST); }
  void SetSnapModeToNextBelowOrEqual() { this->SetSnapMode(SVTK_SNAP_NEXTBELOW_OR_EQUAL); }
  void SetSnapModeToNextAboveOrEqual() { this->SetSnapMode(SVTK_SNAP_NEXTABOVE_OR_EQUAL); }

protected:
  svtkTemporalSnapToTimeStep();
  ~svtkTemporalSnapToTimeStep() override;

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestUpdateExtent(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  std::vector<double> InputTimeValues;
  int HasDiscrete;
  int SnapMode;

private:
  svtkTemporalSnapToTimeStep(const svtkTemporalSnapToTimeStep&) = delete;
  void operator=(const svtkTemporalSnapToTimeStep&) = delete;
};

#endif
