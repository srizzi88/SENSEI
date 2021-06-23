#ifndef svtkLabelHierarchyPrivate_h
#define svtkLabelHierarchyPrivate_h

#include <set>

#include "octree/octree"

//----------------------------------------------------------------------------
// svtkLabelHierarchy::Implementation

class svtkLabelHierarchy::Implementation
{
public:
  Implementation()
  {
    this->Hierarchy2 = nullptr;
    this->Hierarchy3 = nullptr;
    this->ActualDepth = 5;
    this->Z2 = 0.;
  }

  ~Implementation()
  {
    delete this->Hierarchy2;
    delete this->Hierarchy3;
  }

  bool ComparePriorities(svtkIdType a, svtkIdType b)
  {
    svtkDataArray* priorities = this->Husk->GetPriorities();
    return priorities ? priorities->GetTuple1(a) > priorities->GetTuple1(b) : a < b;
  }

  struct PriorityComparator
  {
    svtkLabelHierarchy* Hierarchy;

    PriorityComparator()
    {
      // See comment near declaration of Current for more info:
      this->Hierarchy = svtkLabelHierarchy::Implementation::Current;
    }

    PriorityComparator(svtkLabelHierarchy* h) { this->Hierarchy = h; }

    PriorityComparator(const PriorityComparator& src) { this->Hierarchy = src.Hierarchy; }

    PriorityComparator& operator=(const PriorityComparator& rhs)
    {
      if (this != &rhs)
      {
        this->Hierarchy = rhs.Hierarchy;
      }
      return *this;
    }

    ~PriorityComparator() {}

    bool operator()(const svtkIdType& a, const svtkIdType& b) const
    {
      if (nullptr == this->Hierarchy)
      {
        svtkGenericWarningMacro("error: NULL this->Hierarchy in PriorityComparator");
        return a < b;
      }

      if (nullptr == this->Hierarchy->GetImplementation())
      {
        svtkGenericWarningMacro(
          "error: NULL this->Hierarchy->GetImplementation() in PriorityComparator");
        return a < b;
      }

      return this->Hierarchy->GetImplementation()->ComparePriorities(a, b);
    }
  };

  class LabelSet : public std::multiset<svtkIdType, PriorityComparator>
  {
  public:
    LabelSet(svtkLabelHierarchy* hierarchy)
      : std::multiset<svtkIdType, PriorityComparator>(PriorityComparator(hierarchy))
    {
      this->TotalAnchors = 0;
      this->Size = 1.;
      for (int i = 0; i < 3; ++i)
      {
        this->Center[i] = 0.;
      }
    }

    LabelSet(const LabelSet& src)
      : std::multiset<svtkIdType, PriorityComparator>(src)
    {
      this->TotalAnchors = src.TotalAnchors;
      this->Size = src.Size;
      for (int i = 0; i < 3; ++i)
      {
        this->Center[i] = src.Center[i];
      }
    }

    LabelSet()
      : std::multiset<svtkIdType, PriorityComparator>()
    {
      this->TotalAnchors = 0;
      this->Size = 1.;
      for (int i = 0; i < 3; ++i)
      {
        this->Center[i] = 0.;
      }
    }

    LabelSet& operator=(const LabelSet& rhs)
    {
      if (this != &rhs)
      {
        std::multiset<svtkIdType, PriorityComparator>::operator=(rhs);
        this->TotalAnchors = rhs.TotalAnchors;
        this->Size = rhs.Size;
        for (int i = 0; i < 3; ++i)
        {
          this->Center[i] = rhs.Center[i];
        }
      }
      return *this;
    }
    const double* GetCenter() const { return this->Center; }
    double GetSize() const { return this->Size; }
    void SetGeometry(const double center[3], double length);
    void SetChildGeometry(octree<LabelSet, 2>::octree_node_pointer self);
    void SetChildGeometry(octree<LabelSet, 3>::octree_node_pointer self);
    void AddChildren(octree<LabelSet, 2>::octree_node_pointer self, LabelSet& emptyNode);
    void AddChildren(octree<LabelSet, 3>::octree_node_pointer self, LabelSet& emptyNode);
    void Insert(svtkIdType anchor)
    {
      this->insert(anchor);
      ++this->TotalAnchors;
    }
    void Increment() { ++this->TotalAnchors; }
    svtkIdType GetLocalAnchorCount() const { return static_cast<svtkIdType>(this->size()); }
    svtkIdType GetTotalAnchorCount() const { return this->TotalAnchors; }

