#!/usr/bin/env python
from svtk import *

database = svtkSQLDatabase.CreateFromURL("mysql://enron@vizdb.srn.sandia.gov:3306/enron")
database.Open("enron")

edge_query = database.GetQueryInstance()
edge_query.SetQuery("select sendID, recvID, weight from email_arcs")

vertex_query = database.GetQueryInstance()
vertex_query.SetQuery("select firstName, lastName, eid from employeelist")

edge_table = svtkRowQueryToTable()
edge_table.SetQuery(edge_query)

vertex_table = svtkRowQueryToTable()
vertex_table.SetQuery(vertex_query)

source = svtkTableToGraph()
source.AddInputConnection(edge_table.GetOutputPort())
source.AddLinkVertex("sendID", "eid", False)
source.AddLinkVertex("recvID", "eid", False)
source.AddLinkEdge("sendID", "recvID")
source.SetVertexTableConnection(vertex_table.GetOutputPort())

view = svtkGraphLayoutView()
view.AddRepresentationFromInputConnection(source.GetOutputPort())
view.SetVertexLabelArrayName("lastName")
view.SetVertexLabelVisibility(True)
view.SetEdgeColorArrayName("sendID")
view.SetColorEdges(True)

theme = svtkViewTheme.CreateMellowTheme()
view.ApplyViewTheme(theme)

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()
view.GetInteractor().Start()
