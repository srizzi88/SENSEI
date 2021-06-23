/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostGraphAdapter.h

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
/**
 * @class   svtkDirectedGraph
 *
 *
 * Including this header allows you to use a svtkDirectedGraph* in boost algorithms.
 * To do this, first wrap the class in a svtkDirectedGraph* or
 * svtkUndirectedGraph* depending on whether your graph is directed or undirected.
 * You may then use these objects directly in boost graph algorithms.
 */

#ifndef svtkBoostGraphAdapter_h
#define svtkBoostGraphAdapter_h

#include "svtkAbstractArray.h"
#include "svtkDataArray.h"
#include "svtkDataObject.h"
#include "svtkDirectedGraph.h"
#include "svtkDistributedGraphHelper.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkIntArray.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkTree.h"
#include "svtkUndirectedGraph.h"
#include "svtkVariant.h"

#include <boost/version.hpp>

namespace boost
{
//===========================================================================
// SVTK arrays as property maps
// These need to be defined before including other boost stuff

// Forward declarations are required here, so that we aren't forced
// to include boost/property_map.hpp.
template <typename>
struct property_traits;
struct read_write_property_map_tag;

#define svtkPropertyMapMacro(T, V)                                                                  \
  template <>                                                                                      \
  struct property_traits<T*>                                                                       \
  {                                                                                                \
    typedef V value_type;                                                                          \
    typedef V reference;                                                                           \
    typedef svtkIdType key_type;                                                                    \
    typedef read_write_property_map_tag category;                                                  \
  };                                                                                               \
                                                                                                   \
  inline property_traits<T*>::reference get(T* const& arr, property_traits<T*>::key_type key)      \
  {                                                                                                \
    return arr->GetValue(key);                                                                     \
  }                                                                                                \
                                                                                                   \
  inline void put(                                                                                 \
    T* arr, property_traits<T*>::key_type key, const property_traits<T*>::value_type& value)       \
  {                                                                                                \
    arr->InsertValue(key, value);                                                                  \
  }

svtkPropertyMapMacro(svtkIntArray, int);
svtkPropertyMapMacro(svtkIdTypeArray, svtkIdType);
svtkPropertyMapMacro(svtkDoubleArray, double);
svtkPropertyMapMacro(svtkFloatArray, float);

// svtkDataArray
template <>
struct property_traits<svtkDataArray*>
{
  typedef double value_type;
  typedef double reference;
  typedef svtkIdType key_type;
  typedef read_write_property_map_tag category;
};

inline double get(svtkDataArray* const& arr, svtkIdType key)
{
  return arr->GetTuple1(key);
}

inline void put(svtkDataArray* arr, svtkIdType key, const double& value)
{
  arr->SetTuple1(key, value);
}

// svtkAbstractArray as a property map of svtkVariants
template <>
struct property_traits<svtkAbstractArray*>
{
  typedef svtkVariant value_type;
  typedef svtkVariant reference;
  typedef svtkIdType key_type;
  typedef read_write_property_map_tag category;
};

inline svtkVariant get(svtkAbstractArray* const& arr, svtkIdType key)
{
  return arr->GetVariantValue(key);
}

inline void put(svtkAbstractArray* arr, svtkIdType key, const svtkVariant& value)
{
  arr->InsertVariantValue(key, value);
}
#if defined(_MSC_VER)
namespace detail
{
using ::boost::get;
using ::boost::put;
}
#endif
}

#include <utility> // STL Header

#include <boost/config.hpp>
#include <boost/graph/adjacency_iterator.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/iterator/iterator_facade.hpp>

// The functions and classes in this file allows the user to
// treat a svtkDirectedGraph or svtkUndirectedGraph object
// as a boost graph "as is".

