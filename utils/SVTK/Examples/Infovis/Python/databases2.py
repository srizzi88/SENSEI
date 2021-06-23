#!/usr/bin/env python
from svtk import *
import os.path
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

data_dir = SVTK_DATA_ROOT + "/Data/Infovis/SQLite/"
if not os.path.exists(data_dir):
  data_dir = SVTK_DATA_ROOT + "/Data/Infovis/SQLite/"
if not os.path.exists(data_dir):
  data_dir = SVTK_DATA_ROOT + "/Data/Infovis/SQLite/"
sqlite_file = data_dir + "SmallEmailTest.db"

# Construct a graph from database tables (yes very tricky)
databaseToGraph = svtkSQLDatabaseGraphSource()
databaseToGraph.SetURL("sqlite://" + sqlite_file)
databaseToGraph.SetEdgeQuery("select source, target from emails")
databaseToGraph.SetVertexQuery("select Name, Job, Age from employee")
databaseToGraph.AddLinkVertex("source", "Name", False)
databaseToGraph.AddLinkVertex("target", "Name", False)
databaseToGraph.AddLinkEdge("source", "target")

view = svtkGraphLayoutView()
view.AddRepresentationFromInputConnection(databaseToGraph.GetOutputPort())
view.SetVertexLabelArrayName("label")
view.SetVertexLabelVisibility(True)
view.SetVertexColorArrayName("Age")
view.SetColorVertices(True)
view.SetLayoutStrategyToSimple2D()


theme = svtkViewTheme.CreateMellowTheme()
theme.SetCellColor(.2,.2,.6)
theme.SetLineWidth(5)
theme.SetPointSize(10)
view.ApplyViewTheme(theme)
theme.FastDelete()

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()
view.GetInteractor().Start()
