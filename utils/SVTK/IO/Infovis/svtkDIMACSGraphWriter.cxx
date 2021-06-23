/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDIMACSGraphWriter.cxx

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

#include "svtkDIMACSGraphWriter.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDirectedGraph.h"
#include "svtkEdgeListIterator.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

svtkStandardNewMacro(svtkDIMACSGraphWriter);

void svtkDIMACSGraphWriter::WriteData()
{
  svtkGraph* const input = this->GetInput();

  svtkDebugMacro(<< "Writing svtk graph data...");

  ostream* fp = this->OpenSVTKFile();
  if (!fp)
  {
    svtkErrorMacro("Failed to open output stream");
    return;
  }

  *fp << "c svtkGraph as DIMACS format\n";

  if (svtkDirectedGraph::SafeDownCast(input))
  {
    *fp << "c Graph stored as DIRECTED\n";
  }
  else
  {
    *fp << "c Graph stored as UNDIRECTED\n";
  }

  const svtkIdType vertex_count = input->GetNumberOfVertices();
  const svtkIdType edge_count = input->GetNumberOfEdges();

  // Output this 'special' line with the 'problem type' and then
  // vertex and edge counts
  *fp << "p graph " << vertex_count << " " << edge_count << "\n";

  // See if the input has a "weight" array
  svtkDataArray* weight = input->GetEdgeData()->GetArray("weight");

  // Output either the weight array or just 1 if
  // we have no weight array
  SVTK_CREATE(svtkEdgeListIterator, edges);
  input->GetEdges(edges);
  if (weight)
  {
    while (edges->HasNext())
    {
      svtkEdgeType e = edges->Next();
      float value = weight->GetTuple1(e.Id);
      *fp << "e " << e.Source + 1 << " " << e.Target + 1 << " " << value << "\n";
    }
  }
  else
  {
    while (edges->HasNext())
    {
      svtkEdgeType e = edges->Next();
      *fp << "e " << e.Source + 1 << " " << e.Target + 1 << " 1\n";
    }
  }

  // NOTE: Vertices are incremented by 1 since DIMACS files number vertices
  //       from 1..n.

  this->CloseSVTKFile(fp);
}

int svtkDIMACSGraphWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  return 1;
}

svtkGraph* svtkDIMACSGraphWriter::GetInput()
{
  return svtkGraph::SafeDownCast(this->Superclass::GetInput());
}

svtkGraph* svtkDIMACSGraphWriter::GetInput(int port)
{
  return svtkGraph::SafeDownCast(this->Superclass::GetInput(port));
}

void svtkDIMACSGraphWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
