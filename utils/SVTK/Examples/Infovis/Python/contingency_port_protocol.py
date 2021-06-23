#!/usr/bin/env python
from svtk import *
import os.path
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Open database
data_dir = SVTK_DATA_ROOT + "/Data/Infovis/SQLite/"
if not os.path.exists(data_dir):
  data_dir = SVTK_DATA_ROOT + "/Data/Infovis/SQLite/"
sqlite_file = data_dir + "ports_protocols.db"
database = svtkSQLDatabase.CreateFromURL("sqlite://" + sqlite_file)
database.Open("")

# Query database for edges
edge_query = database.GetQueryInstance()
edge_query.SetQuery("select src, dst, dport, protocol, port_protocol from tcp")
edge_table = svtkRowQueryToTable()
edge_table.SetQuery(edge_query)

# Calculate contingency statistics on (src,dst)
cs = svtkContingencyStatistics()
cs.AddInputConnection(edge_table.GetOutputPort())
cs.AddColumnPair("dport","protocol")
cs.SetAssessOption( 1 )
cs.Update()
cStats = cs.GetOutput( 0 )
cStats.Dump( 12 )

# Query database for vertices
vertex_query = database.GetQueryInstance()
vertex_query.SetQuery("select ip, hostname from dnsnames")
vertex_table = svtkRowQueryToTable()
vertex_table.SetQuery(vertex_query)

graph = svtkTableToGraph()
graph.AddInputConnection(cs.GetOutputPort())
graph.SetVertexTableConnection(vertex_table.GetOutputPort())
graph.AddLinkVertex("src", "ip", False)
graph.AddLinkVertex("dst", "ip", False)
graph.AddLinkEdge("src", "dst")
graph.Update()

view = svtkGraphLayoutView()
view.AddRepresentationFromInputConnection(graph.GetOutputPort())
view.SetVertexLabelArrayName("hostname")
view.SetVertexLabelVisibility(True)
view.SetEdgeLabelArrayName("port_protocol")
view.SetEdgeLabelVisibility(True)
view.SetColorEdges(True)
view.SetEdgeColorArrayName("Px|y(dport,protocol)")
view.SetLayoutStrategyToSimple2D()

theme = svtkViewTheme.CreateMellowTheme()
theme.SetLineWidth(5)
theme.SetCellHueRange(0,.667)
theme.SetCellAlphaRange(0.5,0.5)
theme.SetCellSaturationRange(1,1)
theme.SetCellValueRange(1,1)
theme.SetSelectedCellColor(1,0,1)
theme.SetSelectedPointColor(1,0,1)
view.ApplyViewTheme(theme)
view.SetVertexLabelFontSize(14)
view.SetEdgeLabelFontSize(12)
theme.FastDelete()

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()
view.GetInteractor().Start()
