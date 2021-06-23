#!/usr/bin/env python
"""
This file demonstrates the creation of a tree using the
Python interface to SVTK.
"""

from __future__ import print_function
from svtk import *

#------------------------------------------------------------------------------
# Script Entry Point
#------------------------------------------------------------------------------
if __name__ == "__main__":

    print("svtkTree Example 1: Building a tree from scratch.")

    # Create an empty graph
    G = svtkMutableDirectedGraph()

    vertID = svtkIntArray()
    vertID.SetName("ID")

    G.GetVertexData().AddArray( vertID )

    # Add a root vertex
    root = G.AddVertex()
    vertID.InsertNextValue(root)

    # Add some vertices
    for i in range(3):
        v = G.AddChild(root)
        vertID.InsertNextValue(v)
        for j in range(2):
            u = G.AddChild(v)
            vertID.InsertNextValue(u)

    T = svtkTree()
    T.ShallowCopy(G)


    #----------------------------------------------------------
    # Draw the graph in a window
    view = svtkGraphLayoutView()
    view.AddRepresentationFromInput(G)
    view.SetVertexLabelArrayName("ID")
    view.SetVertexLabelVisibility(True)
    view.SetLayoutStrategyToTree()
    view.SetVertexLabelFontSize(20)

    theme = svtkViewTheme.CreateMellowTheme()
    theme.SetLineWidth(4)
    theme.SetPointSize(10)
    theme.SetCellOpacity(1)
    view.ApplyViewTheme(theme)
    theme.FastDelete()

    view.GetRenderWindow().SetSize(600, 600)
    view.ResetCamera()
    view.Render()

    view.GetInteractor().Start()
    print("svtkTree Example 1: Finished.")
