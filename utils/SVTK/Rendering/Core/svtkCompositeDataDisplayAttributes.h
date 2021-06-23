/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeDataDisplayAttributes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCompositeDataDisplayAttributes
 * @brief   Rendering attributes for a multi-block dataset.
 *
 * The svtkCompositeDataDisplayAttributes class stores display attributes
 * for individual blocks in a multi-block dataset. It uses the actual data
 * block's pointer as a key (svtkDataObject*).
 *
 * @warning It is considered unsafe to dereference key pointers at any time,
 * they should only serve as keys to access the internal map.
 */

#ifndef svtkCompositeDataDisplayAttributes_h
#define svtkCompositeDataDisplayAttributes_h
#include <functional>    // for std::function
#include <unordered_map> // for std::unordered_map

#include "svtkColor.h" // for svtkColor3d
#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // for export macro

class svtkBoundingBox;
class svtkDataObject;

class SVTKRENDERINGCORE_EXPORT svtkCompositeDataDisplayAttributes : public svtkObject
{
public:
  static svtkCompositeDataDisplayAttributes* New();
  svtkTypeMacro(svtkCompositeDataDisplayAttributes, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Returns true if any block has any block visibility is set.
   */
  bool HasBlockVisibilities() const;

  //@{
  /**
   * Set/get the visibility for the block with \p data_object.
   */
  void SetBlockVisibility(svtkDataObject* data_object, bool visible);
  bool GetBlockVisibility(svtkDataObject* data_object) const;
  //@}

  /**
   * Returns true if the block with the given data_object has a visibility
   * set.
   */
  bool HasBlockVisibility(svtkDataObject* data_object) const;

  /**
   * Removes the block visibility flag for the block with data_object.
   */
  void RemoveBlockVisibility(svtkDataObject* data_object);

  /**
   * Removes all block visibility flags. This effectively sets the visibility
   * for all blocks to true.
   */
  void RemoveBlockVisibilities();
  // This method is deprecated and will be removed in SVTK 8.2. It is misspelled.
  SVTK_LEGACY(void RemoveBlockVisibilites());

  /**
   * Returns true if any block has any block pickability is set.
   */
  bool HasBlockPickabilities() const;

  //@{
  /**
   * Set/get the pickability for the block with \p data_object.
   */
  void SetBlockPickability(svtkDataObject* data_object, bool visible);
  bool GetBlockPickability(svtkDataObject* data_object) const;
  //@}

  /**
   * Returns true if the block with the given data_object has a pickability
   * set.
   */
  bool HasBlockPickability(svtkDataObject* data_object) const;

  /**
   * Removes the block pickability flag for the block with data_object.
   */
  void RemoveBlockPickability(svtkDataObject* data_object);

  /**
   * Removes all block pickability flags. This effectively sets the pickability
   * for all blocks to true.
   */
  void RemoveBlockPickabilities();

  //@{
  /**
   * Set/get the color for the block with \p data_object.
   */
  void SetBlockColor(svtkDataObject* data_object, const double color[3]);
  void GetBlockColor(svtkDataObject* data_object, double color[3]) const;
  svtkColor3d GetBlockColor(svtkDataObject* data_object) const;
  //@}

  /**
   * Returns true if any block has any block color is set.
   */
  bool HasBlockColors() const;

  /**
   * Returns true if the block with the given \p data_object has a color.
   */
  bool HasBlockColor(svtkDataObject* data_object) const;

  /**
   * Removes the block color for the block with \p data_object.
   */
  void RemoveBlockColor(svtkDataObject* data_object);

  /**
   * Removes all block colors.
   */
  void RemoveBlockColors();

  //@{
  /**
   * Set/get the opacity for the block with data_object.
   */
  void SetBlockOpacity(svtkDataObject* data_object, double opacity);
  double GetBlockOpacity(svtkDataObject* data_object) const;
  //@}

  /**
   * Returns true if any block has an opacity set.
   */
  bool HasBlockOpacities() const;

  /**
   * Returns true if the block with data_object has an opacity set.
   */
  bool HasBlockOpacity(svtkDataObject* data_object) const;

  /**
   * Removes the set opacity for the block with data_object.
   */
  void RemoveBlockOpacity(svtkDataObject* data_object);

  /**
   * Removes all block opacities.
   */
  void RemoveBlockOpacities();

  //@{
  /**
   * Set/get the material for the block with data_object.
   * Only rendering backends that support advanced materials need to respect these.
   */
  void SetBlockMaterial(svtkDataObject* data_object, const std::string& material);
  const std::string& GetBlockMaterial(svtkDataObject* data_object) const;
  //@}

  /**
   * Returns true if any block has an material set.
   */
  bool HasBlockMaterials() const;

  /**
   * Returns true if the block with data_object has an material set.
   */
  bool HasBlockMaterial(svtkDataObject* data_object) const;

  /**
   * Removes the set material for the block with data_object.
   */
  void RemoveBlockMaterial(svtkDataObject* data_object);

  /**
   * Removes all block materialss.
   */
  void RemoveBlockMaterials();

  /**
   * If the input \a dobj is a svtkCompositeDataSet, we will loop over the
   * hierarchy recursively starting from initial index 0 and use only visible
   * blocks, which is specified in the svtkCompositeDataDisplayAttributes \a cda,
   * to compute the \a bounds.
   */
  static void ComputeVisibleBounds(
    svtkCompositeDataDisplayAttributes* cda, svtkDataObject* dobj, double bounds[6]);

  /**
   * Get the DataObject corresponding to the node with index flat_index under
   * parent_obj. Traverses the entire hierarchy recursively.
   */
  static svtkDataObject* DataObjectFromIndex(
    const unsigned int flat_index, svtkDataObject* parent_obj, unsigned int& current_flat_index);

  void VisitVisibilities(std::function<bool(svtkDataObject*, bool)> visitor)
  {
    for (auto entry : this->BlockVisibilities)
    {
      if (visitor(entry.first, entry.second))
      {
        break;
      }
    }
  }

protected:
  svtkCompositeDataDisplayAttributes();
  ~svtkCompositeDataDisplayAttributes() override;

private:
  svtkCompositeDataDisplayAttributes(const svtkCompositeDataDisplayAttributes&) = delete;
  void operator=(const svtkCompositeDataDisplayAttributes&) = delete;

  /**
   * If the input data \a dobj is a svtkCompositeDataSet, we will
   * loop over the hierarchy recursively starting from the initial block
   * and use only visible blocks, which is specified in the
   * svtkCompositeDataDisplayAttributes \a cda, to compute bounds and the
   * result bounds will be set to the svtkBoundingBox \a bbox. The \a parentVisible
   * is the visibility for the starting block.
   */
  static void ComputeVisibleBoundsInternal(svtkCompositeDataDisplayAttributes* cda,
    svtkDataObject* dobj, svtkBoundingBox* bbox, bool parentVisible = true);

  using BoolMap = std::unordered_map<svtkDataObject*, bool>;
  using DoubleMap = std::unordered_map<svtkDataObject*, double>;
  using ColorMap = std::unordered_map<svtkDataObject*, svtkColor3d>;
  using StringMap = std::unordered_map<svtkDataObject*, std::string>;

  BoolMap BlockVisibilities;
  ColorMap BlockColors;
  DoubleMap BlockOpacities;
  StringMap BlockMaterials;
  BoolMap BlockPickabilities;
};

#endif // svtkCompositeDataDisplayAttributes_h