namespace boost
{

class svtk_vertex_iterator
  : public iterator_facade<svtk_vertex_iterator, svtkIdType, bidirectional_traversal_tag,
      const svtkIdType&, svtkIdType>
{
public:
  explicit svtk_vertex_iterator(svtkIdType i = 0)
    : index(i)
  {
  }

private:
  const svtkIdType& dereference() const { return index; }

  bool equal(const svtk_vertex_iterator& other) const { return index == other.index; }

  void increment() { index++; }
  void decrement() { index--; }

  svtkIdType index;

  friend class iterator_core_access;
};

class svtk_edge_iterator
  : public iterator_facade<svtk_edge_iterator, svtkEdgeType, forward_traversal_tag,
      const svtkEdgeType&, svtkIdType>
{
public:
  explicit svtk_edge_iterator(svtkGraph* g = 0, svtkIdType v = 0)
    : directed(false)
    , vertex(v)
    , lastVertex(v)
    , iter(nullptr)
    , end(nullptr)
    , graph(g)
  {
    if (graph)
    {
      lastVertex = graph->GetNumberOfVertices();
    }

    svtkIdType myRank = -1;
    svtkDistributedGraphHelper* helper = this->graph ? this->graph->GetDistributedGraphHelper() : 0;
    if (helper)
    {
      myRank = this->graph->GetInformation()->Get(svtkDataObject::DATA_PIECE_NUMBER());
      vertex = helper->MakeDistributedId(myRank, vertex);
      lastVertex = helper->MakeDistributedId(myRank, lastVertex);
    }

    if (graph != 0)
    {
      directed = (svtkDirectedGraph::SafeDownCast(graph) != 0);
      while (vertex < lastVertex && this->graph->GetOutDegree(vertex) == 0)
      {
        ++vertex;
      }

      if (vertex < lastVertex)
      {
        // Get the outgoing edges of the first vertex that has outgoing
        // edges
        svtkIdType nedges;
        graph->GetOutEdges(vertex, iter, nedges);
        if (iter)
        {
          end = iter + nedges;

          if (!directed)
          {
            while ( // Skip non-local edges
              (helper && helper->GetEdgeOwner(iter->Id) != myRank)
              // Skip entirely-local edges where Source > Target
              || (((helper && myRank == helper->GetVertexOwner(iter->Target)) || !helper) &&
                   vertex > iter->Target))
            {
              this->inc();
            }
          }
        }
      }
      else
      {
        iter = nullptr;
      }
    }

    RecalculateEdge();
  }

private:
  const svtkEdgeType& dereference() const
  {
    assert(iter);
    return edge;
  }

  bool equal(const svtk_edge_iterator& other) const
  {
    return vertex == other.vertex && iter == other.iter;
  }

  void increment()
  {
    inc();
    if (!directed)
    {
      svtkIdType myRank = -1;
      svtkDistributedGraphHelper* helper =
        this->graph ? this->graph->GetDistributedGraphHelper() : 0;
      if (helper)
      {
        myRank = this->graph->GetInformation()->Get(svtkDataObject::DATA_PIECE_NUMBER());
      }

      while (iter != 0 &&
        ( // Skip non-local edges
          (helper && helper->GetEdgeOwner(iter->Id) != myRank)
          // Skip entirely-local edges where Source > Target
          || (((helper && myRank == helper->GetVertexOwner(iter->Target)) || !helper) &&
               vertex > iter->Target)))
      {
        inc();
      }
    }
    RecalculateEdge();
  }

  void inc()
  {
    ++iter;
    if (iter == end)
    {
      // Find a vertex with nonzero out degree.
      ++vertex;
      while (vertex < lastVertex && this->graph->GetOutDegree(vertex) == 0)
      {
        ++vertex;
      }

      if (vertex < lastVertex)
      {
        svtkIdType nedges;
        graph->GetOutEdges(vertex, iter, nedges);
        end = iter + nedges;
      }
      else
      {
        iter = nullptr;
      }
    }
  }

  void RecalculateEdge()
  {
    if (iter)
    {
      edge = svtkEdgeType(vertex, iter->Target, iter->Id);
    }
  }

  bool directed;
  svtkIdType vertex;
  svtkIdType lastVertex;
  const svtkOutEdgeType* iter;
  const svtkOutEdgeType* end;
  svtkGraph* graph;
  svtkEdgeType edge;

  friend class iterator_core_access;
};

class svtk_out_edge_pointer_iterator
  : public iterator_facade<svtk_out_edge_pointer_iterator, svtkEdgeType, bidirectional_traversal_tag,
      const svtkEdgeType&, ptrdiff_t>
{
public:
  explicit svtk_out_edge_pointer_iterator(svtkGraph* g = 0, svtkIdType v = 0, bool end = false)
    : vertex(v)
    , iter(nullptr)
  {
    if (g)
    {
      svtkIdType nedges;
      g->GetOutEdges(vertex, iter, nedges);
      if (end)
      {
        iter += nedges;
      }
    }
    RecalculateEdge();
  }

private:
  const svtkEdgeType& dereference() const
  {
    assert(iter);
    return edge;
  }

  bool equal(const svtk_out_edge_pointer_iterator& other) const { return iter == other.iter; }

  void increment()
  {
    iter++;
    RecalculateEdge();
  }

  void decrement()
  {
    iter--;
    RecalculateEdge();
  }

  void RecalculateEdge()
  {
    if (iter)
    {
      edge = svtkEdgeType(vertex, iter->Target, iter->Id);
    }
  }

  svtkIdType vertex;
  const svtkOutEdgeType* iter;
  svtkEdgeType edge;

  friend class iterator_core_access;
};

class svtk_in_edge_pointer_iterator
  : public iterator_facade<svtk_in_edge_pointer_iterator, svtkEdgeType, bidirectional_traversal_tag,
      const svtkEdgeType&, ptrdiff_t>
{
public:
  explicit svtk_in_edge_pointer_iterator(svtkGraph* g = 0, svtkIdType v = 0, bool end = false)
    : vertex(v)
    , iter(nullptr)
  {
    if (g)
    {
      svtkIdType nedges;
      g->GetInEdges(vertex, iter, nedges);
      if (end)
      {
        iter += nedges;
      }
    }
    RecalculateEdge();
  }

private:
  const svtkEdgeType& dereference() const
  {
    assert(iter);
    return edge;
  }

  bool equal(const svtk_in_edge_pointer_iterator& other) const { return iter == other.iter; }

  void increment()
  {
    iter++;
    RecalculateEdge();
  }

  void decrement()
  {
    iter--;
    RecalculateEdge();
  }

  void RecalculateEdge()
  {
    if (iter)
    {
      edge = svtkEdgeType(iter->Source, vertex, iter->Id);
    }
  }

  svtkIdType vertex;
  const svtkInEdgeType* iter;
  svtkEdgeType edge;

  friend class iterator_core_access;
};

//===========================================================================
// svtkGraph
// VertexAndEdgeListGraphConcept
// BidirectionalGraphConcept
// AdjacencyGraphConcept

struct svtkGraph_traversal_category
  : public virtual bidirectional_graph_tag
  , public virtual edge_list_graph_tag
  , public virtual vertex_list_graph_tag
  , public virtual adjacency_graph_tag
{
};

template <>
struct graph_traits<svtkGraph*>
{
  typedef svtkIdType vertex_descriptor;
  static vertex_descriptor null_vertex() { return -1; }
  typedef svtkEdgeType edge_descriptor;
  static edge_descriptor null_edge() { return svtkEdgeType(-1, -1, -1); }
  typedef svtk_out_edge_pointer_iterator out_edge_iterator;
  typedef svtk_in_edge_pointer_iterator in_edge_iterator;

