//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ColorTable_h
#define svtk_m_cont_ColorTable_h

#include <svtkm/Range.h>
#include <svtkm/Types.h>

#include <svtkm/cont/svtkm_cont_export.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ColorTableSamples.h>

#include <set>

namespace svtkm
{

namespace exec
{
//forward declare exec objects
class ColorTableBase;
}

namespace cont
{

template <typename T>
class VirtualObjectHandle;


namespace detail
{
struct ColorTableInternals;
}

enum struct ColorSpace
{
  RGB,
  HSV,
  HSV_WRAP,
  LAB,
  DIVERGING
};

/// \brief Color Table for coloring arbitrary fields
///
///
/// The svtkm::cont::ColorTable allows for color mapping in RGB or HSV space and
/// uses a piecewise hermite functions to allow opacity interpolation that can be
/// piecewise constant, piecewise linear, or somewhere in-between
/// (a modified piecewise hermite function that squishes the function
/// according to a sharpness parameter).
///
/// For colors interpolation is handled using a piecewise linear function.
///
/// For opacity we define a piecewise function mapping. This mapping allows the addition
/// of control points, and allows the user to control the function between
/// the control points. A piecewise hermite curve is used between control
/// points, based on the sharpness and midpoint parameters. A sharpness of
/// 0 yields a piecewise linear function and a sharpness of 1 yields a
/// piecewise constant function. The midpoint is the normalized distance
/// between control points at which the curve reaches the median Y value.
/// The midpoint and sharpness values specified when adding a node are used
/// to control the transition to the next node with the last node's values being
/// ignored.
///
/// When adding opacity nodes without an explicit midpoint and sharpness we
/// will default to to Midpoint = 0.5 (halfway between the control points) and
/// Sharpness = 0.0 (linear).
///
/// ColorTable also contains which ColorSpace should be used for interpolation
/// Currently the valid ColorSpaces are:
/// - RGB
/// - HSV
/// - HSV_WRAP
/// - LAB
/// - Diverging
///
/// In HSV_WRAP mode, it will take the shortest path
/// in Hue (going back through 0 if that is the shortest way around the hue
/// circle) whereas HSV will not go through 0 (in order the
/// match the current functionality of svtkLookupTable). In Lab mode,
/// it will take the shortest path in the Lab color space with respect to the
/// CIE Delta E 2000 color distance measure. Diverging is a special
/// mode where colors will pass through white when interpolating between two
/// saturated colors.
///
/// To map a field from a svtkm::cont::DataSet through the color and opacity transfer
/// functions and into a RGB or RGBA array you should use svtkm::filter::FieldToColor.
///
class SVTKM_CONT_EXPORT ColorTable
{
  std::shared_ptr<detail::ColorTableInternals> Impl;

public:
  enum struct Preset
  {
    DEFAULT,
    COOL_TO_WARM,
    COOL_TO_WARM_EXTENDED,
    VIRIDIS,
    INFERNO,
    PLASMA,
    BLACK_BODY_RADIATION,
    X_RAY,
    GREEN,
    BLACK_BLUE_WHITE,
    BLUE_TO_ORANGE,
    GRAY_TO_RED,
    COLD_AND_HOT,
    BLUE_GREEN_ORANGE,
    YELLOW_GRAY_BLUE,
    RAINBOW_UNIFORM,
    JET,
    RAINBOW_DESATURATED
  };

  /// \brief Construct a color table from a preset
  ///
  /// Constructs a color table from a given preset, which might include a NaN color.
  /// The alpha table will have 2 entries of alpha = 1.0 with linear interpolation
  ///
  /// Note: these are a select set of the presets you can get by providing a string identifier.
  ///
  ColorTable(svtkm::cont::ColorTable::Preset preset = svtkm::cont::ColorTable::Preset::DEFAULT);

