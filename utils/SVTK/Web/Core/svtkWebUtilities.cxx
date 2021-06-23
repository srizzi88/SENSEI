/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWebUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkWebUtilities.h"
#include "svtkPython.h" // Need to be first and used for Py_xxx macros

#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkJavaScriptDataWriter.h"
#include "svtkMultiProcessController.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkSplitColumnComponents.h"
#include "svtkTable.h"

#include <sstream>

svtkStandardNewMacro(svtkWebUtilities);
//----------------------------------------------------------------------------
svtkWebUtilities::svtkWebUtilities() = default;

//----------------------------------------------------------------------------
svtkWebUtilities::~svtkWebUtilities() = default;

//----------------------------------------------------------------------------
std::string svtkWebUtilities::WriteAttributesToJavaScript(int field_type, svtkDataSet* dataset)
{
  if (dataset == nullptr ||
    (field_type != svtkDataObject::POINT && field_type != svtkDataObject::CELL))
  {
    return "[]";
  }

  std::ostringstream stream;

  svtkNew<svtkDataSetAttributes> clone;
  clone->PassData(dataset->GetAttributes(field_type));
  clone->RemoveArray("svtkValidPointMask");

  svtkNew<svtkTable> table;
  table->SetRowData(clone);

  svtkNew<svtkSplitColumnComponents> splitter;
  splitter->SetInputDataObject(table);
  splitter->Update();

  svtkNew<svtkJavaScriptDataWriter> writer;
  writer->SetOutputStream(&stream);
  writer->SetInputDataObject(splitter->GetOutputDataObject(0));
  writer->SetVariableName(nullptr);
  writer->SetIncludeFieldNames(false);
  writer->Write();

  return stream.str();
}

//----------------------------------------------------------------------------
std::string svtkWebUtilities::WriteAttributeHeadersToJavaScript(int field_type, svtkDataSet* dataset)
{
  if (dataset == nullptr ||
    (field_type != svtkDataObject::POINT && field_type != svtkDataObject::CELL))
  {
    return "[]";
  }

  std::ostringstream stream;
  stream << "[";

  svtkDataSetAttributes* dsa = dataset->GetAttributes(field_type);
  svtkNew<svtkDataSetAttributes> clone;
  clone->CopyAllocate(dsa, 0);
  clone->RemoveArray("svtkValidPointMask");

  svtkNew<svtkTable> table;
  table->SetRowData(clone);

  svtkNew<svtkSplitColumnComponents> splitter;
  splitter->SetInputDataObject(table);
  splitter->Update();

  dsa = svtkTable::SafeDownCast(splitter->GetOutputDataObject(0))->GetRowData();

  for (int cc = 0; cc < dsa->GetNumberOfArrays(); cc++)
  {
    const char* name = dsa->GetArrayName(cc);
    if (cc != 0)
    {
      stream << ", ";
    }
    stream << "\"" << (name ? name : "") << "\"";
  }
  stream << "]";
  return stream.str();
}

//----------------------------------------------------------------------------
void svtkWebUtilities::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkWebUtilities::ProcessRMIs()
{
  svtkWebUtilities::ProcessRMIs(1, 0);
}

//----------------------------------------------------------------------------
void svtkWebUtilities::ProcessRMIs(int reportError, int dont_loop)
{
  Py_BEGIN_ALLOW_THREADS svtkMultiProcessController::GetGlobalController()->ProcessRMIs(
    reportError, dont_loop);
  Py_END_ALLOW_THREADS
}
