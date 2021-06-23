/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAppendSelection.cxx

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

#include "svtkAppendSelection.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int SelectionCompare(svtkSelectionNode* a, svtkSelectionNode* b)
{
  int errors = 0;
  svtkIdTypeArray* alist = svtkArrayDownCast<svtkIdTypeArray>(a->GetSelectionList());
  svtkIdTypeArray* blist = svtkArrayDownCast<svtkIdTypeArray>(b->GetSelectionList());
  if (a->GetContentType() != b->GetContentType())
  {
    cerr << "ERROR: Content type does not match." << endl;
    errors++;
  }
  if (a->GetContentType() == svtkSelectionNode::VALUES)
  {
    if (!alist->GetName() || !blist->GetName() || strcmp(alist->GetName(), blist->GetName()))
    {
      cerr << "ERROR: The array names do not match." << endl;
    }
  }
  if (a->GetFieldType() != b->GetFieldType())
  {
    cerr << "ERROR: Field type does not match." << endl;
    errors++;
  }
  if ((alist && !blist) || (!alist && blist))
  {
    cerr << "ERROR: One has a selection list while the other does not." << endl;
    errors++;
  }
  if (alist && blist)
  {
    int numComps = alist->GetNumberOfComponents();
    svtkIdType numTuples = alist->GetNumberOfTuples();
    int bnumComps = blist->GetNumberOfComponents();
    svtkIdType bnumTuples = blist->GetNumberOfTuples();
    if (numTuples != bnumTuples)
    {
      cerr << "ERROR: The number of tuples in the selection list do not match (" << numTuples
           << "!=" << bnumTuples << ")." << endl;
      errors++;
    }
    else if (numComps != bnumComps)
    {
      cerr << "ERROR: The number of components in the selection list do not match (" << numComps
           << "!=" << bnumComps << ")." << endl;
      errors++;
    }
    else
    {
      for (svtkIdType i = 0; i < numComps * numTuples; i++)
      {
        if (alist->GetValue(i) != blist->GetValue(i))
        {
          cerr << "ERROR: Selection lists do not match at sel " << i << "(" << alist->GetValue(i)
               << " != " << blist->GetValue(i) << ")." << endl;
          errors++;
          break;
        }
      }
    }
  }
  return errors;
}

int SelectionCompare(svtkSelection* a, svtkSelection* b)
{
  int errors = 0;
  if (a->GetNumberOfNodes() != b->GetNumberOfNodes())
  {
    cerr << "ERROR: Number of nodes do not match." << endl;
    errors++;
  }
  else
  {
    for (unsigned int cc = 0; cc < a->GetNumberOfNodes(); cc++)
    {
      errors += SelectionCompare(a->GetNode(cc), b->GetNode(cc));
    }
  }
  return errors;
}

int TestAppendSelectionCase(svtkSelection* input1, svtkSelection* input2, svtkSelection* correct)
{
  SVTK_CREATE(svtkAppendSelection, append);
  append->AddInputData(input1);
  append->AddInputData(input2);
  append->Update();
  svtkSelection* output = append->GetOutput();
  return SelectionCompare(output, correct);
}

