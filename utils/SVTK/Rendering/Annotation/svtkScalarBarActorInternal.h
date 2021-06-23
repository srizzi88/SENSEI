#ifndef svtkScalarBarActorInternal_h
#define svtkScalarBarActorInternal_h

#ifndef __SVTK_WRAP__

#include "svtkColor.h"                     // for AnnotationColors, LabelColorMap, and tuples
#include "svtkRenderingAnnotationModule.h" // For export macro
#include "svtkSmartPointer.h"              // for "smart vectors"
#include "svtkStdString.h"                 // for LabelMap

#include <map>
#include <vector>

class svtkActor2D;
class svtkCellArray;
class svtkTextActor;
class svtkPoints;
class svtkPolyData;
class svtkPolyDataMapper2D;
class svtkUnsignedCharArray;
class svtkViewport;

/// A vector of smart pointers.
template <class T>
class SVTKRENDERINGANNOTATION_EXPORT svtkSmartVector : public std::vector<svtkSmartPointer<T> >
{
public:
  /**\brief Convert to an array of "dumb" pointers for functions
   *  that need a contiguous array pointer as input.
   */
  T** PointerArray()
  {
    // NB: This is relatively evil. But much cheaper than copying the array.
    // It assumes the compiler won't pad the class.
    return reinterpret_cast<T**>(&((*this)[0]));
  }
};

/// A structure to represent pixel coordinates for text or swatch bounds.
struct svtkScalarBarBox
{
  /// The position of the box in viewport (pixel) coordinates.
  svtkTuple<int, 2> Posn;

  /**\brief Size of the box, stored as (thickness, length) not (width, height).
   *
   * Thickness is a measure of the box size perpendicular to the long axis of the scalar bar.
   * When the scalar bar orientation is horizontal, thickness measures height.
   * Length is a measure of the box size parallel to the long axis of the scalar bar.
   * When the scalar bar orientation is horizontal, length measures width.
   */
  svtkTuple<int, 2> Size;
};

/// Internal state for the scalar bar actor shared with subclasses.
class SVTKRENDERINGANNOTATION_EXPORT svtkScalarBarActorInternal
{
public:
  svtkScalarBarActorInternal()
  {
    this->Viewport = nullptr;
    this->SwatchColors = nullptr;
    this->SwatchPts = nullptr;
    this->Polys = nullptr;
    this->AnnotationBoxes = nullptr;
    this->AnnotationBoxesMapper = nullptr;
    this->AnnotationBoxesActor = nullptr;
    this->AnnotationLeaders = nullptr;
    this->AnnotationLeadersMapper = nullptr;
    this->AnnotationLeadersActor = nullptr;
    this->NanSwatch = nullptr;
    this->NanSwatchMapper = nullptr;
    this->NanSwatchActor = nullptr;

    this->BelowRangeSwatch = nullptr;
    this->BelowRangeSwatchMapper = nullptr;
    this->BelowRangeSwatchActor = nullptr;

    this->AboveRangeSwatch = nullptr;
    this->AboveRangeSwatchMapper = nullptr;
    this->AboveRangeSwatchActor = nullptr;
  }

  // Define types for smart vectors containing various base classes.
  typedef svtkSmartVector<svtkTextActor> ActorVector;

  // Other vector container types.
  typedef std::vector<double> DoubleVector;
  typedef std::vector<svtkColor3ub> ColorVector;

  /**\brief Cache of dimensions fixed during geometry assembly.
   *
   * Only valid within methods invoked by svtkScalarBarActor::RebuildLayout().
   */
  //@{
  svtkViewport* Viewport;

  /// The thickness and length of the (square) NaN swatch.
  double NanSwatchSize;

  /// The thickness and length of the (square) Below Range swatch.
  double BelowRangeSwatchSize;

  /// The thickness and length of the (square) Above Range swatch.
  double AboveRangeSwatchSize;

  /// Space in pixels between swatches when in indexed lookup mode.
  double SwatchPad;

  /// Number of annotated values (at least
  /// lut->GetNumberOfAnnotatedValues(), but maybe more)
  int NumNotes;

  /// Number of color swatches to draw for either the continuous or
  /// categorical scalar bar, not including a NaN swatch.
  int NumColors;

  /// Either NumColors or NumColors + 1, depending on whether the NaN
  /// swatch is to be drawn.
  int NumSwatches;

  /// Permutation of (0, 1) that transforms thickness,length into
  /// width,height.
  int TL[2]; // VERTICAL => TL={0,1}, HORIZONTAL => TL={1,0}, Size[TL[0]] == width, Size[TL[1]] ==
             // height

  /// Point coordinates for the scalar bar actor
  svtkPoints* SwatchPts;

  /// Cells representing color swatches (for the scalar bar actor)
  svtkCellArray* Polys;

  /// Colors of swatches in \a Polys
  svtkUnsignedCharArray* SwatchColors;

  /// The bounding box of the entire scalar bar frame.
  svtkScalarBarBox Frame;

  /// The bounding box of the scalar bar (excluding NaN swatch)
  svtkScalarBarBox ScalarBarBox;

  /// The bounding box of the NaN swatch
  svtkScalarBarBox NanBox;

  /// The bounding box of the Below Range
  svtkScalarBarBox BelowRangeSwatchBox;

  /// The bounding box of the Above Range
  svtkScalarBarBox AboveRangeSwatchBox;

  /// The bounding box of tick mark anchor points (tick labels are not
  /// fully contained)
  svtkScalarBarBox TickBox;

  /// The bounding box of the scalar bar title text.
  svtkScalarBarBox TitleBox;

  /// Map from viewport coordinates to label text of each annotation.
  std::map<double, svtkStdString> Labels;

  /// Map from viewport coordinates to the leader line color of each
  /// annotation.
  std::map<double, svtkColor3ub> LabelColors;
  //@}

  /// Cache of classes holding geometry assembled and ready for rendering.
  //@{
  ActorVector TextActors;
  svtkPolyData* AnnotationBoxes;
  svtkPolyDataMapper2D* AnnotationBoxesMapper;
  svtkActor2D* AnnotationBoxesActor;
  svtkPolyData* AnnotationLeaders;
  svtkPolyDataMapper2D* AnnotationLeadersMapper;
  svtkActor2D* AnnotationLeadersActor;
  ActorVector AnnotationLabels;
  DoubleVector AnnotationAnchors;
  ColorVector AnnotationColors;
  svtkPolyData* NanSwatch;
  svtkPolyDataMapper2D* NanSwatchMapper;
  svtkActor2D* NanSwatchActor;

  svtkPolyData* BelowRangeSwatch;
  svtkPolyDataMapper2D* BelowRangeSwatchMapper;
  svtkActor2D* BelowRangeSwatchActor;

  svtkPolyData* AboveRangeSwatch;
  svtkPolyDataMapper2D* AboveRangeSwatchMapper;
  svtkActor2D* AboveRangeSwatchActor;
  //@}
};

#endif

#endif // svtkScalarBarActorInternal_h
// SVTK-HeaderTest-Exclude: svtkScalarBarActorInternal.h
