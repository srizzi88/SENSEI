#!/usr/bin/env python
from svtk import *

# Network Database
database = svtkSQLDatabase.CreateFromURL("psql://bnwylie@tlp-ds.sandia.gov:5432/sunburst")
database.Open("")


vertex_query_string = """
select d.ip, d.name, i.country_name,i.region_name,i.city_name,i.latitude, i.longitude
from  dnsnames d, ipligence i where ip4(d.ip)<<= ip_range;
"""

edge_query_string = """
select src, dst, dport from tcpsummary where dport != 80
"""

vertex_query = database.GetQueryInstance()
vertex_query.SetQuery(vertex_query_string)

edge_query = database.GetQueryInstance()
edge_query.SetQuery(edge_query_string)

vertex_table = svtkRowQueryToTable()
vertex_table.SetQuery(vertex_query)

edge_table = svtkRowQueryToTable()
edge_table.SetQuery(edge_query)



# Make a graph
graph = svtkTableToGraph()
graph.AddInputConnection(0,edge_table.GetOutputPort())
graph.AddInputConnection(1,vertex_table.GetOutputPort())
graph.AddLinkVertex("src", "ip", False)
graph.AddLinkVertex("dst", "ip", False)
graph.AddLinkEdge("src", "dst")


view = svtkGraphLayoutView()
view.AddRepresentationFromInputConnection(graph.GetOutputPort())
view.SetVertexLabelArrayName("ip")
view.SetVertexLabelVisibility(True)
view.SetEdgeColorArrayName("dport")
view.SetColorEdges(True)

theme = svtkViewTheme.CreateMellowTheme()
view.ApplyViewTheme(theme)

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()
view.GetInteractor().Start()
