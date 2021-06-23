/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPUnstructuredDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPUnstructuredDataWriter.h"

#include "svtkErrorCode.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkPointSet.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkXMLUnstructuredDataWriter.h"

//----------------------------------------------------------------------------
svtkXMLPUnstructuredDataWriter::svtkXMLPUnstructuredDataWriter() = default;

//----------------------------------------------------------------------------
svtkXMLPUnstructuredDataWriter::~svtkXMLPUnstructuredDataWriter() = default;

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredDataWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkPointSet* svtkXMLPUnstructuredDataWriter::GetInputAsPointSet()
{
  return static_cast<svtkPointSet*>(this->GetInput());
}

//----------------------------------------------------------------------------
svtkXMLWriter* svtkXMLPUnstructuredDataWriter::CreatePieceWriter(int index)
{
  svtkXMLUnstructuredDataWriter* pWriter = this->CreateUnstructuredPieceWriter();
  pWriter->SetNumberOfPieces(this->NumberOfPieces);
  pWriter->SetWritePiece(index);
  pWriter->SetGhostLevel(this->GhostLevel);

  return pWriter;
}

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredDataWriter::WritePData(svtkIndent indent)
{
  this->Superclass::WritePData(indent);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  svtkPointSet* input = this->GetInputAsPointSet();
  this->WritePPoints(input->GetPoints(), indent);
}