  typedef svtk_vertex_iterator vertex_iterator;
  typedef svtk_edge_iterator edge_iterator;

  typedef allow_parallel_edge_tag edge_parallel_category;
  typedef svtkGraph_traversal_category traversal_category;
  typedef svtkIdType vertices_size_type;
  typedef svtkIdType edges_size_type;
  typedef svtkIdType degree_size_type;

  typedef adjacency_iterator_generator<svtkGraph*, vertex_descriptor, out_edge_iterator>::type
    adjacency_iterator;
};

#if BOOST_VERSION >= 104500
template <>
struct graph_property_type<svtkGraph*>
{
  typedef no_property type;
};
#endif

template <>
struct vertex_property_type<svtkGraph*>
{
  typedef no_property type;
};

template <>
struct edge_property_type<svtkGraph*>
{
  typedef no_property type;
};

#if BOOST_VERSION >= 104500
template <>
struct graph_bundle_type<svtkGraph*>
{
  typedef no_property type;
};
#endif

template <>
struct vertex_bundle_type<svtkGraph*>
{
  typedef no_property type;
};

template <>
struct edge_bundle_type<svtkGraph*>
{
  typedef no_property type;
};

inline bool has_no_edges(svtkGraph* g)
{
  return ((g->GetNumberOfEdges() > 0) ? false : true);
}

inline void remove_edge(graph_traits<svtkGraph*>::edge_descriptor e, svtkGraph* g)
{
  if (svtkMutableDirectedGraph::SafeDownCast(g))
  {
    svtkMutableDirectedGraph::SafeDownCast(g)->RemoveEdge(e.Id);
  }
  else if (svtkMutableUndirectedGraph::SafeDownCast(g))
  {
    svtkMutableUndirectedGraph::SafeDownCast(g)->RemoveEdge(e.Id);
  }
}

//===========================================================================
// svtkDirectedGraph

template <>
struct graph_traits<svtkDirectedGraph*> : graph_traits<svtkGraph*>
{
  typedef directed_tag directed_category;
};

// The graph_traits for a const graph are the same as a non-const graph.
template <>
struct graph_traits<const svtkDirectedGraph*> : graph_traits<svtkDirectedGraph*>
{
};

// The graph_traits for a const graph are the same as a non-const graph.
template <>
struct graph_traits<svtkDirectedGraph* const> : graph_traits<svtkDirectedGraph*>
{
};

#if BOOST_VERSION >= 104500
// Internal graph properties
template <>
struct graph_property_type<svtkDirectedGraph*> : graph_property_type<svtkGraph*>
{
};

// Internal graph properties
template <>
struct graph_property_type<svtkDirectedGraph* const> : graph_property_type<svtkGraph*>
{
};
#endif

// Internal vertex properties
template <>
struct vertex_property_type<svtkDirectedGraph*> : vertex_property_type<svtkGraph*>
{
};

// Internal vertex properties
template <>
struct vertex_property_type<svtkDirectedGraph* const> : vertex_property_type<svtkGraph*>
{
};

// Internal edge properties
template <>
struct edge_property_type<svtkDirectedGraph*> : edge_property_type<svtkGraph*>
{
};

// Internal edge properties
template <>
struct edge_property_type<svtkDirectedGraph* const> : edge_property_type<svtkGraph*>
{
};

#if BOOST_VERSION >= 104500
// Internal graph properties
template <>
struct graph_bundle_type<svtkDirectedGraph*> : graph_bundle_type<svtkGraph*>
{
};

// Internal graph properties
template <>
struct graph_bundle_type<svtkDirectedGraph* const> : graph_bundle_type<svtkGraph*>
{
};
#endif

// Internal vertex properties
template <>
struct vertex_bundle_type<svtkDirectedGraph*> : vertex_bundle_type<svtkGraph*>
{
};

// Internal vertex properties
template <>
struct vertex_bundle_type<svtkDirectedGraph* const> : vertex_bundle_type<svtkGraph*>
{
};

// Internal edge properties
template <>
struct edge_bundle_type<svtkDirectedGraph*> : edge_bundle_type<svtkGraph*>
{
};

// Internal edge properties
template <>
struct edge_bundle_type<svtkDirectedGraph* const> : edge_bundle_type<svtkGraph*>
{
};

//===========================================================================
// svtkTree

template <>
struct graph_traits<svtkTree*> : graph_traits<svtkDirectedGraph*>
{
};

// The graph_traits for a const graph are the same as a non-const graph.
template <>
struct graph_traits<const svtkTree*> : graph_traits<svtkTree*>
{
};

// The graph_traits for a const graph are the same as a non-const graph.
template <>
struct graph_traits<svtkTree* const> : graph_traits<svtkTree*>
{
};

//===========================================================================
// svtkUndirectedGraph
template <>
struct graph_traits<svtkUndirectedGraph*> : graph_traits<svtkGraph*>
{
  typedef undirected_tag directed_category;
};

// The graph_traits for a const graph are the same as a non-const graph.
template <>
struct graph_traits<const svtkUndirectedGraph*> : graph_traits<svtkUndirectedGraph*>
{
};

// The graph_traits for a const graph are the same as a non-const graph.
template <>
struct graph_traits<svtkUndirectedGraph* const> : graph_traits<svtkUndirectedGraph*>
{
};

#if BOOST_VERSION >= 104500
// Internal graph properties
template <>
struct graph_property_type<svtkUndirectedGraph*> : graph_property_type<svtkGraph*>
{
};

// Internal graph properties
template <>
struct graph_property_type<svtkUndirectedGraph* const> : graph_property_type<svtkGraph*>
{
};
#endif

// Internal vertex properties
template <>
struct vertex_property_type<svtkUndirectedGraph*> : vertex_property_type<svtkGraph*>
{
};

// Internal vertex properties
template <>
struct vertex_property_type<svtkUndirectedGraph* const> : vertex_property_type<svtkGraph*>
{
};

// Internal edge properties
template <>
struct edge_property_type<svtkUndirectedGraph*> : edge_property_type<svtkGraph*>
{
};

// Internal edge properties
template <>
struct edge_property_type<svtkUndirectedGraph* const> : edge_property_type<svtkGraph*>
{
};

#if BOOST_VERSION >= 104500
// Internal graph properties
template <>
struct graph_bundle_type<svtkUndirectedGraph*> : graph_bundle_type<svtkGraph*>
{
};

// Internal graph properties
template <>
struct graph_bundle_type<svtkUndirectedGraph* const> : graph_bundle_type<svtkGraph*>
{
};
#endif

// Internal vertex properties
template <>
struct vertex_bundle_type<svtkUndirectedGraph*> : vertex_bundle_type<svtkGraph*>
{
};

// Internal vertex properties
template <>
struct vertex_bundle_type<svtkUndirectedGraph* const> : vertex_bundle_type<svtkGraph*>
{
};

// Internal edge properties
template <>
struct edge_bundle_type<svtkUndirectedGraph*> : edge_bundle_type<svtkGraph*>
{
};

// Internal edge properties
template <>
struct edge_bundle_type<svtkUndirectedGraph* const> : edge_bundle_type<svtkGraph*>
{
};

//===========================================================================
// svtkMutableDirectedGraph

template <>
struct graph_traits<svtkMutableDirectedGraph*> : graph_traits<svtkDirectedGraph*>
{
};

// The graph_traits for a const graph are the same as a non-const graph.
template <>
struct graph_traits<const svtkMutableDirectedGraph*> : graph_traits<svtkMutableDirectedGraph*>
{
};

// The graph_traits for a const graph are the same as a non-const graph.
template <>
struct graph_traits<svtkMutableDirectedGraph* const> : graph_traits<svtkMutableDirectedGraph*>
{
};

#if BOOST_VERSION >= 104500
// Internal graph properties
template <>
struct graph_property_type<svtkMutableDirectedGraph*> : graph_property_type<svtkDirectedGraph*>
{
};

// Internal graph properties
template <>
struct graph_property_type<svtkMutableDirectedGraph* const> : graph_property_type<svtkDirectedGraph*>
{
};
#endif

// Internal vertex properties
template <>
struct vertex_property_type<svtkMutableDirectedGraph*> : vertex_property_type<svtkDirectedGraph*>
{
};

// Internal vertex properties
template <>
struct vertex_property_type<svtkMutableDirectedGraph* const>
  : vertex_property_type<svtkDirectedGraph*>
{
};

// Internal edge properties
template <>
struct edge_property_type<svtkMutableDirectedGraph*> : edge_property_type<svtkDirectedGraph*>
{
};

// Internal edge properties
template <>
struct edge_property_type<svtkMutableDirectedGraph* const> : edge_property_type<svtkDirectedGraph*>
{
};

#if BOOST_VERSION >= 104500
// Internal graph properties
template <>
struct graph_bundle_type<svtkMutableDirectedGraph*> : graph_bundle_type<svtkDirectedGraph*>
{
};

// Internal graph properties
template <>
struct graph_bundle_type<svtkMutableDirectedGraph* const> : graph_bundle_type<svtkDirectedGraph*>
{
};
#endif

// Internal vertex properties
template <>
struct vertex_bundle_type<svtkMutableDirectedGraph*> : vertex_bundle_type<svtkDirectedGraph*>
{
};

// Internal vertex properties
template <>
struct vertex_bundle_type<svtkMutableDirectedGraph* const> : vertex_bundle_type<svtkDirectedGraph*>
{
};

// Internal edge properties
template <>
struct edge_bundle_type<svtkMutableDirectedGraph*> : edge_bundle_type<svtkDirectedGraph*>
{
};

// Internal edge properties
template <>
struct edge_bundle_type<svtkMutableDirectedGraph* const> : edge_bundle_type<svtkDirectedGraph*>
{
};

//===========================================================================
// svtkMutableUndirectedGraph

template <>
struct graph_traits<svtkMutableUndirectedGraph*> : graph_traits<svtkUndirectedGraph*>
{
};

// The graph_traits for a const graph are the same as a non-const graph.
template <>
struct graph_traits<const svtkMutableUndirectedGraph*> : graph_traits<svtkMutableUndirectedGraph*>
{
};

// The graph_traits for a const graph are the same as a non-const graph.
template <>
struct graph_traits<svtkMutableUndirectedGraph* const> : graph_traits<svtkMutableUndirectedGraph*>
{
};

#if BOOST_VERSION >= 104500
// Internal graph properties
template <>
struct graph_property_type<svtkMutableUndirectedGraph*> : graph_property_type<svtkUndirectedGraph*>
{
};

// Internal graph properties
template <>
struct graph_property_type<svtkMutableUndirectedGraph* const>
  : graph_property_type<svtkUndirectedGraph*>
{
};
#endif

// Internal vertex properties
template <>
struct vertex_property_type<svtkMutableUndirectedGraph*> : vertex_property_type<svtkUndirectedGraph*>
{
};

// Internal vertex properties
template <>
struct vertex_property_type<svtkMutableUndirectedGraph* const>
  : vertex_property_type<svtkUndirectedGraph*>
{
};

// Internal edge properties
template <>
struct edge_property_type<svtkMutableUndirectedGraph*> : edge_property_type<svtkUndirectedGraph*>
{
};

// Internal edge properties
template <>
struct edge_property_type<svtkMutableUndirectedGraph* const>
  : edge_property_type<svtkUndirectedGraph*>
{
};

#if BOOST_VERSION >= 104500
// Internal graph properties
template <>
struct graph_bundle_type<svtkMutableUndirectedGraph*> : graph_bundle_type<svtkUndirectedGraph*>
{
};

// Internal graph properties
template <>
struct graph_bundle_type<svtkMutableUndirectedGraph* const> : graph_bundle_type<svtkUndirectedGraph*>
{
};
#endif

// Internal vertex properties
template <>
struct vertex_bundle_type<svtkMutableUndirectedGraph*> : vertex_bundle_type<svtkUndirectedGraph*>
{
};

// Internal vertex properties
template <>
struct vertex_bundle_type<svtkMutableUndirectedGraph* const>
  : vertex_bundle_type<svtkUndirectedGraph*>
{
};

// Internal edge properties
template <>
struct edge_bundle_type<svtkMutableUndirectedGraph*> : edge_bundle_type<svtkUndirectedGraph*>
{
};

// Internal edge properties
template <>
struct edge_bundle_type<svtkMutableUndirectedGraph* const> : edge_bundle_type<svtkUndirectedGraph*>
{
};

//===========================================================================
// API implementation
template <>
class vertex_property<svtkGraph*>
{
public:
  typedef svtkIdType type;
};

template <>
class edge_property<svtkGraph*>
{
public:
  typedef svtkIdType type;
};
} // end namespace boost