int TestAppendSelection(int, char*[])
{
  int errors = 0;

  {
    cerr << "Testing appending sel selections ..." << endl;
    SVTK_CREATE(svtkSelection, sel1);
    SVTK_CREATE(svtkSelectionNode, sel1Node);
    SVTK_CREATE(svtkIdTypeArray, sel1Arr);
    sel1->AddNode(sel1Node);
    sel1Node->SetContentType(svtkSelectionNode::INDICES);
    sel1Node->SetFieldType(svtkSelectionNode::CELL);
    sel1Node->SetSelectionList(sel1Arr);
    sel1Arr->InsertNextValue(0);
    sel1Arr->InsertNextValue(1);
    sel1Arr->InsertNextValue(2);
    SVTK_CREATE(svtkSelection, sel2);
    SVTK_CREATE(svtkSelectionNode, sel2Node);
    SVTK_CREATE(svtkIdTypeArray, sel2Arr);
    sel2->AddNode(sel2Node);
    sel2Node->SetContentType(svtkSelectionNode::INDICES);
    sel2Node->SetFieldType(svtkSelectionNode::CELL);
    sel2Node->SetSelectionList(sel2Arr);
    sel2Arr->InsertNextValue(3);
    sel2Arr->InsertNextValue(4);
    sel2Arr->InsertNextValue(5);
    SVTK_CREATE(svtkSelection, selAppend);
    SVTK_CREATE(svtkSelectionNode, selAppendNode);
    SVTK_CREATE(svtkIdTypeArray, selAppendArr);
    selAppend->AddNode(selAppendNode);
    selAppendNode->SetContentType(svtkSelectionNode::INDICES);
    selAppendNode->SetFieldType(svtkSelectionNode::CELL);
    selAppendNode->SetSelectionList(selAppendArr);
    selAppendArr->InsertNextValue(0);
    selAppendArr->InsertNextValue(1);
    selAppendArr->InsertNextValue(2);
    selAppendArr->InsertNextValue(3);
    selAppendArr->InsertNextValue(4);
    selAppendArr->InsertNextValue(5);
    errors += TestAppendSelectionCase(sel1, sel2, selAppend);
    cerr << "... done." << endl;
  }

  {
    cerr << "Testing appending value selections ..." << endl;
    SVTK_CREATE(svtkSelection, sel1);
    SVTK_CREATE(svtkSelectionNode, sel1Node);
    SVTK_CREATE(svtkIdTypeArray, sel1Arr);
    sel1->AddNode(sel1Node);
    sel1Arr->SetName("arrayname");
    sel1Node->SetContentType(svtkSelectionNode::VALUES);
    sel1Node->SetFieldType(svtkSelectionNode::CELL);
    sel1Node->SetSelectionList(sel1Arr);
    sel1Arr->InsertNextValue(0);
    sel1Arr->InsertNextValue(1);
    sel1Arr->InsertNextValue(2);
    SVTK_CREATE(svtkSelection, sel2);
    SVTK_CREATE(svtkSelectionNode, sel2Node);
    SVTK_CREATE(svtkIdTypeArray, sel2Arr);
    sel2->AddNode(sel2Node);
    sel2Arr->SetName("arrayname");
    sel2Node->SetContentType(svtkSelectionNode::VALUES);
    sel2Node->SetFieldType(svtkSelectionNode::CELL);
    sel2Node->SetSelectionList(sel2Arr);
    sel2Arr->InsertNextValue(3);
    sel2Arr->InsertNextValue(4);
    sel2Arr->InsertNextValue(5);
    SVTK_CREATE(svtkSelection, selAppend);
    SVTK_CREATE(svtkSelectionNode, selAppendNode);
    SVTK_CREATE(svtkIdTypeArray, selAppendArr);
    selAppend->AddNode(selAppendNode);
    selAppendArr->SetName("arrayname");
    selAppendNode->SetContentType(svtkSelectionNode::VALUES);
    selAppendNode->SetFieldType(svtkSelectionNode::CELL);
    selAppendNode->SetSelectionList(selAppendArr);
    selAppendArr->InsertNextValue(0);
    selAppendArr->InsertNextValue(1);
    selAppendArr->InsertNextValue(2);
    selAppendArr->InsertNextValue(3);
    selAppendArr->InsertNextValue(4);
    selAppendArr->InsertNextValue(5);
    errors += TestAppendSelectionCase(sel1, sel2, selAppend);
    cerr << "... done." << endl;
  }

  {
    cerr << "Testing appending cell selections with different process ids..." << endl;
    SVTK_CREATE(svtkSelection, sel1);
    SVTK_CREATE(svtkSelectionNode, sel1Node);
    SVTK_CREATE(svtkIdTypeArray, sel1Arr);
    sel1->AddNode(sel1Node);
    sel1Node->SetContentType(svtkSelectionNode::INDICES);
    sel1Node->SetFieldType(svtkSelectionNode::CELL);
    sel1Node->SetSelectionList(sel1Arr);
    sel1Node->GetProperties()->Set(svtkSelectionNode::PROCESS_ID(), 0);
    sel1Arr->InsertNextValue(0);
    sel1Arr->InsertNextValue(1);
    sel1Arr->InsertNextValue(2);
    SVTK_CREATE(svtkSelection, sel2);
    SVTK_CREATE(svtkSelectionNode, sel2Node);
    SVTK_CREATE(svtkIdTypeArray, sel2Arr);
    sel2->AddNode(sel2Node);
    sel2Node->SetContentType(svtkSelectionNode::INDICES);
    sel2Node->SetFieldType(svtkSelectionNode::CELL);
    sel2Node->SetSelectionList(sel2Arr);
    sel2Node->GetProperties()->Set(svtkSelectionNode::PROCESS_ID(), 1);
    sel2Arr->InsertNextValue(3);
    sel2Arr->InsertNextValue(4);
    sel2Arr->InsertNextValue(5);
    SVTK_CREATE(svtkSelection, selAppend);
    selAppend->AddNode(sel1Node);
    selAppend->AddNode(sel2Node);
    errors += TestAppendSelectionCase(sel1, sel2, selAppend);
    cerr << "... done." << endl;
  }

  return errors;
}