  /// \brief Construct a color table from a preset color table
  ///
  /// Constructs a color table from a given preset, which might include a NaN color.
  /// The alpha table will have 2 entries of alpha = 1.0 with linear interpolation
  ///
  /// Note: Names are case insensitive
  /// Currently supports the following color tables:
  ///
  /// "Default"
  /// "Cool to Warm"
  /// "Cool to Warm Extended"
  /// "Viridis"
  /// "Inferno"
  /// "Plasma"
  /// "Black-Body Radiation"
  /// "X Ray"
  /// "Green"
  /// "Black - Blue - White"
  /// "Blue to Orange"
  /// "Gray to Red"
  /// "Cold and Hot"
  /// "Blue - Green - Orange"
  /// "Yellow - Gray - Blue"
  /// "Rainbow Uniform"
  /// "Jet"
  /// "Rainbow Desaturated"
  ///
  explicit ColorTable(const std::string& name);

  /// Construct a color table with a zero positions, and an invalid range
  ///
  /// Note: The color table will have 0 entries
  /// Note: The alpha table will have 0 entries
  explicit ColorTable(ColorSpace space);

  /// Construct a color table with a 2 positions
  ///
  /// Note: The color table will have 2 entries of rgb = {1.0,1.0,1.0}
  /// Note: The alpha table will have 2 entries of alpha = 1.0 with linear
  ///       interpolation
  ColorTable(const svtkm::Range& range, ColorSpace space = ColorSpace::LAB);

  /// Construct a color table with 2 positions
  //
  /// Note: The alpha table will have 2 entries of alpha = 1.0 with linear
  ///       interpolation
  ColorTable(const svtkm::Range& range,
             const svtkm::Vec<float, 3>& rgb1,
             const svtkm::Vec<float, 3>& rgb2,
             ColorSpace space = ColorSpace::LAB);

  /// Construct color and alpha and table with 2 positions
  ///
  /// Note: The alpha table will use linear interpolation
  ColorTable(const svtkm::Range& range,
             const svtkm::Vec<float, 4>& rgba1,
             const svtkm::Vec<float, 4>& rgba2,
             ColorSpace space = ColorSpace::LAB);

  /// Construct a color table with a list of colors and alphas. For this version you must also
  /// specify a name.
  ///
  /// This constructor is mostly used for presets.
  ColorTable(const std::string& name,
             svtkm::cont::ColorSpace colorSpace,
             const svtkm::Vec<double, 3>& nanColor,
             const std::vector<double>& rgbPoints,
             const std::vector<double>& alphaPoints = { 0.0, 1.0, 0.5, 0.0, 1.0, 1.0, 0.5, 0.0 });


  ~ColorTable();

  ColorTable& operator=(const ColorTable&) = default;
  ColorTable(const ColorTable&) = default;

  const std::string& GetName() const;
  void SetName(const std::string& name);

  bool LoadPreset(svtkm::cont::ColorTable::Preset preset);

  /// Returns the name of all preset color tables
  ///
  /// This list will include all presets defined in svtkm::cont::ColorTable::Preset and could
  /// include extras as well.
  ///
  static std::set<std::string> GetPresets();

  /// Load a preset color table
  ///
  /// Removes all existing all values in both color and alpha tables,
  /// and will reset the NaN Color if the color table has that information.
  /// Will not modify clamping, below, and above range state.
  ///
  /// Note: Names are case insensitive
  ///
  /// Currently supports the following color tables:
  /// "Default"
  /// "Cool to Warm"
  /// "Cool to Warm Extended"
  /// "Viridis"
  /// "Inferno"
  /// "Plasma"
  /// "Black-Body Radiation"
  /// "X Ray"
  /// "Green"
  /// "Black - Blue - White"
  /// "Blue to Orange"
  /// "Gray to Red"
  /// "Cold and Hot"
  /// "Blue - Green - Orange"
  /// "Yellow - Gray - Blue"
  /// "Rainbow Uniform"
  /// "Jet"
  /// "Rainbow Desaturated"
  ///
  bool LoadPreset(const std::string& name);

  /// Make a deep copy of the current color table
  ///
  /// The ColorTable is implemented so that all stack based copies are 'shallow'
  /// copies. This means that they all alter the same internal instance. But
  /// sometimes you need to make an actual fully independent copy.
  ColorTable MakeDeepCopy();

  ///
  ColorSpace GetColorSpace() const;
  void SetColorSpace(ColorSpace space);

