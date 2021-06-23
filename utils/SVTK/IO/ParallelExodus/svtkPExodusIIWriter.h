/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExodusIIWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class   svtkPExodusIIWriter
 * @brief   Write Exodus II files
 *
 *     This is a svtkWriter that writes it's svtkUnstructuredGrid
 *     input out to an Exodus II file.  Go to http://endo.sandia.gov/SEACAS/
 *     for more information about the Exodus II format.
 *
 *     Exodus files contain much information that is not captured
 *     in a svtkUnstructuredGrid, such as time steps, information
 *     lines, node sets, and side sets.  This information can be
 *     stored in a svtkModelMetadata object.
 *
 *     The svtkExodusReader and svtkPExodusReader can create
 *     a svtkModelMetadata object and embed it in a svtkUnstructuredGrid
 *     in a series of field arrays.  This writer searches for these
 *     field arrays and will use the metadata contained in them
 *     when creating the new Exodus II file.
 *
 *     You can also explicitly give the svtkExodusIIWriter a
 *     svtkModelMetadata object to use when writing the file.
 *
 *     In the absence of the information provided by svtkModelMetadata,
 *     if this writer is not part of a parallel application, we will use
 *     reasonable defaults for all the values in the output Exodus file.
 *     If you don't provide a block ID element array, we'll create a
 *     block for each cell type that appears in the unstructured grid.
 *
 *     However if this writer is part of a parallel application (hence
 *     writing out a distributed Exodus file), then we need at the very
 *     least a list of all the block IDs that appear in the file.  And
 *     we need the element array of block IDs for the input unstructured grid.
 *
 *     In the absence of a svtkModelMetadata object, you can also provide
 *     time step information which we will include in the output Exodus
 *     file.
 *
 * @warning
 *     If the input floating point field arrays and point locations are all
 *     floats or all doubles, this class will operate more efficiently.
 *     Mixing floats and doubles will slow you down, because Exodus II
 *     requires that we write only floats or only doubles.
 *
 * @warning
 *     We use the terms "point" and "node" interchangeably.
 *     Also, we use the terms "element" and "cell" interchangeably.
 */

#ifndef svtkPExodusIIWriter_h
#define svtkPExodusIIWriter_h

#include "svtkExodusIIWriter.h"
#include "svtkIOParallelExodusModule.h" // For export macro
#include "svtkSmartPointer.h"           // For svtkSmartPointer

#include <map>    // STL Header
#include <string> // STL Header
#include <vector> // STL Header

class svtkModelMetadata;
class svtkDoubleArray;
class svtkIntArray;
class svtkUnstructuredGrid;

class SVTKIOPARALLELEXODUS_EXPORT svtkPExodusIIWriter : public svtkExodusIIWriter
{
public:
  static svtkPExodusIIWriter* New();
  svtkTypeMacro(svtkPExodusIIWriter, svtkExodusIIWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkPExodusIIWriter();
  ~svtkPExodusIIWriter() override;
  int CheckParameters() override;
  void CheckBlockInfoMap() override;

  int RequestUpdateExtent(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int GlobalContinueExecuting(int localContinue) override;

  /**
   * Get the maximum length name in the input data set. If it is smaller
   * than 32 characters long we just return the ExodusII default of 32.
   */
  virtual unsigned int GetMaxNameLength() override;

private:
  svtkPExodusIIWriter(const svtkPExodusIIWriter&) = delete;
  void operator=(const svtkPExodusIIWriter&) = delete;
};

#endif
