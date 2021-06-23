/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractTimeSteps.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractTimeSteps
 * @brief   extract specific time-steps from dataset
 *
 * svtkExtractTimeSteps extracts the specified time steps from the input dataset.
 * It has two modes, one to specify timesteps explicitly by their indices and one
 * to specify a range of timesteps to extract.
 *
 * When specifying timesteps explicitly the timesteps to be extracted are
 * specified by their indices. If no time step is specified, all of the input
 * time steps are extracted.
 *
 * When specifying a range, the beginning and end times are specified and the
 * timesteps in between are extracted.  This can be modified by the TimeStepInterval
 * property that sets the filter to extract every Nth timestep.
 *
 * This filter is useful when one wants to work with only a sub-set of the input
 * time steps.
 */

#ifndef svtkExtractTimeSteps_h
#define svtkExtractTimeSteps_h

#include "svtkFiltersExtractionModule.h" // for export macro
#include "svtkPassInputTypeAlgorithm.h"

#include <set> // for time step indices

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractTimeSteps : public svtkPassInputTypeAlgorithm
{
public:
  svtkTypeMacro(svtkExtractTimeSteps, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkExtractTimeSteps* New();

  /**
   * Get the number of time steps that will be extracted
   */
  int GetNumberOfTimeSteps() const { return static_cast<int>(this->TimeStepIndices.size()); }

  /**
   * Add a time step index. Not added if the index already exists.
   */
  void AddTimeStepIndex(int timeStepIndex);

  //@{
  /**
   * Get/Set an array of time step indices. For the Get function,
   * timeStepIndices should be big enough for GetNumberOfTimeSteps() values.
   */
  void SetTimeStepIndices(int count, const int* timeStepIndices);
  void GetTimeStepIndices(int* timeStepIndices) const;
  //@}

  /**
   * Generate a range of indices in [begin, end) with a step size of 'step'
   */
  void GenerateTimeStepIndices(int begin, int end, int step);

  //@{
  /**
   * Clear the time step indices
   */
  void ClearTimeStepIndices()
  {
    this->TimeStepIndices.clear();
    this->Modified();
  }
  //@}

  //@{
  /**
   * Get/Set whether to extract a range of timesteps.  When false, extracts
   * the time steps explicitly set with SetTimeStepIndices.  Defaults to false.
   */
  svtkGetMacro(UseRange, bool);
  svtkSetMacro(UseRange, bool);
  svtkBooleanMacro(UseRange, bool);
  //@}

  //@{
  /**
   * Get/Set the range of time steps to extract.
   */
  svtkGetVector2Macro(Range, int);
  svtkSetVector2Macro(Range, int);
  //@}

  //@{
  /**
   * Get/Set the time step interval to extract.  This is the N in 'extract every
   * Nth timestep in this range'.  Default to 1 or 'extract all timesteps in this range.
   */
  svtkGetMacro(TimeStepInterval, int);
  svtkSetClampMacro(TimeStepInterval, int, 1, SVTK_INT_MAX);
  //@}

  // What timestep to provide when the requested time is between the timesteps
  // the filter is set to extract
  enum
  {
    PREVIOUS_TIMESTEP, // floor the time to the previous timestep
    NEXT_TIMESTEP,     // ceiling the time to the next timestep
    NEAREST_TIMESTEP   // take the timestep whose absolute difference from the requested time is
                       // smallest
  } EstimationMode;
  //@{
  /**
   * Get/Set what to do when the requested time is not one of the timesteps this filter
   * is set to extract.  Should be one of the values of the enum
   * svtkExtractTimeSteps::EstimationMode. The default is PREVIOUS_TIMESTEP.
   */
  svtkGetMacro(TimeEstimationMode, int);
  svtkSetMacro(TimeEstimationMode, int);
  void SetTimeEstimationModeToPrevious() { this->SetTimeEstimationMode(PREVIOUS_TIMESTEP); }
  void SetTimeEstimationModeToNext() { this->SetTimeEstimationMode(NEXT_TIMESTEP); }
  void SetTimeEstimationModeToNearest() { this->SetTimeEstimationMode(NEAREST_TIMESTEP); }
  //@}

protected:
  svtkExtractTimeSteps();
  ~svtkExtractTimeSteps() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  std::set<int> TimeStepIndices;
  bool UseRange;
  int Range[2];
  int TimeStepInterval;
  int TimeEstimationMode;

private:
  svtkExtractTimeSteps(const svtkExtractTimeSteps&) = delete;
  void operator=(const svtkExtractTimeSteps&) = delete;
};

#endif // svtkExtractTimeSteps_h