  /// If clamping is disabled values that lay out side
  /// the color table range are colored based on Below
  /// and Above settings.
  ///
  /// By default clamping is enabled
  void SetClampingOn() { this->SetClamping(true); }
  void SetClampingOff() { this->SetClamping(false); }
  void SetClamping(bool state);
  bool GetClamping() const;

  /// Color to use when clamping is disabled for any value
  /// that is below the given range
  ///
  /// Default value is {0,0,0}
  void SetBelowRangeColor(const svtkm::Vec<float, 3>& c);
  const svtkm::Vec<float, 3>& GetBelowRangeColor() const;

  /// Color to use when clamping is disabled for any value
  /// that is above the given range
  ///
  /// Default value is {0,0,0}
  void SetAboveRangeColor(const svtkm::Vec<float, 3>& c);
  const svtkm::Vec<float, 3>& GetAboveRangeColor() const;

  ///
  void SetNaNColor(const svtkm::Vec<float, 3>& c);
  const svtkm::Vec<float, 3>& GetNaNColor() const;

  /// Remove all existing values in both color and alpha tables.
  /// Does not remove the clamping, below, and above range state or colors.
  void Clear();

  /// Remove only color table values
  void ClearColors();

  /// Remove only alpha table values
  void ClearAlpha();

  /// Reverse the rgb values inside the color table
  void ReverseColors();

  /// Reverse the alpha, mid, and sharp values inside the opacity table.
  ///
  /// Note: To keep the shape correct the mid and sharp values of the last
  /// node are not included in the reversal
  void ReverseAlpha();

  /// Returns min and max position of all function points
  const svtkm::Range& GetRange() const;

  /// Rescale the color and opacity transfer functions to match the
  /// input range.
  void RescaleToRange(const svtkm::Range& range);

  // Functions for Colors

  /// Adds a point to the color function. If the point already exists, it
  /// will be updated to the new value.
  ///
  /// Note: rgb values need to be between 0 and 1.0 (inclusive).
  /// Return the index of the point (0 based), or -1 osn error.
  svtkm::Int32 AddPoint(double x, const svtkm::Vec<float, 3>& rgb);

  /// Adds a point to the color function. If the point already exists, it
  /// will be updated to the new value.
  ///
  /// Note: hsv values need to be between 0 and 1.0 (inclusive).
  /// Return the index of the point (0 based), or -1 on error.
  svtkm::Int32 AddPointHSV(double x, const svtkm::Vec<float, 3>& hsv);

  /// Add a line segment to the color function. All points which lay between x1 and x2
  /// (inclusive) are removed from the function.
  ///
  /// Note: rgb1, and rgb2 values need to be between 0 and 1.0 (inclusive).
  /// Return the index of the point x1 (0 based), or -1 on error.
  svtkm::Int32 AddSegment(double x1,
                         const svtkm::Vec<float, 3>& rgb1,
                         double x2,
                         const svtkm::Vec<float, 3>& rgb2);

  /// Add a line segment to the color function. All points which lay between x1 and x2
  /// (inclusive) are removed from the function.
  ///
  /// Note: hsv1, and hsv2 values need to be between 0 and 1.0 (inclusive)
  /// Return the index of the point x1 (0 based), or -1 on error
  svtkm::Int32 AddSegmentHSV(double x1,
                            const svtkm::Vec<float, 3>& hsv1,
                            double x2,
                            const svtkm::Vec<float, 3>& hsv2);

  /// Get the location, and rgb information for an existing point in the opacity function.
  ///
  /// Note: components 1-3 are rgb and will have values between 0 and 1.0 (inclusive)
  /// Return the index of the point (0 based), or -1 on error.
  bool GetPoint(svtkm::Int32 index, svtkm::Vec<double, 4>&) const;

  /// Update the location, and rgb information for an existing point in the color function.
  /// If the location value for the index is modified the point is removed from
  /// the function and re-inserted in the proper sorted location.
  ///
  /// Note: components 1-3 are rgb and must have values between 0 and 1.0 (inclusive).
  /// Return the new index of the updated point (0 based), or -1 on error.
  svtkm::Int32 UpdatePoint(svtkm::Int32 index, const svtkm::Vec<double, 4>&);

  /// Remove the Color function point that exists at exactly x
  ///
  /// Return true if the point x exists and has been removed
  bool RemovePoint(double x);

