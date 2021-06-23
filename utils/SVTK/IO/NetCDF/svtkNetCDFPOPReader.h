/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkNetCDFPOPReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkNetCDFPOPReader
 * @brief   read NetCDF files
 * .Author Joshua Wu 09.15.2009
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

#ifndef svtkNetCDFPOPReader_h
#define svtkNetCDFPOPReader_h

#include "svtkIONetCDFModule.h" // For export macro
#include "svtkRectilinearGridAlgorithm.h"

class svtkDataArraySelection;
class svtkCallbackCommand;
class svtkNetCDFPOPReaderInternal;

class SVTKIONETCDF_EXPORT svtkNetCDFPOPReader : public svtkRectilinearGridAlgorithm
{
public:
  svtkTypeMacro(svtkNetCDFPOPReader, svtkRectilinearGridAlgorithm);
  static svtkNetCDFPOPReader* New();
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

protected:
  svtkNetCDFPOPReader();
  ~svtkNetCDFPOPReader() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  static void SelectionModifiedCallback(
    svtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  static void EventCallback(svtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  svtkCallbackCommand* SelectionObserver;

  char* FileName;

  /**
   * The NetCDF file descriptor.
   */
  int NCDFFD;

  /**
   * The file name of the opened file.
   */
  char* OpenedFileName;

  svtkSetStringMacro(OpenedFileName);

  int Stride[3];

private:
  svtkNetCDFPOPReader(const svtkNetCDFPOPReader&) = delete;
  void operator=(const svtkNetCDFPOPReader&) = delete;

  svtkNetCDFPOPReaderInternal* Internals;
};
#endif