    svtkIdType TotalAnchors; // Count of all anchors stored in this node and its children.
    double Center[3];       // Geometric coordinates of this node's center.
    double Size;            // Length of each edge of this node.
  };

  typedef octree<LabelSet, 2> HierarchyType2;
  typedef octree<LabelSet, 2>::cursor HierarchyCursor2;
  typedef octree<LabelSet, 2>::iterator HierarchyIterator2;

  typedef octree<LabelSet> HierarchyType3;
  typedef octree<LabelSet>::cursor HierarchyCursor3;
  typedef octree<LabelSet>::iterator HierarchyIterator3;

  // typedef std::map<Coord,std::pair<int,std::set<svtkIdType> > >::iterator MapCoordIter;

  // Description:
  // Computes the depth of the generated hierarchy.
  // void ComputeActualDepth();

  // Description:
  // Routines called by ComputeHierarchy()
  void BinAnchorsToLevel(int level);
  void PromoteAnchors();
  void DemoteAnchors(int level);
  void RecursiveNodeDivide(HierarchyCursor2& cursor);
  void RecursiveNodeDivide(HierarchyCursor3& cursor);

  // Description:
  // Routines called by ComputeHierarchy()
  void PrepareSortedAnchors(LabelSet& anchors);
  void FillHierarchyRoot(LabelSet& anchors);
  void DropAnchor2(svtkIdType anchor);
  void DropAnchor3(svtkIdType anchor);
  void SmudgeAnchor2(HierarchyCursor2& cursor, svtkIdType anchor, double* x);
  void SmudgeAnchor3(HierarchyCursor3& cursor, svtkIdType anchor, double* x);

  double Z2; // common z-coordinate of all label anchors when quadtree (Hierarchy2) is used.
  HierarchyType2* Hierarchy2; // 2-D quadtree of label anchors (all input points have same z coord)
  HierarchyType3*
    Hierarchy3; // 3-D octree of label anchors (input point bounds have non-zero z range)
  svtkTimeStamp HierarchyTime;
  HierarchyType3::size_type ActualDepth;
  svtkLabelHierarchy* Husk;

  static svtkLabelHierarchy* Current;
};

inline void svtkLabelHierarchy::Implementation::LabelSet::SetGeometry(
  const double center[3], double length)
{
  for (int i = 0; i < 3; ++i)
  {
    this->Center[i] = center[i];
  }
  this->Size = length;
}

inline void svtkLabelHierarchy::Implementation::LabelSet::SetChildGeometry(
  octree<LabelSet, 2>::octree_node_pointer self)
{
  double sz2 = this->Size / 2.;
  double x[3];
  for (int i = 0; i < self->num_children(); ++i)
  {
    for (int j = 0; j < 2; ++j)
    {
      x[j] = this->Center[j] + ((i & (1 << j)) ? 0.5 : -0.5) * sz2;
    }
    x[2] = this->Center[2];
    (*self)[i].value().SetGeometry(x, sz2);
  }
}

inline void svtkLabelHierarchy::Implementation::LabelSet::SetChildGeometry(
  octree<LabelSet, 3>::octree_node_pointer self)
{
  double sz2 = this->Size / 2.;
  double x[3];
  for (int i = 0; i < self->num_children(); ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      x[j] = this->Center[j] + ((i & (1 << j)) ? 0.5 : -0.5) * sz2;
    }
    (*self)[i].value().SetGeometry(x, sz2);
  }
}

inline void svtkLabelHierarchy::Implementation::LabelSet::AddChildren(
  octree<LabelSet, 2>::octree_node_pointer self, LabelSet& emptyNode)
{
  self->add_children(emptyNode);
  this->SetChildGeometry(self);
}

inline void svtkLabelHierarchy::Implementation::LabelSet::AddChildren(
  octree<LabelSet, 3>::octree_node_pointer self, LabelSet& emptyNode)
{
  self->add_children(emptyNode);
  this->SetChildGeometry(self);
}

#endif // svtkLabelHierarchyPrivate_h
// SVTK-HeaderTest-Exclude: svtkLabelHierarchyPrivate.h