  /// Remove the Color function point n
  ///
  /// Return true if n >= 0 && n < GetNumberOfPoints
  bool RemovePoint(svtkm::Int32 index);

  /// Returns the number of points in the color function
  svtkm::Int32 GetNumberOfPoints() const;

  // Functions for Opacity

  /// Adds a point to the opacity function. If the point already exists, it
  /// will be updated to the new value. Uses a midpoint of 0.5 (halfway between the control points)
  /// and sharpness of 0.0 (linear).
  ///
  /// Note: alpha needs to be a value between 0 and 1.0 (inclusive).
  /// Return the index of the point (0 based), or -1 on error.
  svtkm::Int32 AddPointAlpha(double x, float alpha) { return AddPointAlpha(x, alpha, 0.5f, 0.0f); }

  /// Adds a point to the opacity function. If the point already exists, it
  /// will be updated to the new value.
  ///
  /// Note: alpha, midpoint, and sharpness values need to be between 0 and 1.0 (inclusive)
  /// Return the index of the point (0 based), or -1 on error.
  svtkm::Int32 AddPointAlpha(double x, float alpha, float midpoint, float sharpness);

  /// Add a line segment to the opacity function. All points which lay between x1 and x2
  /// (inclusive) are removed from the function. Uses a midpoint of
  /// 0.5 (halfway between the control points) and sharpness of 0.0 (linear).
  ///
  /// Note: alpha values need to be between 0 and 1.0 (inclusive)
  /// Return the index of the point x1 (0 based), or -1 on error
  svtkm::Int32 AddSegmentAlpha(double x1, float alpha1, double x2, float alpha2)
  {
    svtkm::Vec<float, 2> mid_sharp(0.5f, 0.0f);
    return AddSegmentAlpha(x1, alpha1, x2, alpha2, mid_sharp, mid_sharp);
  }

  /// Add a line segment to the opacity function. All points which lay between x1 and x2
  /// (inclusive) are removed from the function.
  ///
  /// Note: alpha, midpoint, and sharpness values need to be between 0 and 1.0 (inclusive)
  /// Return the index of the point x1 (0 based), or -1 on error
  svtkm::Int32 AddSegmentAlpha(double x1,
                              float alpha1,
                              double x2,
                              float alpha2,
                              const svtkm::Vec<float, 2>& mid_sharp1,
                              const svtkm::Vec<float, 2>& mid_sharp2);

  /// Get the location, alpha, midpoint and sharpness information for an existing
  /// point in the opacity function.
  ///
  /// Note: alpha, midpoint, and sharpness values all will be between 0 and 1.0 (inclusive)
  /// Return the index of the point (0 based), or -1 on error.
  bool GetPointAlpha(svtkm::Int32 index, svtkm::Vec<double, 4>&) const;

  /// Update the location, alpha, midpoint and sharpness information for an existing
  /// point in the opacity function.
  /// If the location value for the index is modified the point is removed from
  /// the function and re-inserted in the proper sorted location
  ///
  /// Note: alpha, midpoint, and sharpness values need to be between 0 and 1.0 (inclusive)
  /// Return the new index of the updated point (0 based), or -1 on error.
  svtkm::Int32 UpdatePointAlpha(svtkm::Int32 index, const svtkm::Vec<double, 4>&);

  /// Remove the Opacity function point that exists at exactly x
  ///
  /// Return true if the point x exists and has been removed
  bool RemovePointAlpha(double x);

  /// Remove the Opacity function point n
  ///
  /// Return true if n >= 0 && n < GetNumberOfPointsAlpha
  bool RemovePointAlpha(svtkm::Int32 index);

  /// Returns the number of points in the alpha function
  svtkm::Int32 GetNumberOfPointsAlpha() const;

  /// Fill the Color table from a double pointer
  ///
  /// The double pointer is required to have the layout out of [X1, R1,
  /// G1, B1, X2, R2, G2, B2, ..., Xn, Rn, Gn, Bn] where n is the
  /// number of nodes.
  /// This will remove any existing color control points.
  ///
  /// Note: n represents the length of the array, so ( n/4 == number of control points )
  ///
  /// Note: This is provided as a interoperability method with SVTK
  /// Will return false and not modify anything if n is <= 0 or ptr == nullptr
  bool FillColorTableFromDataPointer(svtkm::Int32 n, const double* ptr);

