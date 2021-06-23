/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeDataDisplayAttributesLegacy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCompositeDataDisplayAttributesLegacy
 * @brief   rendering attributes for a
 * multi-block dataset.
 *
 * The svtkCompositeDataDisplayAttributesLegacy class stores display attributes
 * for individual blocks in a multi-block dataset. Attributes are mapped to
 * blocks through their flat-index; This is the mechanism used in legacy
 * OpenGL classes.
 */

#ifndef svtkCompositeDataDisplayAttributesLegacy_h
#define svtkCompositeDataDisplayAttributesLegacy_h

#include "svtkColor.h" // for svtkColor3d
#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // for export macro

#include <map> // for std::map

class svtkBoundingBox;
class svtkDataObject;

class SVTKRENDERINGCORE_EXPORT svtkCompositeDataDisplayAttributesLegacy : public svtkObject
{
public:
  static svtkCompositeDataDisplayAttributesLegacy* New();
  svtkTypeMacro(svtkCompositeDataDisplayAttributesLegacy, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Returns true if any block has any block visibility is set.
   */
  bool HasBlockVisibilities() const;

  //@{
  /**
   * Set/get the visibility for the block with \p flat_index.
   */
  void SetBlockVisibility(unsigned int flat_index, bool visible);
  bool GetBlockVisibility(unsigned int flat_index) const;
  //@}

  /**
   * Returns true if the block with the given flat_index has a visibility
   * set.
   */
  bool HasBlockVisibility(unsigned int flat_index) const;

  /**
   * Removes the block visibility flag for the block with flat_index.
   */
  void RemoveBlockVisibility(unsigned int flat_index);

  /**
   * Removes all block visibility flags. The effectively sets the visibility
   * for all blocks to true.
   */
  void RemoveBlockVisibilities();
  // This method is deprecated and will be removed in SVTK 8.2. It is misspelled.
  SVTK_LEGACY(void RemoveBlockVisibilites());

  /**
   * Returns true if any block has any block visibility is set.
   */
  bool HasBlockPickabilities() const;

  //@{
  /**
   * Set/get the visibility for the block with \p flat_index.
   */
  void SetBlockPickability(unsigned int flat_index, bool visible);
  bool GetBlockPickability(unsigned int flat_index) const;
  //@}

  /**
   * Returns true if the block with the given flat_index has a visibility
   * set.
   */
  bool HasBlockPickability(unsigned int flat_index) const;

  /**
   * Removes the block visibility flag for the block with flat_index.
   */
  void RemoveBlockPickability(unsigned int flat_index);

  /**
   * Removes all block visibility flags. The effectively sets the visibility
   * for all blocks to true.
   */
  void RemoveBlockPickabilities();

  //@{
  /**
   * Set/get the color for the block with \p flat_index.
   */
  void SetBlockColor(unsigned int flat_index, const double color[3]);
  void GetBlockColor(unsigned int flat_index, double color[3]) const;
  svtkColor3d GetBlockColor(unsigned int flat_index) const;
  //@}

  /**
   * Returns true if any block has any block color is set.
   */
  bool HasBlockColors() const;

  /**
   * Returns true if the block with the given \p flat_index has a color.
   */
  bool HasBlockColor(unsigned int flat_index) const;

  /**
   * Removes the block color for the block with \p flat_index.
   */
  void RemoveBlockColor(unsigned int flat_index);

  /**
   * Removes all block colors.
   */
  void RemoveBlockColors();

  //@{
  /**
   * Set/get the opacity for the block with flat_index.
   */
  void SetBlockOpacity(unsigned int flat_index, double opacity);
  double GetBlockOpacity(unsigned int flat_index) const;
  //@}

  /**
   * Returns true if any block has an opacity set.
   */
  bool HasBlockOpacities() const;

  /**
   * Returns true if the block with flat_index has an opacity set.
   */
  bool HasBlockOpacity(unsigned int flat_index) const;

  /**
   * Removes the set opacity for the block with flat_index.
   */
  void RemoveBlockOpacity(unsigned int flat_index);

  /**
   * Removes all block opacities.
   */
  void RemoveBlockOpacities();

  // If the input \a dobj is a svtkCompositeDataSet, we will loop over the
  // hierarchy recursively starting from initial index 0 and use only visible
  // blocks, which is specified in the svtkCompositeDataDisplayAttributesLegacy \a cda,
  // to compute the \a bounds.
  static void ComputeVisibleBounds(
    svtkCompositeDataDisplayAttributesLegacy* cda, svtkDataObject* dobj, double bounds[6]);

protected:
  svtkCompositeDataDisplayAttributesLegacy();
  ~svtkCompositeDataDisplayAttributesLegacy() override;

private:
  svtkCompositeDataDisplayAttributesLegacy(const svtkCompositeDataDisplayAttributesLegacy&) = delete;
  void operator=(const svtkCompositeDataDisplayAttributesLegacy&) = delete;

  /**
   * If the input data \a dobj is a svtkCompositeDataSet, we will
   * loop over the hierarchy recursively starting from initial index
   * \a flat_index and use only visible blocks, which is
   * specified in the svtkCompositeDataDisplayAttributesLegacy \a cda,
   * to compute bounds and the result bounds will be set to
   * the svtkBoundingBox \a bbox. The \a paraentVisible is the
   * visibility for the starting flat_index.
   */
  static void ComputeVisibleBoundsInternal(svtkCompositeDataDisplayAttributesLegacy* cda,
    svtkDataObject* dobj, unsigned int& flat_index, svtkBoundingBox* bbox, bool parentVisible = true);

  std::map<unsigned int, bool> BlockVisibilities;
  std::map<unsigned int, svtkColor3d> BlockColors;
  std::map<unsigned int, double> BlockOpacities;
  std::map<unsigned int, bool> BlockPickabilities;
};

#endif // svtkCompositeDataDisplayAttributesLegacy_h
