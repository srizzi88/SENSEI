/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkNewickTreeWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkNewickTreeWriter.h"

#include "svtkDataSetAttributes.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkTree.h"

svtkStandardNewMacro(svtkNewickTreeWriter);

//----------------------------------------------------------------------------
svtkNewickTreeWriter::svtkNewickTreeWriter()
{
  this->SetFileTypeToASCII();

  this->EdgeWeightArrayName = "weight";
  this->NodeNameArrayName = "node name";

  this->EdgeWeightArray = nullptr;
  this->NodeNameArray = nullptr;
}

//----------------------------------------------------------------------------
void svtkNewickTreeWriter::WriteData()
{
  svtkDebugMacro(<< "Writing svtk tree data...");

  svtkTree* const input = this->GetInput();

  this->EdgeWeightArray = input->GetEdgeData()->GetAbstractArray(this->EdgeWeightArrayName.c_str());

  this->NodeNameArray = input->GetVertexData()->GetAbstractArray(this->NodeNameArrayName.c_str());

  ostream* fp = this->OpenSVTKFile();
  if (!fp)
  {
    svtkErrorMacro("Failed to open output stream");
    return;
  }

  this->WriteVertex(fp, input, input->GetRoot());

  // the tree ends with a semi-colon
  *fp << ";";

  this->CloseSVTKFile(fp);
}

//----------------------------------------------------------------------------
void svtkNewickTreeWriter::WriteVertex(ostream* fp, svtkTree* const input, svtkIdType vertex)
{
  svtkIdType numChildren = input->GetNumberOfChildren(vertex);
  if (numChildren > 0)
  {
    *fp << "(";
    for (svtkIdType child = 0; child < numChildren; ++child)
    {
      this->WriteVertex(fp, input, input->GetChild(vertex, child));
      if (child != numChildren - 1)
      {
        *fp << ",";
      }
    }
    *fp << ")";
  }

  if (this->NodeNameArray)
  {
    svtkStdString name = this->NodeNameArray->GetVariantValue(vertex).ToString();
    if (!name.empty())
    {
      *fp << name;
    }
  }

  if (this->EdgeWeightArray)
  {
    svtkIdType parent = input->GetParent(vertex);
    if (parent != -1)
    {
      svtkIdType edge = input->GetEdgeId(parent, vertex);
      if (edge != -1)
      {
        double weight = this->EdgeWeightArray->GetVariantValue(edge).ToDouble();
        *fp << ":" << weight;
      }
    }
  }
}

//----------------------------------------------------------------------------
int svtkNewickTreeWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTree");
  return 1;
}

//----------------------------------------------------------------------------
svtkTree* svtkNewickTreeWriter::GetInput()
{
  return svtkTree::SafeDownCast(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
svtkTree* svtkNewickTreeWriter::GetInput(int port)
{
  return svtkTree::SafeDownCast(this->Superclass::GetInput(port));
}

//----------------------------------------------------------------------------
void svtkNewickTreeWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "EdgeWeightArrayName: " << this->EdgeWeightArrayName << endl;
  os << indent << "NodeNameArrayName: " << this->NodeNameArrayName << endl;
}
