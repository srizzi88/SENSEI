/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSimple3DCirclesStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSimple3DCirclesStrategy.h"

#include "svtkAbstractArray.h"
#include "svtkCharArray.h"       // For temporary store for point ordering
#include "svtkDirectedGraph.h"   // For this->Graph type check
#include "svtkIdTypeArray.h"     // For Ordered array
#include "svtkInEdgeIterator.h"  // For in edge(s) checks
#include "svtkIntArray.h"        // For hierarchy layers
#include "svtkMath.h"            // For cross, outer, norm and dot
#include "svtkObjectFactory.h"   // For SVTK ::New() function
#include "svtkOutEdgeIterator.h" // For out edge(s) checks
#include "svtkPoints.h"          // For output target
#include "svtkSmartPointer.h"    // For good memory handling

#include <cmath> // For abs, sin, cos and tan

#include <algorithm> // For min, max, swap, etc.
#include <list>      // For internal store

template <class T>
bool IsZero(T value)
{
  return ((value < SVTK_DBL_EPSILON) && (value > (-1.0 * SVTK_DBL_EPSILON)));
}

class svtkSimple3DCirclesStrategyInternal
{
public:
  svtkSimple3DCirclesStrategyInternal() = default;
  svtkSimple3DCirclesStrategyInternal(const svtkSimple3DCirclesStrategyInternal& from)
  {
    if (&from != this)
      this->store = from.store;
  };
  svtkSimple3DCirclesStrategyInternal& operator=(const svtkSimple3DCirclesStrategyInternal& from)
  {
    if (&from != this)
      this->store = from.store;
    return *this;
  };
  svtkSimple3DCirclesStrategyInternal& operator=(const std::list<svtkIdType>& from)
  {
    this->store = from;
    return *this;
  };
  svtkIdType front() { return this->store.front(); };
  void pop_front() { this->store.pop_front(); };
  std::size_t size() { return this->store.size(); };
  void push_back(const svtkIdType& value) { this->store.push_back(value); };
  ~svtkSimple3DCirclesStrategyInternal() { this->store.clear(); };

private:
  std::list<svtkIdType> store;
};

svtkStandardNewMacro(svtkSimple3DCirclesStrategy);

void svtkSimple3DCirclesStrategy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Radius : " << this->Radius << endl;

  os << indent << "Height : " << this->Height << endl;

  os << indent << "Origin  : (" << this->Origin[0] << "," << this->Origin[1] << ","
     << this->Origin[2] << ")" << endl;

  os << indent << "Direction  : (" << this->Direction[0] << "," << this->Direction[1] << ","
     << this->Direction[2] << ")" << endl;

  os << indent << "Rotate matrix : [[" << this->T[0][0] << ";" << this->T[1][0] << ";"
     << this->T[2][0] << "]";
  os << "[" << this->T[0][1] << ";" << this->T[1][1] << ";" << this->T[2][1] << "]";
  os << "[" << this->T[0][2] << ";" << this->T[1][2] << ";" << this->T[2][2] << "]]" << endl;

  os << indent << "Method : ";
  if (this->Method == FixedRadiusMethod)
    os << "fixed radius method" << endl;
  else if (this->Method == FixedDistanceMethod)
    os << "fixed distance method" << endl;

  os << indent << "MarkValue : " << this->MarkedValue << endl;

  os << indent << "Auto height : ";
  if (this->AutoHeight == 1)
    os << "On" << endl;
  else
    os << "Off" << endl;

  os << indent << "Minimum degree for autoheight : " << this->MinimumRadian << " rad ["
     << svtkMath::DegreesFromRadians(this->MinimumRadian) << " deg]" << endl;

  os << indent << "Registered MarkedStartPoints :";
  if (this->MarkedStartVertices == nullptr)
    os << " (none)" << endl;
  else
  {
    os << endl;
    this->MarkedStartVertices->PrintSelf(os, indent.GetNextIndent());
  }

  os << indent << "Registered HierarchicalLayers :";
  if (this->HierarchicalLayers == nullptr)
    os << " (none)" << endl;
  else
  {
    os << endl;
    this->HierarchicalLayers->PrintSelf(os, indent.GetNextIndent());
  }

  os << indent << "Registered HierarchicalOrder :";
  if (this->HierarchicalOrder == nullptr)
    os << " (none)" << endl;
  else
  {
    os << endl;
    this->HierarchicalOrder->PrintSelf(os, indent.GetNextIndent());
  }

  os << indent
     << "ForceToUseUniversalStartPointsFinder :" << this->ForceToUseUniversalStartPointsFinder
     << endl;
}

