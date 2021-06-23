/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSimple3DCirclesStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSimple3DCirclesStrategy
 * @brief   places vertices on circles in 3D
 *
 *
 * Places vertices on circles depending on the graph vertices hierarchy level.
 * The source graph could be svtkDirectedAcyclicGraph or svtkDirectedGraph if MarkedStartPoints array
 * was added. The algorithm collects the standalone points, too and take them to a separated circle.
 * If method is FixedRadiusMethod, the radius of the circles will be equal. If method is
 * FixedDistanceMethod, the distance between the points on circles will be equal.
 *
 * In first step initial points are searched. A point is initial, if its in degree equal zero and
 * out degree is greater than zero (or marked by MarkedStartVertices and out degree is greater than
 * zero). Independent vertices (in and out degree equal zero) are collected separatelly. In second
 * step the hierarchical level is generated for every vertex. In third step the hierarchical order
 * is generated. If a vertex has no hierarchical level and it is not independent, the graph has loop
 * so the algorithm exit with error message. Finally the vertices positions are calculated by the
 * hierarchical order and by the vertices hierarchy levels.
 *
 * @par Thanks:
 * Ferenc Nasztanovics, naszta@naszta.hu, Budapest University of Technology and Economics,
 * Department of Structural Mechanics
 *
 * @par References:
 * in 3D rotation was used: http://en.citizendium.org/wiki/Rotation_matrix
 */

#ifndef svtkSimple3DCirclesStrategy_h
#define svtkSimple3DCirclesStrategy_h

#include "svtkGraphLayoutStrategy.h"
#include "svtkInfovisLayoutModule.h" // For export macro
#include "svtkVariant.h"             // For variant API

class svtkAbstractArray;
class svtkDirectedGraph;
class svtkIdTypeArray;
class svtkIntArray;
class svtkSimple3DCirclesStrategyInternal;

