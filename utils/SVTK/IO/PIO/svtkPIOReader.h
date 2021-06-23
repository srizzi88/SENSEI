/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPIOReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPIOReader
 * @brief   class for reading PIO (Parallel Input Output) data files
 *
 * This class reads in dump files generated from xRage, a LANL physics code.
 * The PIO (Parallel Input Output) library is used to create the dump files.
 *
 * @sa
 * svtkMultiBlockReader
 */

#ifndef svtkPIOReader_h
#define svtkPIOReader_h

#include "svtkIOPIOModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

class svtkCallbackCommand;
class svtkDataArraySelection;
class svtkFloatArray;
class svtkInformation;
class svtkMultiBlockDataSet;
class svtkMultiProcessController;
class svtkStdString;

class PIOAdaptor;
class PIO_DATA;

class SVTKIOPIO_EXPORT svtkPIOReader : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkPIOReader* New();
  svtkTypeMacro(svtkPIOReader, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify file name of PIO data file to read.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specify the timestep to be loaded
   */
  svtkSetMacro(CurrentTimeStep, int);
  svtkGetMacro(CurrentTimeStep, int);
  //@}

  //@{
  /**
   * Specify the creation of hypertree grid
   */
  svtkGetMacro(HyperTreeGrid, bool);
  svtkSetMacro(HyperTreeGrid, bool);
  //@}

  //@{
  /**
   * Specify the creation of tracer data
   */
  svtkSetMacro(Tracers, bool);
  svtkGetMacro(Tracers, bool);
  //@}

  //@{
  /**
   * Specify the use of float64 for data
   */
  svtkSetMacro(Float64, bool);
  svtkGetMacro(Float64, bool);
  //@}

  //@{
  /**
   * Get the reader's output
   */
  svtkMultiBlockDataSet* GetOutput();
  svtkMultiBlockDataSet* GetOutput(int index);
  //@}

  //@{
  /**
   * The following methods allow selective reading of solutions fields.
   * By default, ALL data fields on the nodes are read, but this can
   * be modified.
   */
  int GetNumberOfCellArrays();
  const char* GetCellArrayName(int index);
  int GetCellArrayStatus(const char* name);
  void SetCellArrayStatus(const char* name, int status);
  void DisableAllCellArrays();
  void EnableAllCellArrays();
  //@}

protected:
  svtkPIOReader();
  ~svtkPIOReader() override;

  char* FileName; // First field part file giving path

  int Rank;      // Number of this processor
  int TotalRank; // Number of processors

  PIOAdaptor* pioAdaptor; // Adapts data format to SVTK

  int NumberOfVariables; // Number of variables to display

  int NumberOfTimeSteps; // Temporal domain
  double* TimeSteps;     // Times available for request
  int CurrentTimeStep;   // Time currently displayed
  int LastTimeStep;      // Last time displayed

  bool HyperTreeGrid; // Create HTG rather than UnstructuredGrid
  bool Tracers;       // Create UnstructuredGrid for tracer info
  bool Float64;       // Load variable data as 64 bit float

  // Controls initializing and querrying MPI
  svtkMultiProcessController* MPIController;

  // Selected field of interest
  svtkDataArraySelection* CellDataArraySelection;

  // Observer to modify this object when array selections are modified
  svtkCallbackCommand* SelectionObserver;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(
    svtkInformation*, svtkInformationVector** inVector, svtkInformationVector*) override;

  static void SelectionModifiedCallback(
    svtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

private:
  svtkPIOReader(const svtkPIOReader&) = delete;
  void operator=(const svtkPIOReader&) = delete;
};

#endif