  /// Fill the Color table from a float pointer
  ///
  /// The double pointer is required to have the layout out of [X1, R1,
  /// G1, B1, X2, R2, G2, B2, ..., Xn, Rn, Gn, Bn] where n is the
  /// number of nodes.
  /// This will remove any existing color control points.
  ///
  /// Note: n represents the length of the array, so ( n/4 == number of control points )
  ///
  /// Note: This is provided as a interoperability method with SVTK
  /// Will return false and not modify anything if n is <= 0 or ptr == nullptr
  bool FillColorTableFromDataPointer(svtkm::Int32 n, const float* ptr);

  /// Fill the Opacity table from a double pointer
  ///
  /// The double pointer is required to have the layout out of [X1, A1, M1, S1, X2, A2, M2, S2,
  /// ..., Xn, An, Mn, Sn] where n is the number of nodes. The Xi values represent the value to
  /// map, the Ai values represent alpha (opacity) value, the Mi values represent midpoints, and
  /// the Si values represent sharpness. Use 0.5 for midpoint and 0.0 for sharpness to have linear
  /// interpolation of the alpha.
  ///
  /// This will remove any existing opacity control points.
  ///
  /// Note: n represents the length of the array, so ( n/4 == number of control points )
  ///
  /// Will return false and not modify anything if n is <= 0 or ptr == nullptr
  bool FillOpacityTableFromDataPointer(svtkm::Int32 n, const double* ptr);

  /// Fill the Opacity table from a float pointer
  ///
  /// The float pointer is required to have the layout out of [X1, A1, M1, S1, X2, A2, M2, S2,
  /// ..., Xn, An, Mn, Sn] where n is the number of nodes. The Xi values represent the value to
  /// map, the Ai values represent alpha (opacity) value, the Mi values represent midpoints, and
  /// the Si values represent sharpness. Use 0.5 for midpoint and 0.0 for sharpness to have linear
  /// interpolation of the alpha.
  ///
  /// This will remove any existing opacity control points.
  ///
  /// Note: n represents the length of the array, so ( n/4 == number of control points )
  ///
  /// Will return false and not modify anything if n is <= 0 or ptr == nullptr
  bool FillOpacityTableFromDataPointer(svtkm::Int32 n, const float* ptr);


  /// \brief Sample each value through an intermediate lookup/sample table to generate RGBA colors
  ///
  /// Each value in \c values is binned based on its value in relationship to the range
  /// of the color table and will use the color value at that bin from the \c samples.
  /// To generate the lookup table use \c Sample .
  ///
  /// Here is a simple example.
  /// \code{.cpp}
  ///
  /// svtkm::cont::ColorTableSamplesRGBA samples;
  /// svtkm::cont::ColorTable table("black-body radiation");
  /// table.Sample(256, samples);
  /// svtkm::cont::ArrayHandle<svtkm::Vec4ui_8> colors;
  /// table.Map(input, samples, colors);
  ///
  /// \endcode
  template <typename T, typename S>
  inline bool Map(const svtkm::cont::ArrayHandle<T, S>& values,
                  const svtkm::cont::ColorTableSamplesRGBA& samples,
                  svtkm::cont::ArrayHandle<svtkm::Vec4ui_8>& rgbaOut) const;

  /// \brief Sample each value through an intermediate lookup/sample table to generate RGB colors
  ///
  /// Each value in \c values is binned based on its value in relationship to the range
  /// of the color table and will use the color value at that bin from the \c samples.
  /// To generate the lookup table use \c Sample .
  ///
  /// Here is a simple example.
  /// \code{.cpp}
  ///
  /// svtkm::cont::ColorTableSamplesRGB samples;
  /// svtkm::cont::ColorTable table("black-body radiation");
  /// table.Sample(256, samples);
  /// svtkm::cont::ArrayHandle<svtkm::Vec3ui_8> colors;
  /// table.Map(input, samples, colors);
  ///
  /// \endcode
  template <typename T, typename S>
  inline bool Map(const svtkm::cont::ArrayHandle<T, S>& values,
                  const svtkm::cont::ColorTableSamplesRGB& samples,
                  svtkm::cont::ArrayHandle<svtkm::Vec3ui_8>& rgbaOut) const;

