/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVPICReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVPICReader
 * @brief   class for reading VPIC data files
 *
 * svtkDataReader is a helper superclass that reads the svtk data file header,
 * dataset type, and attribute data (point and cell attributes such as
 * scalars, vectors, normals, etc.) from a svtk data file.  See text for
 * the format of the various svtk file types.
 *
 * @sa
 * svtkPolyDataReader svtkStructuredPointsReader svtkStructuredGridReader
 * svtkUnstructuredGridReader svtkRectilinearGridReader
 */

#ifndef svtkVPICReader_h
#define svtkVPICReader_h

#include "svtkIOVPICModule.h" // For export macro
#include "svtkImageAlgorithm.h"

class svtkCallbackCommand;
class svtkDataArraySelection;
class svtkFloatArray;
class svtkStdString;
class svtkMultiProcessController;
class svtkInformation;

class VPICDataSet;
class GridExchange;

class SVTKIOVPIC_EXPORT svtkVPICReader : public svtkImageAlgorithm
{
public:
  static svtkVPICReader* New();
  svtkTypeMacro(svtkVPICReader, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify file name of VPIC data file to read.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Set the stride in each dimension
   */
  svtkSetVector3Macro(Stride, int);
  svtkGetVector3Macro(Stride, int);
  //@}

  //@{
  /**
   * Set the simulation file decomposition in each dimension
   */
  svtkSetVector2Macro(XExtent, int);
  svtkSetVector2Macro(YExtent, int);
  svtkSetVector2Macro(ZExtent, int);
  //@}

  // Get the full layout size in files for setting the range in GUI
  svtkGetVector2Macro(XLayout, int);
  svtkGetVector2Macro(YLayout, int);
  svtkGetVector2Macro(ZLayout, int);

  //@{
  /**
   * Get the reader's output
   */
  svtkImageData* GetOutput();
  svtkImageData* GetOutput(int index);
  //@}

  //@{
  /**
   * The following methods allow selective reading of solutions fields.
   * By default, ALL data fields on the nodes are read, but this can
   * be modified.
   */
  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int index);
  int GetPointArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  void DisableAllPointArrays();
  void EnableAllPointArrays();
  //@}

protected:
  svtkVPICReader();
  ~svtkVPICReader() override;

  char* FileName; // First field part file giving path

  int Rank;      // Number of this processor
  int TotalRank; // Number of processors
  int UsedRank;  // Number of processors used in display

  VPICDataSet* vpicData;   // Data structure controlling access
  GridExchange* exchanger; // Exchange ghost cells between procs

  svtkIdType NumberOfNodes;  // Number of points in grid
  svtkIdType NumberOfCells;  // Number of cells in grid
  svtkIdType NumberOfTuples; // Number of tuples in sub extent

  int WholeExtent[6];  // Problem image extent
  int SubExtent[6];    // Processor problem extent
  int Dimension[3];    // Size of image
  int SubDimension[3]; // Size of subextent of image
  int XLayout[2];      // Extent in complete files
  int YLayout[2];      // Extent in complete files
  int ZLayout[2];      // Extent in complete files

  int NumberOfVariables;      // Number of variables to display
  svtkStdString* VariableName; // Names of each variable
  int* VariableStruct;        // Scalar, vector or tensor

  int NumberOfTimeSteps; // Temporal domain
  double* TimeSteps;     // Times available for request
  int CurrentTimeStep;   // Time currently displayed

  int Stride[3];  // Stride over actual data
  int XExtent[2]; // Subview extent in files
  int YExtent[2]; // Subview extent in files
  int ZExtent[2]; // Subview extent in files

  svtkFloatArray** data; // Actual data arrays
  int* dataLoaded;      // Data is loaded for current time

  int Start[3];            // Start offset for processor w ghosts
  int GhostDimension[3];   // Dimension including ghosts on proc
  int NumberOfGhostTuples; // Total ghost cells per component
  int ghostLevel0;         // Left plane number of ghosts
  int ghostLevel1;         // Right plane number of ghosts

  // Controls initializing and querrying MPI
  svtkMultiProcessController* MPIController;

  // Selected field of interest
  svtkDataArraySelection* PointDataArraySelection;

  // Observer to modify this object when array selections are modified
  svtkCallbackCommand* SelectionObserver;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(
    svtkInformation*, svtkInformationVector** inVector, svtkInformationVector*) override;

  void LoadVariableData(int var, int timeStep);
  void LoadComponent(float* varData, float* block, int comp, int numberOfComponents);

  static void SelectionCallback(
    svtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void EventCallback(svtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

private:
  svtkVPICReader(const svtkVPICReader&) = delete;
  void operator=(const svtkVPICReader&) = delete;
};

#endif