void svtkSimple3DCirclesStrategy::SetDirection(double dx, double dy, double dz)
{
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting Direction to (" << dx << ","
                << dy << "," << dz << ")");

  if ((this->Direction[0] != dx) || (this->Direction[1] != dy) || (this->Direction[2] != dz))
  {
    double global[3], local[3];
    global[0] = dx;
    global[1] = dy;
    global[2] = dz;

    local[0] = 0.0;
    local[1] = 1.0;
    local[2] = 0.0;

    double length_global = svtkMath::Norm(global);

    if (IsZero(length_global))
    {
      svtkWarningMacro(<< "The length of direction vector is zero! Direction has not been changed!");
      return;
    }

    double cosfi, n[3], E[3][3], U[3][3], u[3][3], number;

    global[0] = global[0] / length_global;
    global[1] = global[1] / length_global;
    global[2] = global[2] / length_global;

    // http://en.citizendium.org/wiki/Rotation_matrix
    // we are going from local to global.
    // cos(fi) = local.global -> cosfi, because |local|=1 and |global|=1
    cosfi = svtkMath::Dot(local, global);
    // if fi == 2*Pi -> cosfi = -1
    if (IsZero(cosfi + 1.0))
    {
      // if "local" is on "z" axes
      if (IsZero(local[2] + 1.0) || IsZero(local[2] - 1.0))
      {
        this->T[0][0] = this->T[2][2] = -1.0;
        this->T[1][1] = 1.0;
        this->T[0][1] = this->T[1][0] = this->T[0][2] = this->T[2][0] = this->T[1][2] =
          this->T[2][1] = 0.0;
      }
      // if local != ( (0,0,1) or (0,0,-1) )
      else
      {
        // n vector
        n[0] = 1.0 / (1.0 - local[2] * local[2]) * local[1];
        n[1] = -1.0 / (1.0 - local[2] * local[2]) * local[0];
        n[2] = 0.0;
        // u = n X n
        svtkMath::Outer(n, n, u);

        // -E
        E[0][0] = E[1][1] = E[2][2] = -1.0;
        E[0][1] = E[1][0] = E[0][2] = E[2][0] = E[1][2] = E[2][1] = 0.0;

        // T = -E + 2*u
        int i, j;
        for (i = 0; i < 3; ++i)
          for (j = 0; j < 3; ++j)
            this->T[i][j] = E[i][j] + (u[i][j] * 2.0);
      }
    }
    // fi < 2*Pi
    else
    {
      // n = local x global -> n(nx,ny,nz)
      svtkMath::Cross(local, global, n);
      //
      // cos(fi)*E
      //
      E[0][0] = E[1][1] = E[2][2] = cosfi;
      E[0][1] = E[1][0] = E[0][2] = E[2][0] = E[1][2] = E[2][1] = 0.0;
      //                 |  0  -nz  ny |
      // U = sin(fi)*N = |  nz  0  -nx |
      //                 | -ny  nx  0  |
      U[0][0] = U[1][1] = U[2][2] = 0.0;
      U[0][1] = -1.0 * n[2];
      U[1][0] = n[2];
      U[0][2] = n[1];
      U[2][0] = -1.0 * n[1];
      U[1][2] = -1.0 * n[0];
      U[2][1] = n[0];

      // u = n X n
      svtkMath::Outer(n, n, u);

      int i, j;
      // T = cos(fi)*E + U + 1/(1+cos(fi))*[n X n]
      // [ number = 1/(1+cos(fi)) ]
      number = 1.0 / (1.0 + cosfi);
      for (i = 0; i < 3; ++i)
        for (j = 0; j < 3; ++j)
          this->T[i][j] = E[i][j] + U[i][j] + (u[i][j] * number);
    }

    this->Direction[0] = dx;
    this->Direction[1] = dy;
    this->Direction[2] = dz;

    svtkDebugMacro(<< "Transformation matrix : [[" << this->T[0][0] << "," << this->T[1][0] << ","
                  << this->T[2][0] << "][" << this->T[0][1] << "," << this->T[1][1] << ","
                  << this->T[2][1] << "][" << this->T[0][2] << "," << this->T[1][2] << ","
                  << this->T[2][2] << "]]");

    this->Modified();
  }
}

void svtkSimple3DCirclesStrategy::SetDirection(double d[3])
{
  this->SetDirection(d[0], d[1], d[2]);
}

svtkCxxSetObjectMacro(svtkSimple3DCirclesStrategy, MarkedStartVertices, svtkAbstractArray);