inline boost::graph_traits<svtkGraph*>::vertex_descriptor source(
  boost::graph_traits<svtkGraph*>::edge_descriptor e, svtkGraph*)
{
  return e.Source;
}

inline boost::graph_traits<svtkGraph*>::vertex_descriptor target(
  boost::graph_traits<svtkGraph*>::edge_descriptor e, svtkGraph*)
{
  return e.Target;
}

inline std::pair<boost::graph_traits<svtkGraph*>::vertex_iterator,
  boost::graph_traits<svtkGraph*>::vertex_iterator>
vertices(svtkGraph* g)
{
  typedef boost::graph_traits<svtkGraph*>::vertex_iterator Iter;
  svtkIdType start = 0;
  if (svtkDistributedGraphHelper* helper = g->GetDistributedGraphHelper())
  {
    int rank = g->GetInformation()->Get(svtkDataObject::DATA_PIECE_NUMBER());
    start = helper->MakeDistributedId(rank, start);
  }

  return std::make_pair(Iter(start), Iter(start + g->GetNumberOfVertices()));
}

inline std::pair<boost::graph_traits<svtkGraph*>::edge_iterator,
  boost::graph_traits<svtkGraph*>::edge_iterator>
edges(svtkGraph* g)
{
  typedef boost::graph_traits<svtkGraph*>::edge_iterator Iter;
  return std::make_pair(Iter(g), Iter(g, g->GetNumberOfVertices()));
}