class SVTKINFOVISLAYOUT_EXPORT svtkSimple3DCirclesStrategy : public svtkGraphLayoutStrategy
{
public:
  static svtkSimple3DCirclesStrategy* New();
  svtkTypeMacro(svtkSimple3DCirclesStrategy, svtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum
  {
    FixedRadiusMethod = 0,
    FixedDistanceMethod = 1
  };

  //@{
  /**
   * Set or get circle generating method (FixedRadiusMethod/FixedDistanceMethod). Default is
   * FixedRadiusMethod.
   */
  svtkSetMacro(Method, int);
  svtkGetMacro(Method, int);
  //@}
  //@{
  /**
   * If Method is FixedRadiusMethod: Set or get the radius of the circles.
   * If Method is FixedDistanceMethod: Set or get the distance of the points in the circle.
   */
  svtkSetMacro(Radius, double);
  svtkGetMacro(Radius, double);
  //@}
  //@{
  /**
   * Set or get the vertical (local z) distance between the circles. If AutoHeight is on, this is
   * the minimal height between the circle layers
   */
  svtkSetMacro(Height, double);
  svtkGetMacro(Height, double);
  //@}
  //@{
  /**
   * Set or get the origin of the geometry. This is the center of the first circle. SetOrigin(x,y,z)
   */
  svtkSetVector3Macro(Origin, double);
  svtkGetVector3Macro(Origin, double);
  //@}
  //@{
  /**
   * Set or get the normal vector of the circles plain. The height is growing in this direction. The
   * direction must not be zero vector. The default vector is (0.0,0.0,1.0)
   */
  virtual void SetDirection(double dx, double dy, double dz);
  virtual void SetDirection(double d[3]);
  svtkGetVector3Macro(Direction, double);
  //@}
  //@{
  /**
   * Set or get initial vertices. If MarkedStartVertices is added, loop is accepted in the graph.
   * (If all of the loop start vertices are marked in MarkedStartVertices array.)
   * MarkedStartVertices size must be equal with the number of the vertices in the graph. Start
   * vertices must be marked by MarkedValue. (E.g.: if MarkedValue=3 and MarkedStartPoints is { 0,
   * 3, 5, 3 }, the start points ids will be {1,3}.) )
   */
  virtual void SetMarkedStartVertices(svtkAbstractArray* _arg);
  svtkGetObjectMacro(MarkedStartVertices, svtkAbstractArray);
  //@}
  //@{
  /**
   * Set or get MarkedValue. See: MarkedStartVertices.
   */
  virtual void SetMarkedValue(svtkVariant _arg);
  virtual svtkVariant GetMarkedValue(void);
  //@}
  //@{
  /**
   * Set or get ForceToUseUniversalStartPointsFinder. If ForceToUseUniversalStartPointsFinder is
   * true, MarkedStartVertices won't be used. In this case the input graph must be
   * svtkDirectedAcyclicGraph (Default: false).
   */
  svtkSetMacro(ForceToUseUniversalStartPointsFinder, svtkTypeBool);
  svtkGetMacro(ForceToUseUniversalStartPointsFinder, svtkTypeBool);
  svtkBooleanMacro(ForceToUseUniversalStartPointsFinder, svtkTypeBool);
  //@}
  //@{
  /**
   * Set or get auto height (Default: false). If AutoHeight is true, (r(i+1) - r(i-1))/Height will
   * be smaller than tan(MinimumRadian). If you want equal distances and parallel circles, you
   * should turn off AutoHeight.
   */
  svtkSetMacro(AutoHeight, svtkTypeBool);
  svtkGetMacro(AutoHeight, svtkTypeBool);
  svtkBooleanMacro(AutoHeight, svtkTypeBool);
  //@}
  //@{
  /**
   * Set or get minimum radian (used by auto height).
   */
  svtkSetMacro(MinimumRadian, double);
  svtkGetMacro(MinimumRadian, double);
  //@}
  //@{
  /**
   * Set or get minimum degree (used by auto height). There is no separated minimum degree, so
   * minimum radian will be changed.
   */
  virtual void SetMinimumDegree(double degree);
  virtual double GetMinimumDegree(void);
  //@}
  //@{
  /**
   * Set or get hierarchical layers id by vertices (An usual vertex's layer id is greater or equal
   * to zero. If a vertex is standalone, its layer id is -2.) If no HierarchicalLayers array is
   * defined, svtkSimple3DCirclesStrategy will generate it automatically (default).
   */
  virtual void SetHierarchicalLayers(svtkIntArray* _arg);
  svtkGetObjectMacro(HierarchicalLayers, svtkIntArray);
  //@}
  //@{
  /**
   * Set or get hierarchical ordering of vertices (The array starts from the first vertex's id. All
   * id must be greater or equal to zero!) If no HierarchicalOrder is defined,
   * svtkSimple3DCirclesStrategy will generate it automatically (default).
   */
  virtual void SetHierarchicalOrder(svtkIdTypeArray* _arg);
  svtkGetObjectMacro(HierarchicalOrder, svtkIdTypeArray);
  //@}
  /**
   * Standard layout method
   */
  void Layout(void) override;
  /**
   * Set graph (warning: HierarchicalOrder and HierarchicalLayers will set to zero. These reference
   * counts will be decreased!)
   */
  void SetGraph(svtkGraph* graph) override;

protected:
  svtkSimple3DCirclesStrategy();
  ~svtkSimple3DCirclesStrategy() override;

  inline void Transform(double Local[], double Global[]);

  double Radius;
  double Height;
  double Origin[3];
  double Direction[3];
  int Method;
  svtkAbstractArray* MarkedStartVertices;
  svtkVariant MarkedValue;
  svtkTypeBool ForceToUseUniversalStartPointsFinder;
  svtkTypeBool AutoHeight;
  double MinimumRadian;

  svtkIntArray* HierarchicalLayers;
  svtkIdTypeArray* HierarchicalOrder;

private:
  /**
   * Search and fill in target all zero input degree vertices where the output degree is more than
   * zero. The finded vertices hierarchical layer ID will be zero.
   */
  virtual int UniversalStartPoints(svtkDirectedGraph* input,
    svtkSimple3DCirclesStrategyInternal* target, svtkSimple3DCirclesStrategyInternal* StandAlones,
    svtkIntArray* layers);
  /**
   * Build hierarchical layers in the graph. A vertices hierarchical layer number is equal the
   * maximum of its inputs hierarchical layer numbers plus one.
   */
  virtual int BuildLayers(
    svtkDirectedGraph* input, svtkSimple3DCirclesStrategyInternal* source, svtkIntArray* layers);
  /**
   * Build hierarchical ordering of the graph points.
   */
  virtual void BuildPointOrder(svtkDirectedGraph* input, svtkSimple3DCirclesStrategyInternal* source,
    svtkSimple3DCirclesStrategyInternal* StandAlones, svtkIntArray* layers, svtkIdTypeArray* order);

  double T[3][3];

  svtkSimple3DCirclesStrategy(const svtkSimple3DCirclesStrategy&) = delete;
  void operator=(const svtkSimple3DCirclesStrategy&) = delete;
};

#endif