void svtkSimple3DCirclesStrategy::SetMarkedValue(svtkVariant val)
{
  if (!this->MarkedValue.IsEqual(val))
  {
    this->MarkedValue = val;
    svtkDebugMacro(<< "Setting MarkedValue : " << this->MarkedValue);
    this->Modified();
  }
}

svtkVariant svtkSimple3DCirclesStrategy::GetMarkedValue()
{
  return this->MarkedValue;
}

void svtkSimple3DCirclesStrategy::SetMinimumDegree(double degree)
{
  this->SetMinimumRadian(svtkMath::RadiansFromDegrees(degree));
}

double svtkSimple3DCirclesStrategy::GetMinimumDegree()
{
  return svtkMath::DegreesFromRadians(this->GetMinimumRadian());
}

svtkCxxSetObjectMacro(svtkSimple3DCirclesStrategy, HierarchicalLayers, svtkIntArray);
svtkCxxSetObjectMacro(svtkSimple3DCirclesStrategy, HierarchicalOrder, svtkIdTypeArray);

svtkSimple3DCirclesStrategy::svtkSimple3DCirclesStrategy()
  : Radius(1)
  , Height(1)
  , Method(FixedRadiusMethod)
  , MarkedStartVertices(nullptr)
  , ForceToUseUniversalStartPointsFinder(0)
  , AutoHeight(0)
  , MinimumRadian(svtkMath::Pi() / 6.0)
  , HierarchicalLayers(nullptr)
  , HierarchicalOrder(nullptr)
{
  this->Direction[0] = this->Direction[1] = 0.0;
  this->Direction[2] = 1.0;
  this->T[0][1] = this->T[0][2] = this->T[1][2] = 0.0;
  this->T[1][0] = this->T[2][0] = this->T[2][1] = 0.0;
  this->T[0][0] = this->T[1][1] = this->T[2][2] = 1.0;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
}

svtkSimple3DCirclesStrategy::~svtkSimple3DCirclesStrategy()
{
  this->SetMarkedStartVertices(nullptr);
  this->SetHierarchicalLayers(nullptr);
  this->SetHierarchicalOrder(nullptr);
}

