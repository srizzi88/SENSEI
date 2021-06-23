/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTemporalPathLineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTemporalPathLineFilter
 * @brief   Generate a Polydata Pointset from any Dataset.
 *
 *
 * svtkTemporalPathLineFilter takes any dataset as input, it extracts the point
 * locations of all cells over time to build up a polyline trail.
 * The point number (index) is used as the 'key' if the points are randomly
 * changing their respective order in the points list, then you should specify
 * a scalar that represents the unique ID. This is intended to handle the output
 * of a filter such as the svtkParticleTracer.
 *
 * @sa
 * svtkParticleTracer
 *
 * @par Thanks:
 * John Bidiscombe of
 * CSCS - Swiss National Supercomputing Centre
 * for creating and contributing this class.
 */

#ifndef svtkTemporalPathLineFilter_h
#define svtkTemporalPathLineFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkPoints;
class svtkCellArray;
class svtkMergePoints;
class svtkFloatArray;

#include "svtkSmartPointer.h" // for memory safety
#include <set>               // Because we want to use it
class ParticleTrail;
class svtkTemporalPathLineFilterInternals;
typedef svtkSmartPointer<ParticleTrail> TrailPointer;

class SVTKFILTERSGENERAL_EXPORT svtkTemporalPathLineFilter : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard Type-Macro
   */
  static svtkTemporalPathLineFilter* New();
  svtkTypeMacro(svtkTemporalPathLineFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Set the number of particles to track as a ratio of the input
   * example: setting MaskPoints to 10 will track every 10th point
   */
  svtkSetMacro(MaskPoints, int);
  svtkGetMacro(MaskPoints, int);
  //@}

  //@{
  /**
   * If the Particles being traced animate for a long time, the
   * trails or traces will become long and stringy. Setting
   * the MaxTraceTimeLength will limit how much of the trace
   * is displayed. Tracks longer then the Max will disappear
   * and the trace will appear like a snake of fixed length
   * which progresses as the particle moves
   */
  svtkSetMacro(MaxTrackLength, unsigned int);
  svtkGetMacro(MaxTrackLength, unsigned int);
  //@}

  //@{
  /**
   * Specify the name of a scalar array which will be used to fetch
   * the index of each point. This is necessary only if the particles
   * change position (Id order) on each time step. The Id can be used
   * to identify particles at each step and hence track them properly.
   * If this array is nullptr, the global point ids are used.  If an Id
   * array cannot otherwise be found, the point index is used as the ID.
   */
  svtkSetStringMacro(IdChannelArray);
  svtkGetStringMacro(IdChannelArray);
  //@}

  //@{
  /**
   * If a particle disappears from one end of a simulation and reappears
   * on the other side, the track left will be unrepresentative.
   * Set a MaxStepDistance{x,y,z} which acts as a threshold above which
   * if a step occurs larger than the value (for the dimension), the track will
   * be dropped and restarted after the step. (ie the part before the wrap
   * around will be dropped and the newer part kept).
   */
  svtkSetVector3Macro(MaxStepDistance, double);
  svtkGetVector3Macro(MaxStepDistance, double);
  //@}

  //@{
  /**
   * When a particle 'disappears', the trail belonging to it is removed from
   * the list. When this flag is enabled, dead trails will persist
   * until the next time the list is cleared. Use carefully as it may cause
   * excessive memory consumption if left on by mistake.
   */
  svtkSetMacro(KeepDeadTrails, int);
  svtkGetMacro(KeepDeadTrails, int);
  //@}

  /**
   * Flush will wipe any existing data so that traces can be restarted from
   * whatever time step is next supplied.
   */
  void Flush();

  /**
   * Set a second input which is a selection. Particles with the same
   * Id in the selection as the primary input will be chosen for pathlines
   * Note that you must have the same IdChannelArray in the selection as the input
   */
  void SetSelectionConnection(svtkAlgorithmOutput* algOutput);

  /**
   * Set a second input which is a selection. Particles with the same
   * Id in the selection as the primary input will be chosen for pathlines
   * Note that you must have the same IdChannelArray in the selection as the input
   */
  void SetSelectionData(svtkDataSet* input);

protected:
  svtkTemporalPathLineFilter();
  ~svtkTemporalPathLineFilter() override;

  //
  // Make sure the pipeline knows what type we expect as input
  //
  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  //@{
  /**
   * The necessary parts of the standard pipeline update mechanism
   */
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  //
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  //@}

  TrailPointer GetTrail(svtkIdType i);
  void IncrementTrail(TrailPointer trail, svtkDataSet* input, svtkIdType i);

  // internal data variables
  int NumberOfTimeSteps;
  int MaskPoints;
  unsigned int MaxTrackLength;
  unsigned int LastTrackLength;
  int FirstTime;
  char* IdChannelArray;
  double MaxStepDistance[3];
  double LatestTime;
  int KeepDeadTrails;
  int UsingSelection;
  //

  svtkSmartPointer<svtkCellArray> PolyLines;
  svtkSmartPointer<svtkCellArray> Vertices;
  svtkSmartPointer<svtkPoints> LineCoordinates;
  svtkSmartPointer<svtkPoints> VertexCoordinates;
  svtkSmartPointer<svtkFloatArray> TrailId;
  svtkSmartPointer<svtkTemporalPathLineFilterInternals> Internals;
  std::set<svtkIdType> SelectionIds;

  //
private:
  svtkTemporalPathLineFilter(const svtkTemporalPathLineFilter&) = delete;
  void operator=(const svtkTemporalPathLineFilter&) = delete;
};

#endif
