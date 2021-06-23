/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmf3Writer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkXdmf3Writer
 * @brief   write eXtensible Data Model and Format files
 *
 * svtkXdmf3Writer converts svtkDataObjects to XDMF format. This is intended to
 * replace svtkXdmfWriter, which is not up to date with the capabilities of the
 * newer XDMF3 library. This writer understands SVTK's composite data types and
 * produces full trees in the output XDMF files.
 */

#ifndef svtkXdmf3Writer_h
#define svtkXdmf3Writer_h

#include "svtkIOXdmf3Module.h" // For export macro

#include "svtkDataObjectAlgorithm.h"

class svtkDoubleArray;

class SVTKIOXDMF3_EXPORT svtkXdmf3Writer : public svtkDataObjectAlgorithm
{
public:
  static svtkXdmf3Writer* New();
  svtkTypeMacro(svtkXdmf3Writer, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set the input data set.
   */
  virtual void SetInputData(svtkDataObject* dobj);

  //@{
  /**
   * Set or get the file name of the xdmf file.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * We never write out ghost cells.  This variable is here to satisfy
   * the behavior of ParaView on invoking a parallel writer.
   */
  void SetGhostLevel(int) {}
  int GetGhostLevel() { return 0; }
  //@}

  /**
   * Write data to output. Method executes subclasses WriteData() method, as
   * well as StartMethod() and EndMethod() methods.
   * Returns 1 on success and 0 on failure.
   */
  virtual int Write();

  //@{
  /**
   * Topology Geometry and Attribute arrays smaller than this are written in line into the XML.
   * Default is 100.
   */
  svtkSetMacro(LightDataLimit, unsigned int);
  svtkGetMacro(LightDataLimit, unsigned int);
  //@}

  //@{
  /**
   * Controls whether writer automatically writes all input time steps, or
   * just the timestep that is currently on the input.
   * Default is OFF.
   */
  svtkSetMacro(WriteAllTimeSteps, bool);
  svtkGetMacro(WriteAllTimeSteps, bool);
  svtkBooleanMacro(WriteAllTimeSteps, bool);
  //@}

protected:
  svtkXdmf3Writer();
  ~svtkXdmf3Writer() override;

  // Overridden to set up automatic loop over time steps.
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  // Overridden to continue automatic loop over time steps.
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  // Write out the input data objects as XDMF and HDF output files.
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char* FileName;
  unsigned int LightDataLimit;
  bool WriteAllTimeSteps;
  int NumberOfProcesses;
  int MyRank;

  svtkDoubleArray* TimeValues;
  svtkDataObject* OriginalInput;
  void WriteDataInternal(svtkInformation* request);
  int CheckParametersInternal(int NumberOfProcesses, int MyRank);
  virtual int CheckParameters();
  // If writing in parallel multiple time steps exchange after each time step
  // if we should continue the execution. Pass local continueExecution as a
  // parameter and return the global continueExecution.
  virtual int GlobalContinueExecuting(int localContinueExecution);

  bool InitWriters;

private:
  svtkXdmf3Writer(const svtkXdmf3Writer&) = delete;
  void operator=(const svtkXdmf3Writer&) = delete;

  class Internals;
  Internals* Internal;
};

#endif /* svtkXdmf3Writer_h */