void svtkSimple3DCirclesStrategy::Layout()
{
  if (this->Graph == nullptr)
  {
    svtkErrorMacro(<< "Graph is null!");
    return;
  }
  if (this->Graph->GetNumberOfVertices() == 0)
  {
    svtkDebugMacro(<< "Graph is empty (no no vertices)!");
    return;
  }

  svtkSmartPointer<svtkDirectedGraph> target = svtkSmartPointer<svtkDirectedGraph>::New();
  if (!target->CheckedShallowCopy(this->Graph))
  {
    svtkErrorMacro(<< "Graph must be directed graph!");
    return;
  }

  svtkSimple3DCirclesStrategyInternal start_points, order_points, stand_alones;

  // Layers begin
  svtkSmartPointer<svtkIntArray> layers = nullptr;
  if (this->HierarchicalLayers != nullptr)
  {
    if ((this->HierarchicalLayers->GetMaxId() + 1) == target->GetNumberOfVertices())
    {
      layers = this->HierarchicalLayers;
    }
  }
  if (layers == nullptr)
  {
    layers = svtkSmartPointer<svtkIntArray>::New();
    if (this->HierarchicalLayers != nullptr)
      this->HierarchicalLayers->UnRegister(this);
    this->HierarchicalLayers = layers;
    this->HierarchicalLayers->Register(this);

    layers->SetNumberOfValues(target->GetNumberOfVertices());
    for (svtkIdType i = 0; i <= layers->GetMaxId(); ++i)
      layers->SetValue(i, -1);

    if (this->UniversalStartPoints(target, &start_points, &stand_alones, layers) == -1)
    {
      svtkErrorMacro(<< "There is no start point!");
      return;
    }
    order_points = start_points;
    this->BuildLayers(target, &start_points, layers);
  }
  else
  {
    for (svtkIdType i = 0; i <= layers->GetMaxId(); ++i)
    {
      if (layers->GetValue(i) == 0)
        order_points.push_back(i);
      else if (layers->GetValue(i) == -2)
        stand_alones.push_back(i);
    }
  }
  // Layers end

  // Order begin
  svtkSmartPointer<svtkIdTypeArray> order = nullptr;
  if (this->HierarchicalOrder != nullptr)
  {
    if ((this->HierarchicalOrder->GetMaxId() + 1) == target->GetNumberOfVertices())
    {
      order = this->HierarchicalOrder;
    }
  }

  if (order == nullptr)
  {
    order = svtkSmartPointer<svtkIdTypeArray>::New();
    if (this->HierarchicalOrder != nullptr)
      this->HierarchicalOrder->UnRegister(this);
    this->HierarchicalOrder = order;
    this->HierarchicalOrder->Register(this);
    order->SetNumberOfValues(target->GetNumberOfVertices());
    for (svtkIdType i = 0; i <= order->GetMaxId(); ++i)
      order->SetValue(i, -1);

    this->BuildPointOrder(target, &order_points, &stand_alones, layers, order);
  }
  // Order end

  if (order->GetValue(order->GetMaxId()) == -1)
  {
    svtkErrorMacro(<< "Not all parts of the graph is accessible. There may be a loop.");
    return;
  }

  int index = 0;
  int layer = 0;
  int start = 0;
  double R = this->Radius;
  double Rprev = 0.0;
  double localXYZ[3], globalXYZ[3], localHeight = this->Height;
  double alfa = 0.0;
  double tangent = tan(svtkMath::Pi() / 2 - this->MinimumRadian);
  int ind = 0;

  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  points->SetNumberOfPoints(target->GetNumberOfVertices());

  while (index <= order->GetMaxId())
  {
    start = index;
    R = this->Radius;
    layer = layers->GetValue(order->GetValue(index));
    while (index <= order->GetMaxId())
    {
      if (layers->GetValue(order->GetValue(index)) == layer)
        ++index;
      else
        break;
    }

    alfa = 2.0 * svtkMath::Pi() / double(index - start);

    if (this->Method == FixedDistanceMethod)
    {
      R = double(index - start - 1) * this->Radius / svtkMath::Pi();
    }
    else if (this->Method == FixedRadiusMethod)
    {
      if ((index - start) == 1)
        R = 0.0;
    }
    else
    {
      svtkErrorMacro(<< "Method must be FixedRadiusMethod or FixedDistanceMethod!");
      return;
    }

    if ((this->AutoHeight == 1) && (this->Method == FixedDistanceMethod))
    {
      if (fabs(tangent * (R - Rprev)) > this->Height)
        localHeight = fabs(tangent * (R - Rprev));
      else
        localHeight = this->Height;
    }

    if (layer != 0)
      localXYZ[2] = localXYZ[2] + localHeight;
    else
      localXYZ[2] = 0.0;

    for (ind = start; ind < index; ++ind)
    {
      localXYZ[0] = R * cos(double(ind - start) * alfa);
      localXYZ[1] = R * sin(double(ind - start) * alfa);
      this->Transform(localXYZ, globalXYZ);
      points->SetPoint(order->GetValue(ind), globalXYZ);
    }

    Rprev = R;
  }

  this->Graph->SetPoints(points);
  svtkDebugMacro(<< "svtkPoints is added to the graph. Vertex layout is ready.");
}

void svtkSimple3DCirclesStrategy::SetGraph(svtkGraph* graph)
{
  if (this->Graph != graph)
  {
    this->Superclass::SetGraph(graph);
    if (this->HierarchicalLayers != nullptr)
    {
      this->HierarchicalLayers->UnRegister(this);
      this->HierarchicalLayers = nullptr;
    }
    if (this->HierarchicalOrder != nullptr)
    {
      this->HierarchicalOrder->UnRegister(this);
      this->HierarchicalOrder = nullptr;
    }
  }
}

int svtkSimple3DCirclesStrategy::UniversalStartPoints(svtkDirectedGraph* input,
  svtkSimple3DCirclesStrategyInternal* target, svtkSimple3DCirclesStrategyInternal* StandAlones,
  svtkIntArray* layers)
{
  if ((this->MarkedStartVertices != nullptr) && (this->ForceToUseUniversalStartPointsFinder == 0))
  {
    if (this->MarkedStartVertices->GetMaxId() == layers->GetMaxId())
    {
      for (svtkIdType i = 0; i < input->GetNumberOfVertices(); ++i)
      {
        if ((input->GetInDegree(i) == 0) && (input->GetOutDegree(i) > 0))
        {
          target->push_back(i);
          layers->SetValue(i, 0);
        }
        else if ((input->GetInDegree(i) == 0) && (input->GetOutDegree(i) == 0))
        {
          layers->SetValue(i, -2);
          StandAlones->push_back(i);
        }
        else if ((this->MarkedStartVertices->GetVariantValue(i) == this->MarkedValue) &&
          (input->GetOutDegree(i) > 0))
        {
          target->push_back(i);
          layers->SetValue(i, 0);
        }
      }

      svtkDebugMacro(
        << "StartPoint finder: Universal start point finder was used. Number of start point(s): "
        << target->size() << "; Number of stand alone point(s): " << StandAlones->size());
      return static_cast<int>(target->size());
    }
    else
    {
      svtkErrorMacro(<< "MarkedStartPoints number is NOT equal number of vertices!");
      return -1;
    }
  }

  for (svtkIdType i = 0; i < input->GetNumberOfVertices(); ++i)
  {
    if ((input->GetInDegree(i) == 0) && (input->GetOutDegree(i) > 0))
    {
      target->push_back(i);
      layers->SetValue(i, 0);
    }
    else if ((input->GetInDegree(i) == 0) && (input->GetOutDegree(i) == 0))
    {
      layers->SetValue(i, -2);
      StandAlones->push_back(i);
    }
  }

  svtkDebugMacro(
    << "StartPoint finder: Universal start point finder was used. Number of start points: "
    << target->size() << "; Number of stand alone point(s): " << StandAlones->size());
  return static_cast<int>(target->size());
}

