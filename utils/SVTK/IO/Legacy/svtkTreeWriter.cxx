/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTreeWriter.h"

#include "svtkByteSwap.h"
#include "svtkCellArray.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkTree.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

svtkStandardNewMacro(svtkTreeWriter);

void svtkTreeWriter::WriteEdges(ostream& Stream, svtkTree* Tree)
{
  for (svtkIdType e = 0; e < Tree->GetNumberOfEdges(); ++e)
  {
    svtkIdType parent = Tree->GetSourceVertex(e);
    svtkIdType child = Tree->GetTargetVertex(e);
    Stream << child << " " << parent << "\n";
  }
}

void svtkTreeWriter::WriteData()
{
  ostream* fp;
  svtkTree* const input = this->GetInput();

  svtkDebugMacro(<< "Writing svtk tree data...");

  if (!(fp = this->OpenSVTKFile()) || !this->WriteHeader(fp))
  {
    if (fp)
    {
      if (this->FileName)
      {
        svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
        this->CloseSVTKFile(fp);
        unlink(this->FileName);
      }
      else
      {
        this->CloseSVTKFile(fp);
        svtkErrorMacro("Could not read memory header. ");
      }
    }
    return;
  }

  *fp << "DATASET TREE\n";

  bool error_occurred = 0;

  if (!this->WriteFieldData(fp, input->GetFieldData()))
  {
    error_occurred = 1;
  }
  if (!error_occurred && !this->WritePoints(fp, input->GetPoints()))
  {
    error_occurred = 1;
  }
  if (!error_occurred)
  {
    const svtkIdType edge_count = input->GetNumberOfEdges();
    *fp << "EDGES " << edge_count << "\n";
    this->WriteEdges(*fp, input);
  }
  if (!error_occurred && !this->WriteEdgeData(fp, input))
  {
    error_occurred = 1;
  }
  if (!error_occurred && !this->WriteVertexData(fp, input))
  {
    error_occurred = 1;
  }

  if (error_occurred)
  {
    if (this->FileName)
    {
      svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
      this->CloseSVTKFile(fp);
      unlink(this->FileName);
    }
    else
    {
      svtkErrorMacro("Error writing data set to memory");
      this->CloseSVTKFile(fp);
    }
    return;
  }
  this->CloseSVTKFile(fp);
}

int svtkTreeWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTree");
  return 1;
}

svtkTree* svtkTreeWriter::GetInput()
{
  return svtkTree::SafeDownCast(this->Superclass::GetInput());
}

svtkTree* svtkTreeWriter::GetInput(int port)
{
  return svtkTree::SafeDownCast(this->Superclass::GetInput(port));
}

void svtkTreeWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