inline std::pair<boost::graph_traits<svtkGraph*>::out_edge_iterator,
  boost::graph_traits<svtkGraph*>::out_edge_iterator>
out_edges(boost::graph_traits<svtkGraph*>::vertex_descriptor u, svtkGraph* g)
{
  typedef boost::graph_traits<svtkGraph*>::out_edge_iterator Iter;
  std::pair<Iter, Iter> p = std::make_pair(Iter(g, u), Iter(g, u, true));
  return p;
}

inline std::pair<boost::graph_traits<svtkGraph*>::in_edge_iterator,
  boost::graph_traits<svtkGraph*>::in_edge_iterator>
in_edges(boost::graph_traits<svtkGraph*>::vertex_descriptor u, svtkGraph* g)
{
  typedef boost::graph_traits<svtkGraph*>::in_edge_iterator Iter;
  std::pair<Iter, Iter> p = std::make_pair(Iter(g, u), Iter(g, u, true));
  return p;
}

inline std::pair<boost::graph_traits<svtkGraph*>::adjacency_iterator,
  boost::graph_traits<svtkGraph*>::adjacency_iterator>
adjacent_vertices(boost::graph_traits<svtkGraph*>::vertex_descriptor u, svtkGraph* g)
{
  typedef boost::graph_traits<svtkGraph*>::adjacency_iterator Iter;
  typedef boost::graph_traits<svtkGraph*>::out_edge_iterator OutEdgeIter;
  std::pair<OutEdgeIter, OutEdgeIter> out = out_edges(u, g);
  return std::make_pair(Iter(out.first, &g), Iter(out.second, &g));
}

