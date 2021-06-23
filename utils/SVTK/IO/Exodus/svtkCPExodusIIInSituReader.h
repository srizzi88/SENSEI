/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCPExodusIIInSituReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkCPExodusIIInSituReader
 * @brief   Read an Exodus II file into data structures
 * that map the raw arrays returned by the Exodus II library into a multi-block
 * data set containing svtkUnstructuredGridBase subclasses.
 *
 *
 * This class can be used to import Exodus II files into SVTK without repacking
 * the data into the standard SVTK memory layout, avoiding the cost of a deep
 * copy.
 */

#ifndef svtkCPExodusIIInSituReader_h
#define svtkCPExodusIIInSituReader_h

#include "svtkIOExodusModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"
#include "svtkNew.h" // For svtkNew
#include <string>   // For std::string
#include <vector>   // For std::vector

class svtkDataArrayCollection;
class svtkPointData;
class svtkPoints;

class SVTKIOEXODUS_EXPORT svtkCPExodusIIInSituReader : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkCPExodusIIInSituReader* New();
  svtkTypeMacro(svtkCPExodusIIInSituReader, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the name of the Exodus file to read.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Get/Set the current timestep to read as a zero-based index.
   */
  svtkGetMacro(CurrentTimeStep, int);
  svtkSetMacro(CurrentTimeStep, int);
  //@}

  //@{
  /**
   * Get the range of timesteps, represented as [0, numTimeSteps - 1]. Call
   * UpdateInformation first to set this without reading any timestep data.
   */
  svtkGetVector2Macro(TimeStepRange, int);
  //@}

  /**
   * Get the floating point tag associated with the timestep at 'step'.
   */
  double GetTimeStepValue(int step) { return TimeSteps.at(step); }

protected:
  svtkCPExodusIIInSituReader();
  ~svtkCPExodusIIInSituReader() override;

  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkCPExodusIIInSituReader(const svtkCPExodusIIInSituReader&) = delete;
  void operator=(const svtkCPExodusIIInSituReader&) = delete;

  bool ExOpen();
  char* FileName;
  int FileId;

  bool ExGetMetaData();
  int NumberOfDimensions;
  int NumberOfNodes;
  int NumberOfElementBlocks;
  std::vector<std::string> NodalVariableNames;
  std::vector<std::string> ElementVariableNames;
  std::vector<int> ElementBlockIds;
  std::vector<double> TimeSteps;
  int TimeStepRange[2];

  bool ExGetCoords();
  svtkNew<svtkPoints> Points;

  bool ExGetNodalVars();
  svtkNew<svtkPointData> PointData;

  bool ExGetElemBlocks();
  svtkNew<svtkMultiBlockDataSet> ElementBlocks;

  void ExClose();

  int CurrentTimeStep;
};

#endif // svtkCPExodusIIInSituReader_h
