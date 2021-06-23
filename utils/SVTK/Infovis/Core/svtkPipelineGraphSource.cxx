/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPipelineGraphSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPipelineGraphSource.h"
#include "svtkAbstractArray.h"
#include "svtkAlgorithmOutput.h"
#include "svtkAnnotationLink.h"
#include "svtkArrayData.h"
#include "svtkCollection.h"
#include "svtkDataSetAttributes.h"
#include "svtkEdgeListIterator.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTree.h"
#include "svtkVariantArray.h"

#include <map>
#include <sstream>

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

svtkStandardNewMacro(svtkPipelineGraphSource);

// ----------------------------------------------------------------------

svtkPipelineGraphSource::svtkPipelineGraphSource()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->Sinks = svtkCollection::New();
}

// ----------------------------------------------------------------------

svtkPipelineGraphSource::~svtkPipelineGraphSource()
{
  if (this->Sinks)
  {
    this->Sinks->Delete();
    this->Sinks = nullptr;
  }
}

// ----------------------------------------------------------------------

void svtkPipelineGraphSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------

void svtkPipelineGraphSource::AddSink(svtkObject* sink)
{
  if (sink != nullptr && !this->Sinks->IsItemPresent(sink))
  {
    this->Sinks->AddItem(sink);
    this->Modified();
  }
}

void svtkPipelineGraphSource::RemoveSink(svtkObject* sink)
{
  if (sink != nullptr && this->Sinks->IsItemPresent(sink))
  {
    this->Sinks->RemoveItem(sink);
    this->Modified();
  }
}

// ----------------------------------------------------------------------

static void InsertObject(svtkObject* object, std::map<svtkObject*, svtkIdType>& object_map,
  svtkMutableDirectedGraph* builder, svtkStringArray* vertex_class_name_array,
  svtkVariantArray* vertex_object_array, svtkStringArray* edge_output_port_array,
  svtkStringArray* edge_input_port_array, svtkStringArray* edge_class_name_array,
  svtkVariantArray* edge_object_array)
{
  if (!object)
    return;

  if (object_map.count(object))
    return;

  // Insert pipeline algorithms ...
  if (svtkAlgorithm* const algorithm = svtkAlgorithm::SafeDownCast(object))
  {
    object_map[algorithm] = builder->AddVertex();
    vertex_class_name_array->InsertNextValue(algorithm->GetClassName());
    vertex_object_array->InsertNextValue(algorithm);

    // Recursively insert algorithm inputs ...
    for (int i = 0; i != algorithm->GetNumberOfInputPorts(); ++i)
    {
      for (int j = 0; j != algorithm->GetNumberOfInputConnections(i); ++j)
      {
        svtkAlgorithm* const input_algorithm = algorithm->GetInputConnection(i, j)->GetProducer();
        InsertObject(input_algorithm, object_map, builder, vertex_class_name_array,
          vertex_object_array, edge_output_port_array, edge_input_port_array, edge_class_name_array,
          edge_object_array);

        builder->AddEdge(object_map[input_algorithm], object_map[algorithm]);

        svtkDataObject* input_data =
          input_algorithm->GetOutputDataObject(algorithm->GetInputConnection(i, j)->GetIndex());
        edge_output_port_array->InsertNextValue(
          svtkVariant(algorithm->GetInputConnection(i, j)->GetIndex()).ToString());
        edge_input_port_array->InsertNextValue(svtkVariant(i).ToString());
        edge_class_name_array->InsertNextValue(input_data ? input_data->GetClassName() : "");
        edge_object_array->InsertNextValue(input_data);
      }
    }
  }
}

int svtkPipelineGraphSource::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  // Setup the graph data structure ...
  SVTK_CREATE(svtkMutableDirectedGraph, builder);

  svtkStringArray* vertex_class_name_array = svtkStringArray::New();
  vertex_class_name_array->SetName("class_name");
  builder->GetVertexData()->AddArray(vertex_class_name_array);
  vertex_class_name_array->Delete();

  svtkVariantArray* vertex_object_array = svtkVariantArray::New();
  vertex_object_array->SetName("object");
  builder->GetVertexData()->AddArray(vertex_object_array);
  vertex_object_array->Delete();

  svtkStringArray* edge_output_port_array = svtkStringArray::New();
  edge_output_port_array->SetName("output_port");
  builder->GetEdgeData()->AddArray(edge_output_port_array);
  edge_output_port_array->Delete();

  svtkStringArray* edge_input_port_array = svtkStringArray::New();
  edge_input_port_array->SetName("input_port");
  builder->GetEdgeData()->AddArray(edge_input_port_array);
  edge_input_port_array->Delete();

  svtkStringArray* edge_class_name_array = svtkStringArray::New();
  edge_class_name_array->SetName("class_name");
  builder->GetEdgeData()->AddArray(edge_class_name_array);
  edge_class_name_array->Delete();

  svtkVariantArray* edge_object_array = svtkVariantArray::New();
  edge_object_array->SetName("object");
  builder->GetEdgeData()->AddArray(edge_object_array);
  edge_object_array->Delete();

  // Recursively insert pipeline components into the graph ...
  std::map<svtkObject*, svtkIdType> object_map;
  for (svtkIdType i = 0; i != this->Sinks->GetNumberOfItems(); ++i)
  {
    InsertObject(this->Sinks->GetItemAsObject(i), object_map, builder, vertex_class_name_array,
      vertex_object_array, edge_output_port_array, edge_input_port_array, edge_class_name_array,
      edge_object_array);
  }

  // Finish creating the output graph ...
  svtkDirectedGraph* const output_graph = svtkDirectedGraph::GetData(outputVector);
  if (!output_graph->CheckedShallowCopy(builder))
  {
    svtkErrorMacro(<< "Invalid graph structure");
    return 0;
  }

  return 1;
}

