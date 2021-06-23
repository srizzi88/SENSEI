/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtPkNetCDFPOPReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPNetCDFPOPReader
 * @brief   read NetCDF files in parallel with MPI
 * .Author Ross Miller 03.14.2011
 *
 * svtkNetCDFPOPReader is a source object that reads NetCDF files.
 * It should be able to read most any NetCDF file that wants to output a
 * rectilinear grid.  The ordering of the variables is changed such that
 * the NetCDF x, y, z directions correspond to the svtkRectilinearGrid
 * z, y, x directions, respectively.  The striding is done with
 * respect to the svtkRectilinearGrid ordering.  Additionally, the
 * z coordinates of the svtkRectilinearGrid are negated so that the
 * first slice/plane has the highest z-value and the last slice/plane
 * has the lowest z-value.
 */

#ifndef svtkPNetCDFPOPReader_h
#define svtkPNetCDFPOPReader_h

#include "svtkIOParallelNetCDFModule.h" // For export macro
#include "svtkRectilinearGridAlgorithm.h"

class svtkDataArraySelection;
class svtkCallbackCommand;
class svtkMPIController;
class svtkPNetCDFPOPReaderInternal;

class SVTKIOPARALLELNETCDF_EXPORT svtkPNetCDFPOPReader : public svtkRectilinearGridAlgorithm
{
public:
  svtkTypeMacro(svtkPNetCDFPOPReader, svtkRectilinearGridAlgorithm);
  static svtkPNetCDFPOPReader* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The file to open
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Enable subsampling in i,j and k dimensions in the svtkRectilinearGrid
   */
  svtkSetVector3Macro(Stride, int);
  svtkGetVector3Macro(Stride, int);
  //@}

  //@{
  /**
   * Variable array selection.
   */
  virtual int GetNumberOfVariableArrays();
  virtual const char* GetVariableArrayName(int idx);
  virtual int GetVariableArrayStatus(const char* name);
  virtual void SetVariableArrayStatus(const char* name, int status);
  //@}

  /**
   * Set ranks that will actually open and read the netCDF files.  Pass in
   * null to chose reasonable defaults)
   */
  void SetReaderRanks(svtkIdList*);

  // Set/Get the svtkMultiProcessController which will handle communications
  // for the parallel rendering.
  svtkGetObjectMacro(Controller, svtkMPIController);
  void SetController(svtkMPIController* controller);

protected:
  svtkPNetCDFPOPReader();
  ~svtkPNetCDFPOPReader() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  virtual int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  // Helper function for RequestData:  Reads part of the netCDF
  // file and sends sub-arrays to all ranks that need that data
  int ReadAndSend(svtkInformation* outInfo, int varID);

  // Returns the MPI rank of the process that should read the specified depth
  int ReaderForDepth(unsigned depth);

  bool IsReaderRank();
  bool IsFirstReaderRank();

  static void SelectionModifiedCallback(
    svtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  static void EventCallback(svtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  svtkCallbackCommand* SelectionObserver;

  char* FileName;
  char* OpenedFileName;
  svtkSetStringMacro(OpenedFileName);

  int NCDFFD; // netcdf file descriptor

  int Stride[3];

  svtkMPIController* Controller;

private:
  svtkPNetCDFPOPReader(const svtkPNetCDFPOPReader&) = delete;
  void operator=(const svtkPNetCDFPOPReader&) = delete;

  svtkPNetCDFPOPReaderInternal* Internals;
};
#endif
