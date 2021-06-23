/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSQLGraphReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkActor.h"
#include "svtkGraphMapper.h"
#include "svtkMath.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSQLGraphReader.h"
#include "svtkSQLQuery.h"
#include "svtkSQLiteDatabase.h"
#include "svtkSmartPointer.h"
#include "svtkStdString.h"

#include <sstream>

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestSQLGraphReader(int argc, char* argv[])
{
  svtkIdType vertices = 10;

  // Create a SQLite in-memory database
  SVTK_CREATE(svtkSQLiteDatabase, database);
  database->SetDatabaseFileName(":memory:");

  bool ok = database->Open("");
  if (!ok)
  {
    cerr << "Could not open database!" << endl;
    cerr << database->GetLastErrorText() << endl;
    return 1;
  }

  // Build a graph
  svtkSmartPointer<svtkSQLQuery> q;
  q.TakeReference(database->GetQueryInstance());
  std::ostringstream oss;

  q->SetQuery("DROP TABLE IF EXISTS vertices");
  q->Execute();

  q->SetQuery("CREATE TABLE vertices (id INTEGER, x FLOAT, y FLOAT)");
  q->Execute();
  for (int i = 0; i < vertices; i++)
  {
    oss.str("");
    oss << "INSERT INTO vertices VALUES(" << i << ", "
        << 0.5 * cos(i * 2.0 * svtkMath::Pi() / vertices) << ", "
        << 0.5 * sin(i * 2.0 * svtkMath::Pi() / vertices) << ")" << endl;
    q->SetQuery(oss.str().c_str());
    q->Execute();
  }

  q->SetQuery("DROP TABLE IF EXISTS edges");
  q->Execute();

  q->SetQuery("CREATE TABLE edges (id INTEGER, source INTEGER, target INTEGER)");
  q->Execute();
  for (int i = 0; i < vertices; i++)
  {
    oss.str("");
    oss << "INSERT INTO edges VALUES(" << 2 * i + 0 << ", " << i << ", " << (i + 1) % vertices
        << ")" << endl;
    q->SetQuery(oss.str().c_str());
    q->Execute();
    oss.str("");
    oss << "INSERT INTO edges VALUES(" << 2 * i + 1 << ", " << (i + 3) % vertices << ", " << i
        << ")" << endl;
    q->SetQuery(oss.str().c_str());
    q->Execute();
  }

  // Set up graph reader
  SVTK_CREATE(svtkSQLGraphReader, reader);
  svtkSmartPointer<svtkSQLQuery> edgeQuery;
  edgeQuery.TakeReference(database->GetQueryInstance());
  edgeQuery->SetQuery("select * from edges");
  reader->SetEdgeQuery(edgeQuery);

  svtkSmartPointer<svtkSQLQuery> vertexQuery;
  vertexQuery.TakeReference(database->GetQueryInstance());
  vertexQuery->SetQuery("select * from vertices");
  reader->SetVertexQuery(vertexQuery);

  reader->SetSourceField("source");
  reader->SetTargetField("target");
  reader->SetVertexIdField("id");
  reader->SetXField("x");
  reader->SetYField("y");

  // Display the graph
  SVTK_CREATE(svtkGraphMapper, mapper);
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetEdgeColorArrayName("id");
  mapper->ColorEdgesOn();
  mapper->SetVertexColorArrayName("id");
  mapper->ColorVerticesOn();
  SVTK_CREATE(svtkActor, actor);
  actor->SetMapper(mapper);
  SVTK_CREATE(svtkRenderer, ren);
  ren->AddActor(actor);
  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  SVTK_CREATE(svtkRenderWindow, win);
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  int retVal = svtkRegressionTestImage(win);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Initialize();
    iren->Start();

    retVal = svtkRegressionTester::PASSED;
  }

  return !retVal;
}