inline boost::graph_traits<svtkGraph*>::vertices_size_type num_vertices(svtkGraph* g)
{
  return g->GetNumberOfVertices();
}

inline boost::graph_traits<svtkGraph*>::edges_size_type num_edges(svtkGraph* g)
{
  return g->GetNumberOfEdges();
}

inline boost::graph_traits<svtkGraph*>::degree_size_type out_degree(
  boost::graph_traits<svtkGraph*>::vertex_descriptor u, svtkGraph* g)
{
  return g->GetOutDegree(u);
}

inline boost::graph_traits<svtkDirectedGraph*>::degree_size_type in_degree(
  boost::graph_traits<svtkDirectedGraph*>::vertex_descriptor u, svtkDirectedGraph* g)
{
  return g->GetInDegree(u);
}

inline boost::graph_traits<svtkGraph*>::degree_size_type degree(
  boost::graph_traits<svtkGraph*>::vertex_descriptor u, svtkGraph* g)
{
  return g->GetDegree(u);
}

inline boost::graph_traits<svtkMutableDirectedGraph*>::vertex_descriptor add_vertex(
  svtkMutableDirectedGraph* g)
{
  return g->AddVertex();
}

inline std::pair<boost::graph_traits<svtkMutableDirectedGraph*>::edge_descriptor, bool> add_edge(
  boost::graph_traits<svtkMutableDirectedGraph*>::vertex_descriptor u,
  boost::graph_traits<svtkMutableDirectedGraph*>::vertex_descriptor v, svtkMutableDirectedGraph* g)
{
  boost::graph_traits<svtkMutableDirectedGraph*>::edge_descriptor e = g->AddEdge(u, v);
  return std::make_pair(e, true);
}