  /// \brief Use magnitude of a vector with a sample table to generate RGBA colors
  ///
  template <typename T, int N, typename S>
  inline bool MapMagnitude(const svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, S>& values,
                           const svtkm::cont::ColorTableSamplesRGBA& samples,
                           svtkm::cont::ArrayHandle<svtkm::Vec4ui_8>& rgbaOut) const;

  /// \brief Use magnitude of a vector with a sample table to generate RGB colors
  ///
  template <typename T, int N, typename S>
  inline bool MapMagnitude(const svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, S>& values,
                           const svtkm::cont::ColorTableSamplesRGB& samples,
                           svtkm::cont::ArrayHandle<svtkm::Vec3ui_8>& rgbaOut) const;

  /// \brief Use a single component of a vector with a sample table to generate RGBA colors
  ///
  template <typename T, int N, typename S>
  inline bool MapComponent(const svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, S>& values,
                           svtkm::IdComponent comp,
                           const svtkm::cont::ColorTableSamplesRGBA& samples,
                           svtkm::cont::ArrayHandle<svtkm::Vec4ui_8>& rgbaOut) const;

  /// \brief Use a single component of a vector with a sample table to generate RGB colors
  ///
  template <typename T, int N, typename S>
  inline bool MapComponent(const svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, S>& values,
                           svtkm::IdComponent comp,
                           const svtkm::cont::ColorTableSamplesRGB& samples,
                           svtkm::cont::ArrayHandle<svtkm::Vec3ui_8>& rgbOut) const;


  /// \brief Interpolate each value through the color table to generate RGBA colors
  ///
  /// Each value in \c values will be sampled through the entire color table
  /// to determine a color.
  ///
  /// Note: This is more costly than using Sample/Map with the generated intermediate lookup table
  template <typename T, typename S>
  inline bool Map(const svtkm::cont::ArrayHandle<T, S>& values,
                  svtkm::cont::ArrayHandle<svtkm::Vec4ui_8>& rgbaOut) const;

  /// \brief Interpolate each value through the color table to generate RGB colors
  ///
  /// Each value in \c values will be sampled through the entire color table
  /// to determine a color.
  ///
  /// Note: This is more costly than using Sample/Map with the generated intermediate lookup table
  template <typename T, typename S>
  inline bool Map(const svtkm::cont::ArrayHandle<T, S>& values,
                  svtkm::cont::ArrayHandle<svtkm::Vec3ui_8>& rgbOut) const;

  /// \brief Use magnitude of a vector to generate RGBA colors
  ///
  template <typename T, int N, typename S>
  inline bool MapMagnitude(const svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, S>& values,
                           svtkm::cont::ArrayHandle<svtkm::Vec4ui_8>& rgbaOut) const;

  /// \brief Use magnitude of a vector to generate RGB colors
  ///
  template <typename T, int N, typename S>
  inline bool MapMagnitude(const svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, S>& values,
                           svtkm::cont::ArrayHandle<svtkm::Vec3ui_8>& rgbOut) const;

  /// \brief Use a single component of a vector to generate RGBA colors
  ///
  template <typename T, int N, typename S>
  inline bool MapComponent(const svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, S>& values,
                           svtkm::IdComponent comp,
                           svtkm::cont::ArrayHandle<svtkm::Vec4ui_8>& rgbaOut) const;

  /// \brief Use a single component of a vector to generate RGB colors
  ///
  template <typename T, int N, typename S>
  inline bool MapComponent(const svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, S>& values,
                           svtkm::IdComponent comp,
                           svtkm::cont::ArrayHandle<svtkm::Vec3ui_8>& rgbOut) const;