int svtkSimple3DCirclesStrategy::BuildLayers(
  svtkDirectedGraph* input, svtkSimple3DCirclesStrategyInternal* source, svtkIntArray* layers)
{
  svtkSmartPointer<svtkOutEdgeIterator> edge_out_iterator =
    svtkSmartPointer<svtkOutEdgeIterator>::New();
  svtkSmartPointer<svtkInEdgeIterator> edge_in_iterator = svtkSmartPointer<svtkInEdgeIterator>::New();
  int layer = 0, flayer = 0;
  svtkInEdgeType in_edge;
  svtkOutEdgeType out_edge;
  bool HasAllInput = true;
  svtkIdType ID = 0;
  int max_layer_id = -1;

  while (source->size() > 0)
  {
    ID = source->front();
    source->pop_front();

    input->GetOutEdges(ID, edge_out_iterator);

    while (edge_out_iterator->HasNext())
    {
      out_edge = edge_out_iterator->Next();
      if (layers->GetValue(out_edge.Target) == -1)
      {
        input->GetInEdges(out_edge.Target, edge_in_iterator);
        layer = layers->GetValue(ID);
        HasAllInput = true;

        while (edge_in_iterator->HasNext() && HasAllInput)
        {
          in_edge = edge_in_iterator->Next();
          flayer = layers->GetValue(in_edge.Source);
          if (flayer == -1)
            HasAllInput = false;
          layer = std::max(layer, flayer);
        }

        if (HasAllInput)
        {
          source->push_back(out_edge.Target);
          layers->SetValue(out_edge.Target, layer + 1);
          max_layer_id = std::max(max_layer_id, layer + 1);
        }
      }
    }
  }

  svtkDebugMacro(<< "Layer building is successful.");
  return max_layer_id;
}

void svtkSimple3DCirclesStrategy::BuildPointOrder(svtkDirectedGraph* input,
  svtkSimple3DCirclesStrategyInternal* source, svtkSimple3DCirclesStrategyInternal* StandAlones,
  svtkIntArray* layers, svtkIdTypeArray* order)
{
  svtkSmartPointer<svtkOutEdgeIterator> edge_out_iterator =
    svtkSmartPointer<svtkOutEdgeIterator>::New();
  svtkSmartPointer<svtkCharArray> mark = svtkSmartPointer<svtkCharArray>::New();
  svtkOutEdgeType out_edge;
  int step = 0;
  int layer = 0;
  svtkIdType ID = 0;

  mark->SetNumberOfValues(input->GetNumberOfVertices());
  for (svtkIdType i = 0; i <= mark->GetMaxId(); ++i)
    mark->SetValue(i, 0);

  while (source->size() > 0)
  {
    ID = source->front();
    source->pop_front();

    order->SetValue(step, ID);
    input->GetOutEdges(ID, edge_out_iterator);
    layer = layers->GetValue(ID) + 1;

    while (edge_out_iterator->HasNext())
    {
      out_edge = edge_out_iterator->Next();
      if (mark->GetValue(out_edge.Target) == 0)
      {
        if (layers->GetValue(out_edge.Target) == layer)
        {
          mark->SetValue(out_edge.Target, 1);
          source->push_back(out_edge.Target);
        }
      }
    }

    ++step;
  }

  while (StandAlones->size() > 0)
  {
    order->SetValue(step, StandAlones->front());
    StandAlones->pop_front();
    ++step;
  }

  svtkDebugMacro(<< "Vertex order building is successful.");
}

void svtkSimple3DCirclesStrategy::Transform(double Local[], double Global[])
{
  svtkMath::Multiply3x3(this->T, Local, Global);
  Global[0] = this->Origin[0] + Global[0];
  Global[1] = this->Origin[1] + Global[1];
  Global[2] = this->Origin[2] + Global[2];
}