inline boost::graph_traits<svtkMutableUndirectedGraph*>::vertex_descriptor add_vertex(
  svtkMutableUndirectedGraph* g)
{
  return g->AddVertex();
}

inline std::pair<boost::graph_traits<svtkMutableUndirectedGraph*>::edge_descriptor, bool> add_edge(
  boost::graph_traits<svtkMutableUndirectedGraph*>::vertex_descriptor u,
  boost::graph_traits<svtkMutableUndirectedGraph*>::vertex_descriptor v,
  svtkMutableUndirectedGraph* g)
{
  boost::graph_traits<svtkMutableUndirectedGraph*>::edge_descriptor e = g->AddEdge(u, v);
  return std::make_pair(e, true);
}

namespace boost
{
//===========================================================================
// An edge map for svtkGraph.
// This is a common input needed for algorithms.

struct svtkGraphEdgeMap
{
};

template <>
struct property_traits<svtkGraphEdgeMap>
{
  typedef svtkIdType value_type;
  typedef svtkIdType reference;
  typedef svtkEdgeType key_type;
  typedef readable_property_map_tag category;
};

inline property_traits<svtkGraphEdgeMap>::reference get(
  svtkGraphEdgeMap svtkNotUsed(arr), property_traits<svtkGraphEdgeMap>::key_type key)
{
  return key.Id;
}

//===========================================================================
// Helper for svtkGraph edge property maps
// Automatically converts boost edge ids to svtkGraph edge ids.

template <typename PMap>
class svtkGraphEdgePropertyMapHelper
{
public:
  svtkGraphEdgePropertyMapHelper(PMap m)
    : pmap(m)
  {
  }
  PMap pmap;
  typedef typename property_traits<PMap>::value_type value_type;
  typedef typename property_traits<PMap>::reference reference;
  typedef svtkEdgeType key_type;
  typedef typename property_traits<PMap>::category category;

  reference operator[](const key_type& key) const { return get(pmap, key.Id); }
};

template <typename PMap>
inline typename property_traits<PMap>::reference get(
  svtkGraphEdgePropertyMapHelper<PMap> helper, svtkEdgeType key)
{
  return get(helper.pmap, key.Id);
}

template <typename PMap>
inline void put(svtkGraphEdgePropertyMapHelper<PMap> helper, svtkEdgeType key,
  const typename property_traits<PMap>::value_type& value)
{
  put(helper.pmap, key.Id, value);
}

//===========================================================================
// Helper for svtkGraph vertex property maps
// Automatically converts boost vertex ids to svtkGraph vertex ids.

template <typename PMap>
class svtkGraphVertexPropertyMapHelper
{
public:
  svtkGraphVertexPropertyMapHelper(PMap m)
    : pmap(m)
  {
  }
  PMap pmap;
  typedef typename property_traits<PMap>::value_type value_type;
  typedef typename property_traits<PMap>::reference reference;
  typedef svtkIdType key_type;
  typedef typename property_traits<PMap>::category category;