  /// \brief generate RGB colors using regular spaced samples along the range.
  ///
  /// Will use the current range of the color table to generate evenly spaced
  /// values using either svtkm::Float32 or svtkm::Float64 space.
  /// Will use svtkm::Float32 space when the difference between the float and double
  /// values when the range is within float space and the following are within a tolerance:
  ///
  /// - (max-min) / numSamples
  /// - ((max-min) / numSamples) * numSamples
  ///
  /// Note: This will return false if the number of samples is less than 2
  inline bool Sample(svtkm::Int32 numSamples,
                     svtkm::cont::ColorTableSamplesRGBA& samples,
                     double tolerance = 0.002) const;

  /// \brief generate a sample lookup table using regular spaced samples along the range.
  ///
  /// Will use the current range of the color table to generate evenly spaced
  /// values using either svtkm::Float32 or svtkm::Float64 space.
  /// Will use svtkm::Float32 space when the difference between the float and double
  /// values when the range is within float space and the following are within a tolerance:
  ///
  /// - (max-min) / numSamples
  /// - ((max-min) / numSamples) * numSamples
  ///
  /// Note: This will return false if the number of samples is less than 2
  inline bool Sample(svtkm::Int32 numSamples,
                     svtkm::cont::ColorTableSamplesRGB& samples,
                     double tolerance = 0.002) const;

  /// \brief generate RGBA colors using regular spaced samples along the range.
  ///
  /// Will use the current range of the color table to generate evenly spaced
  /// values using either svtkm::Float32 or svtkm::Float64 space.
  /// Will use svtkm::Float32 space when the difference between the float and double
  /// values when the range is within float space and the following are within a tolerance:
  ///
  /// - (max-min) / numSamples
  /// - ((max-min) / numSamples) * numSamples
  ///
  /// Note: This will return false if the number of samples is less than 2
  inline bool Sample(svtkm::Int32 numSamples,
                     svtkm::cont::ArrayHandle<svtkm::Vec4ui_8>& colors,
                     double tolerance = 0.002) const;

  /// \brief generate RGB colors using regular spaced samples along the range.
  ///
  /// Will use the current range of the color table to generate evenly spaced
  /// values using either svtkm::Float32 or svtkm::Float64 space.
  /// Will use svtkm::Float32 space when the difference between the float and double
  /// values when the range is within float space and the following are within a tolerance:
  ///
  /// - (max-min) / numSamples
  /// - ((max-min) / numSamples) * numSamples
  ///
  /// Note: This will return false if the number of samples is less than 2
  inline bool Sample(svtkm::Int32 numSamples,
                     svtkm::cont::ArrayHandle<svtkm::Vec3ui_8>& colors,
                     double tolerance = 0.002) const;


  /// \brief returns a virtual object pointer of the exec color table
  ///
  /// This pointer is only valid as long as the ColorTable is unmodified
  inline const svtkm::exec::ColorTableBase* PrepareForExecution(
    svtkm::cont::DeviceAdapterId deviceId) const;

  /// \brief returns the modified count for the virtual object handle of the exec color table
  ///
  /// The modified count allows consumers of a shared color table to keep track
  /// if the color table has been modified since the last time they used it.
  svtkm::Id GetModifiedCount() const;

  struct TransferState
  {
    bool NeedsTransfer;
    svtkm::exec::ColorTableBase* Portal;
    const svtkm::cont::ArrayHandle<double>& ColorPosHandle;
    const svtkm::cont::ArrayHandle<svtkm::Vec<float, 3>>& ColorRGBHandle;
    const svtkm::cont::ArrayHandle<double>& OpacityPosHandle;
    const svtkm::cont::ArrayHandle<float>& OpacityAlphaHandle;
    const svtkm::cont::ArrayHandle<svtkm::Vec<float, 2>>& OpacityMidSharpHandle;
  };

private:
  bool NeedToCreateExecutionColorTable() const;

  //takes ownership of the pointer passed in
  void UpdateExecutionColorTable(
    svtkm::cont::VirtualObjectHandle<svtkm::exec::ColorTableBase>*) const;

  ColorTable::TransferState GetExecutionDataForTransfer() const;

  svtkm::exec::ColorTableBase* GetControlRepresentation() const;

  svtkm::cont::VirtualObjectHandle<svtkm::exec::ColorTableBase> const* GetExecutionHandle() const;
};
}
} //namespace svtkm::cont
#endif //svtk_m_cont_ColorTable_h
