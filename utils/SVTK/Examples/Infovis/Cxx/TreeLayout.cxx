/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TreeLayout.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example shows how to create a simple tree view from an XML file.
// You may specify the label array and color array from the command line.
//

#include "svtkActor.h"
#include "svtkActor2D.h"
#include "svtkDynamic2DLabelMapper.h"
#include "svtkGlyph3D.h"
#include "svtkGlyphSource2D.h"
#include "svtkGraphLayout.h"
#include "svtkGraphToPolyData.h"
#include "svtkInteractorStyleImage.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStringToNumeric.h"
#include "svtkTextProperty.h"
#include "svtkTreeLayoutStrategy.h"
#include "svtkXMLTreeReader.h"

void usage()
{
  cerr << endl;
  cerr << "usage: TreeLayout filename [label_attribute] [color_attribute]" << endl;
  cerr << "  filename is an xml file" << endl;
  cerr << "  label_attribute is the attribute to use as labels." << endl;
  cerr << "    Default is .tagname which labels using the element tag." << endl;
  cerr << "  color_attribute is the attribute to color by (numeric)." << endl;
  cerr << "    Default is no color." << endl;
}

int main(int argc, char* argv[])
{
  // Initialize parameters from the command line.
  const char* labelArray = ".tagname";
  const char* colorArray = nullptr;
  if (argc < 2)
  {
    usage();
    return 0;
  }
  char* filename = argv[1];
  if (argc >= 3)
  {
    labelArray = argv[2];
  }
  if (argc >= 4)
  {
    colorArray = argv[3];
  }

  // Read in the XML file into a tree.
  // This creates a tree with string columns for every attribute
  // present in the file, plus the special arrays named .tagname
  // (containing the XML tag name) and .chardata (containing the
  // character data within the tag).
  svtkXMLTreeReader* reader = svtkXMLTreeReader::New();
  reader->SetFileName(filename);

  // Automatically convert string columns containing numeric
  // values into integer and double arrays.
  svtkStringToNumeric* stringToNumeric = svtkStringToNumeric::New();
  stringToNumeric->SetInputConnection(reader->GetOutputPort());

  // Retrieve the tree from the pipeline so we can check whether
  // the specified label and color arrays exist.
  stringToNumeric->Update();
  svtkTree* tree = svtkTree::SafeDownCast(stringToNumeric->GetOutput());
  if (tree->GetVertexData()->GetAbstractArray(labelArray) == nullptr)
  {
    cerr << "ERROR: The label attribute " << labelArray << " is not defined in the file." << endl;
    reader->Delete();
    stringToNumeric->Delete();
    usage();
    return 0;
  }
  if (colorArray && tree->GetVertexData()->GetAbstractArray(colorArray) == nullptr)
  {
    cerr << "ERROR: The color attribute " << colorArray << " is not defined in the file." << endl;
    reader->Delete();
    stringToNumeric->Delete();
    usage();
    return 0;
  }
  if (colorArray &&
    svtkArrayDownCast<svtkDataArray>(tree->GetVertexData()->GetAbstractArray(colorArray)) == nullptr)
  {
    cerr << "ERROR: The color attribute " << colorArray << " does not have numeric values." << endl;
    reader->Delete();
    stringToNumeric->Delete();
    usage();
    return 0;
  }

  // If coloring the vertices, get the range of the color array.
  double colorRange[2] = { 0, 1 };
  if (colorArray)
  {
    svtkDataArray* color =
      svtkArrayDownCast<svtkDataArray>(tree->GetVertexData()->GetAbstractArray(colorArray));
    color->GetRange(colorRange);
  }

  // Layout the tree using svtkGraphLayout.
  svtkGraphLayout* layout = svtkGraphLayout::New();
  layout->SetInputConnection(stringToNumeric->GetOutputPort());

  // Specify that we want to use the tree layout strategy.
  svtkTreeLayoutStrategy* strategy = svtkTreeLayoutStrategy::New();
  strategy->RadialOn();      // Radial layout (as opposed to standard top-down layout)
  strategy->SetAngle(360.0); // The tree fills a full circular arc.
  layout->SetLayoutStrategy(strategy);

  // svtkGraphToPolyData converts a graph or tree to polydata.
  svtkGraphToPolyData* graphToPoly = svtkGraphToPolyData::New();
  graphToPoly->SetInputConnection(layout->GetOutputPort());

  // Create the standard SVTK polydata mapper and actor
  // for the connections (edges) in the tree.
  svtkPolyDataMapper* edgeMapper = svtkPolyDataMapper::New();
  edgeMapper->SetInputConnection(graphToPoly->GetOutputPort());
  svtkActor* edgeActor = svtkActor::New();
  edgeActor->SetMapper(edgeMapper);
  edgeActor->GetProperty()->SetColor(0.0, 0.5, 1.0);

  // Glyph the points of the tree polydata to create
  // SVTK_VERTEX cells at each vertex in the tree.
  svtkGlyph3D* vertGlyph = svtkGlyph3D::New();
  vertGlyph->SetInputConnection(0, graphToPoly->GetOutputPort());
  svtkGlyphSource2D* glyphSource = svtkGlyphSource2D::New();
  glyphSource->SetGlyphTypeToVertex();
  vertGlyph->SetInputConnection(1, glyphSource->GetOutputPort());

  // Create a mapper for the vertices, and tell the mapper
  // to use the specified color array.
  svtkPolyDataMapper* vertMapper = svtkPolyDataMapper::New();
  vertMapper->SetInputConnection(vertGlyph->GetOutputPort());
  if (colorArray)
  {
    vertMapper->SetScalarModeToUsePointFieldData();
    vertMapper->SelectColorArray(colorArray);
    vertMapper->SetScalarRange(colorRange);
  }

  // Create an actor for the vertices.  Move the actor forward
  // in the z direction so it is drawn on top of the edge actor.
  svtkActor* vertActor = svtkActor::New();
  vertActor->SetMapper(vertMapper);
  vertActor->GetProperty()->SetPointSize(5);
  vertActor->SetPosition(0, 0, 0.001);

  // Use a dynamic label mapper to draw the labels.  This mapper
  // does not allow labels to overlap, as long as the camera is
  // not rotated from pointing down the z axis.
  svtkDynamic2DLabelMapper* labelMapper = svtkDynamic2DLabelMapper::New();
  labelMapper->SetInputConnection(graphToPoly->GetOutputPort());
  labelMapper->GetLabelTextProperty()->SetJustificationToLeft();
  labelMapper->GetLabelTextProperty()->SetColor(0, 0, 0);
  if (labelArray)
  {
    labelMapper->SetLabelModeToLabelFieldData();
    labelMapper->SetFieldDataName(labelArray);
  }
  svtkActor2D* labelActor = svtkActor2D::New();
  labelActor->SetMapper(labelMapper);

  // Add the edges, vertices, and labels to the renderer.
  svtkRenderer* ren = svtkRenderer::New();
  ren->SetBackground(0.8, 0.8, 0.8);
  ren->AddActor(edgeActor);
  ren->AddActor(vertActor);
  ren->AddActor(labelActor);

  // Setup the render window and interactor.
  svtkRenderWindow* win = svtkRenderWindow::New();
  win->AddRenderer(ren);
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(win);

  // Constrain movement to zoom and pan using the image interactor style.
  svtkInteractorStyleImage* style = svtkInteractorStyleImage::New();
  iren->SetInteractorStyle(style);

  // Start the main application loop.
  iren->Initialize();
  iren->Start();

  // Clean up.
  style->Delete();
  iren->Delete();
  win->Delete();
  ren->Delete();
  labelActor->Delete();
  labelMapper->Delete();
  vertActor->Delete();
  vertMapper->Delete();
  glyphSource->Delete();
  vertGlyph->Delete();
  edgeMapper->Delete();
  edgeActor->Delete();
  graphToPoly->Delete();
  strategy->Delete();
  layout->Delete();
  stringToNumeric->Delete();
  reader->Delete();

  return 0;
}
