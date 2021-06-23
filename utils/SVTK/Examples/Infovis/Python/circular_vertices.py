#!/usr/bin/env python
"""
This file demonstrates drawing graphs using circular vertices via
svtkRenderedGraphRepresentation.
"""
from svtk import *

#------------------------------------------------------------------------------
# Script Entry Point
#------------------------------------------------------------------------------
if __name__ == "__main__":

    # Create a random graph
    source = svtkRandomGraphSource()
    source.SetNumberOfVertices(15)
    source.SetStartWithTree(True)

    #----------------------------------------------------------
    # Draw the graph in a window
    theme = svtkViewTheme.CreateMellowTheme()
    theme.SetLineWidth(4)
    theme.SetPointSize(15)
    theme.SetCellOpacity(1)
    theme.FastDelete()

    # Rendered graph representation to make vertices circles
    rep = svtkRenderedGraphRepresentation()
    rep.SetInputConnection(0, source.GetOutputPort())

    # svtkGraphToGlyph::CIRCLE == 7
    rep.SetGlyphType(7)

    # View containing the merged graph
    view = svtkGraphLayoutView()
    view.SetRepresentation( rep )
    view.SetVertexLabelArrayName("vertex id")
    view.SetVertexLabelVisibility(True)
    view.SetLayoutStrategyToSimple2D()
    view.ApplyViewTheme(theme)

    view.GetRenderWindow().SetSize(600, 600)
    view.ResetCamera()
    view.Render()

    view.GetInteractor().Start()