void svtkPipelineGraphSource::PipelineToDot(
  svtkAlgorithm* sink, ostream& output, const svtkStdString& graph_name)
{
  svtkSmartPointer<svtkCollection> sinks = svtkSmartPointer<svtkCollection>::New();
  sinks->AddItem(sink);

  PipelineToDot(sinks, output, graph_name);
}

namespace
{

void replace_all(std::string& str, const std::string& oldStr, const std::string& newStr)
{
  size_t pos = 0;
  while ((pos = str.find(oldStr, pos)) != std::string::npos)
  {
    str.replace(pos, oldStr.length(), newStr);
    pos += newStr.length();
  }
}

}

void svtkPipelineGraphSource::PipelineToDot(
  svtkCollection* sinks, ostream& output, const svtkStdString& graph_name)
{
  // Create a graph representation of the pipeline ...
  svtkSmartPointer<svtkPipelineGraphSource> pipeline = svtkSmartPointer<svtkPipelineGraphSource>::New();
  for (svtkIdType i = 0; i != sinks->GetNumberOfItems(); ++i)
  {
    pipeline->AddSink(sinks->GetItemAsObject(i));
  }
  pipeline->Update();
  svtkGraph* const pipeline_graph = pipeline->GetOutput();

  svtkAbstractArray* const vertex_object_array =
    pipeline_graph->GetVertexData()->GetAbstractArray("object");
  svtkAbstractArray* const edge_output_port_array =
    pipeline_graph->GetEdgeData()->GetAbstractArray("output_port");
  svtkAbstractArray* const edge_input_port_array =
    pipeline_graph->GetEdgeData()->GetAbstractArray("input_port");
  svtkAbstractArray* const edge_object_array =
    pipeline_graph->GetEdgeData()->GetAbstractArray("object");

  output << "digraph \"" << graph_name << "\"\n";
  output << "{\n";

  // Do some standard formatting ...
  output << "  node [ fontname=\"helvetica\" fontsize=\"10\" shape=\"record\" style=\"filled\" ]\n";
  output << "  edge [ fontname=\"helvetica\" fontsize=\"9\" ]\n\n";

  // Write-out vertices ...
  for (svtkIdType i = 0; i != pipeline_graph->GetNumberOfVertices(); ++i)
  {
    svtkObjectBase* const object = vertex_object_array->GetVariantValue(i).ToSVTKObject();

    std::stringstream buffer;
    object->PrintSelf(buffer, svtkIndent());

    std::string line;
    std::string object_state;
    while (std::getline(buffer, line))
    {
      replace_all(line, "\"", "'");
      replace_all(line, "\r", "");
      replace_all(line, "\n", "");

      if (0 == line.find("Debug:"))
        continue;
      if (0 == line.find("Modified Time:"))
        continue;
      if (0 == line.find("Reference Count:"))
        continue;
      if (0 == line.find("Registered Events:"))
        continue;
      if (0 == line.find("Executive:"))
        continue;
      if (0 == line.find("ErrorCode:"))
        continue;
      if (0 == line.find("Information:"))
        continue;
      if (0 == line.find("AbortExecute:"))
        continue;
      if (0 == line.find("Progress:"))
        continue;
      if (0 == line.find("Progress Text:"))
        continue;
      if (0 == line.find("  "))
        continue;

      object_state += line + "\\n";
    }

    std::string fillcolor = "#ccffcc";
    if (svtkAnnotationLink::SafeDownCast(object))
    {
      fillcolor = "#ccccff";
    }

    output << "  "
           << "node_" << object << " [ fillcolor=\"" << fillcolor << "\" label=\"{"
           << object->GetClassName() << "|" << object_state << "}\" svtk_class_name=\""
           << object->GetClassName() << "\" ]\n";
  }

  // Write-out edges ...
  svtkSmartPointer<svtkEdgeListIterator> edges = svtkSmartPointer<svtkEdgeListIterator>::New();
  edges->SetGraph(pipeline_graph);
  while (edges->HasNext())
  {
    svtkEdgeType edge = edges->Next();
    svtkObjectBase* const source = vertex_object_array->GetVariantValue(edge.Source).ToSVTKObject();
    svtkObjectBase* const target = vertex_object_array->GetVariantValue(edge.Target).ToSVTKObject();
    const svtkStdString output_port = edge_output_port_array->GetVariantValue(edge.Id).ToString();
    const svtkStdString input_port = edge_input_port_array->GetVariantValue(edge.Id).ToString();
    svtkObjectBase* const object = edge_object_array->GetVariantValue(edge.Id).ToSVTKObject();

    std::string color = "black";
    if (svtkTree::SafeDownCast(object))
    {
      color = "#00bb00";
    }
    else if (svtkTable::SafeDownCast(object))
    {
      color = "blue";
    }
    else if (svtkArrayData* const array_data = svtkArrayData::SafeDownCast(object))
    {
      if (array_data->GetNumberOfArrays())
      {
        color = "";
        for (svtkIdType i = 0; i != array_data->GetNumberOfArrays(); ++i)
        {
          if (i)
            color += ":";

          if (array_data->GetArray(i)->IsDense())
            color += "purple";
          else
            color += "red";
        }
      }
    }
    else if (svtkGraph::SafeDownCast(object))
    {
      color = "#cc6600";
    }

    output << "  "
           << "node_" << source << " -> "
           << "node_" << target;
    output << " [";
    output << " color=\"" << color << "\" fontcolor=\"" << color << "\"";
    output << " label=\"" << (object ? object->GetClassName() : "") << "\"";
    output << " headlabel=\"" << input_port << "\"";
    output << " taillabel=\"" << output_port << "\"";
    output << " ]\n";
  }

  output << "}\n";
}
