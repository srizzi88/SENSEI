/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGraphWriter.h"

#include "svtkByteSwap.h"
#include "svtkCellArray.h"
#include "svtkDirectedGraph.h"
#include "svtkEdgeListIterator.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkMolecule.h"
#include "svtkObjectFactory.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

svtkStandardNewMacro(svtkGraphWriter);

void svtkGraphWriter::WriteData()
{
  ostream* fp;
  svtkGraph* const input = this->GetInput();

  svtkDebugMacro(<< "Writing svtk graph data...");

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

  svtkMolecule* mol = svtkMolecule::SafeDownCast(input);
  if (mol) // molecule is most derived, test first
  {
    *fp << "DATASET MOLECULE\n";
    this->WriteMoleculeData(fp, mol);
  }
  else if (svtkDirectedGraph::SafeDownCast(input))
  {
    *fp << "DATASET DIRECTED_GRAPH\n";
  }
  else
  {
    *fp << "DATASET UNDIRECTED_GRAPH\n";
  }

  int error_occurred = 0;

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
    const svtkIdType vertex_count = input->GetNumberOfVertices();
    *fp << "VERTICES " << vertex_count << "\n";
    const svtkIdType edge_count = input->GetNumberOfEdges();
    *fp << "EDGES " << edge_count << "\n";
    for (svtkIdType e = 0; e < edge_count; ++e)
    {
      *fp << input->GetSourceVertex(e) << " " << input->GetTargetVertex(e) << "\n";
    }
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

void svtkGraphWriter::WriteMoleculeData(std::ostream* fp, svtkMolecule* m)
{
  if (m->HasLattice())
  {
    svtkVector3d a;
    svtkVector3d b;
    svtkVector3d c;
    svtkVector3d origin;
    m->GetLattice(a, b, c, origin);
    *fp << "LATTICE_A " << a[0] << " " << a[1] << " " << a[2] << "\n";
    *fp << "LATTICE_B " << b[0] << " " << b[1] << " " << b[2] << "\n";
    *fp << "LATTICE_C " << c[0] << " " << c[1] << " " << c[2] << "\n";
    *fp << "LATTICE_ORIGIN " << origin[0] << " " << origin[1] << " " << origin[2] << "\n";
  }
}

int svtkGraphWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  return 1;
}

svtkGraph* svtkGraphWriter::GetInput()
{
  return svtkGraph::SafeDownCast(this->Superclass::GetInput());
}

svtkGraph* svtkGraphWriter::GetInput(int port)
{
  return svtkGraph::SafeDownCast(this->Superclass::GetInput(port));
}

void svtkGraphWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