  reference operator[](const key_type& key) const { return get(pmap, key); }
};

template <typename PMap>
inline typename property_traits<PMap>::reference get(
  svtkGraphVertexPropertyMapHelper<PMap> helper, svtkIdType key)
{
  return get(helper.pmap, key);
}

template <typename PMap>
inline void put(svtkGraphVertexPropertyMapHelper<PMap> helper, svtkIdType key,
  const typename property_traits<PMap>::value_type& value)
{
  put(helper.pmap, key, value);
}

//===========================================================================
// An index map for svtkGraph
// This is a common input needed for algorithms

struct svtkGraphIndexMap
{
};

template <>
struct property_traits<svtkGraphIndexMap>
{
  typedef svtkIdType value_type;
  typedef svtkIdType reference;
  typedef svtkIdType key_type;
  typedef readable_property_map_tag category;
};

inline property_traits<svtkGraphIndexMap>::reference get(
  svtkGraphIndexMap svtkNotUsed(arr), property_traits<svtkGraphIndexMap>::key_type key)
{
  return key;
}

//===========================================================================
// Helper for svtkGraph property maps
// Automatically multiplies the property value by some value (default 1)
template <typename PMap>
class svtkGraphPropertyMapMultiplier
{
public:
  svtkGraphPropertyMapMultiplier(PMap m, float multi = 1)
    : pmap(m)
    , multiplier(multi)
  {
  }
  PMap pmap;
  float multiplier;
  typedef typename property_traits<PMap>::value_type value_type;
  typedef typename property_traits<PMap>::reference reference;
  typedef typename property_traits<PMap>::key_type key_type;
  typedef typename property_traits<PMap>::category category;
};

template <typename PMap>
inline typename property_traits<PMap>::reference get(
  svtkGraphPropertyMapMultiplier<PMap> multi, const typename property_traits<PMap>::key_type& key)
{
  return multi.multiplier * get(multi.pmap, key);
}

template <typename PMap>
inline void put(svtkGraphPropertyMapMultiplier<PMap> multi,
  const typename property_traits<PMap>::key_type& key,
  const typename property_traits<PMap>::value_type& value)
{
  put(multi.pmap, key, value);
}

// Allow algorithms to automatically extract svtkGraphIndexMap from a
// SVTK graph
template <>
struct property_map<svtkGraph*, vertex_index_t>
{
  typedef svtkGraphIndexMap type;
  typedef svtkGraphIndexMap const_type;
};

template <>
struct property_map<svtkDirectedGraph*, vertex_index_t> : property_map<svtkGraph*, vertex_index_t>
{
};

template <>
struct property_map<svtkUndirectedGraph*, vertex_index_t> : property_map<svtkGraph*, vertex_index_t>
{
};

inline svtkGraphIndexMap get(vertex_index_t, svtkGraph*)
{
  return svtkGraphIndexMap();
}

template <>
struct property_map<svtkGraph*, edge_index_t>
{
  typedef svtkGraphIndexMap type;
  typedef svtkGraphIndexMap const_type;
};

template <>
struct property_map<svtkDirectedGraph*, edge_index_t> : property_map<svtkGraph*, edge_index_t>
{
};

template <>
struct property_map<svtkUndirectedGraph*, edge_index_t> : property_map<svtkGraph*, edge_index_t>
{
};

inline svtkGraphIndexMap get(edge_index_t, svtkGraph*)
{
  return svtkGraphIndexMap();
}

// property_map specializations for const-qualified graphs
template <>
struct property_map<svtkDirectedGraph* const, vertex_index_t>
  : property_map<svtkDirectedGraph*, vertex_index_t>
{
};

template <>
struct property_map<svtkUndirectedGraph* const, vertex_index_t>
  : property_map<svtkUndirectedGraph*, vertex_index_t>
{
};

template <>
struct property_map<svtkDirectedGraph* const, edge_index_t>
  : property_map<svtkDirectedGraph*, edge_index_t>
{
};

template <>
struct property_map<svtkUndirectedGraph* const, edge_index_t>
  : property_map<svtkUndirectedGraph*, edge_index_t>
{
};
} // namespace boost

#if BOOST_VERSION > 104000
#include <boost/property_map/vector_property_map.hpp>
#else
#include <boost/vector_property_map.hpp>
#endif

#endif // svtkBoostGraphAdapter_h
// SVTK-HeaderTest-Exclude: svtkBoostGraphAdapter.h
