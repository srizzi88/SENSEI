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
 * @class   svtkPXdmf3Writer
 * @brief   mpi parallel writer for XDMF/HDF5 files
 *
 * svtkPXdmf3Writer converts svtkDataObjects to XDMF format and and when
 * run in parallel under MPI each rank writes only the data it is
 * responsible for.
 *
 * In the absence of the information provided by svtkModelMetadata,
 * if this writer is not part of a parallel application, we will use
 * reasonable defaults for all the values in the output XDMF file.
 * If you don't provide a block ID element array, we'll create a
 * block for each cell type that appears in the unstructured grid.
 *
 * However if this writer is part of a parallel application (hence
 * writing out a distributed XDMF file), then we need at the very
 * least a list of all the block IDs that appear in the file.  And
 * we need the element array of block IDs for the input unstructured grid.
 *
 * In the absence of a svtkModelMetadata object, you can also provide
 * time step information which we will include in the output XDMF
 * file.
 */

#ifndef svtkPXdmf3Writer_h
#define svtkPXdmf3Writer_h

#include "svtkIOParallelXdmf3Module.h" // For export macro
#include "svtkXdmf3Writer.h"

class SVTKIOPARALLELXDMF3_EXPORT svtkPXdmf3Writer : public svtkXdmf3Writer
{
public:
  static svtkPXdmf3Writer* New();
  svtkTypeMacro(svtkPXdmf3Writer, svtkXdmf3Writer);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkPXdmf3Writer();
  ~svtkPXdmf3Writer() override;
  int CheckParameters() override;

  int RequestUpdateExtent(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int GlobalContinueExecuting(int localContinue) override;

private:
  svtkPXdmf3Writer(const svtkPXdmf3Writer&) = delete;
  void operator=(const svtkPXdmf3Writer&) = delete;
};

#endif /* svtkPXdmf3Writer_h */
