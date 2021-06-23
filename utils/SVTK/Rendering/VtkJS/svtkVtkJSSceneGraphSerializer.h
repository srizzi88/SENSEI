/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVtkJSSceneGraphSerializer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVtkJSSceneGraphSerializer
 * @brief   Converts elements of a SVTK scene graph into svtk-js elements
 *
 * svtkVtkJSSceneGraphSerializer accepts nodes and their renderables from a scene
 * graph and a) composes the elements' data and topology into a Json data
 * structure and b) correlates unique identifiers for data objects in the Json
 * data structure to the data objects themselves. It is designed to operate with
 * svtkVtkJSViewNodeFactory, which handles the logic for scene graph traversal.
 *
 * When constructing the Json description for a single scene graph node and its
 * renderable, the Add(node, renderable) step processes the renderable into its
 * corresponding svtk-js form. For many renderables this is a no-op, but current
 * restrictions in svtk-js (such as the lack of support for composite mappers and
 * the requirement for data conversion to svtkPolyData) require a nontrival
 * conversion step for certain renderable types. The subsequent
 * ToJson(renderable) is a straightforward conversion of the renderable's data
 * members into a svtk-js Json format.
 *
 *
 * @sa
 * svtkVtkJSViewNodeFactory
 */

#ifndef svtkVtkJSSceneGraphSerializer_h
#define svtkVtkJSSceneGraphSerializer_h

#include "svtkRenderingVtkJSModule.h" // For export macro

#include "svtkObject.h"
#include <svtk_jsoncpp.h> // For Json::Value

class svtkActor;
class svtkAlgorithm;
class svtkCamera;
class svtkCompositePolyDataMapper;
class svtkCompositePolyDataMapper2;
class svtkDataArray;
class svtkDataObject;
class svtkDataSet;
class svtkGlyph3DMapper;
class svtkImageData;
class svtkLight;
class svtkLookupTable;
class svtkMapper;
class svtkPolyData;
class svtkProperty;
class svtkRenderer;
class svtkRenderWindow;
class svtkTexture;
class svtkTransform;
class svtkViewNode;

class SVTKRENDERINGSVTKJS_EXPORT svtkVtkJSSceneGraphSerializer : public svtkObject
{
public:
  static svtkVtkJSSceneGraphSerializer* New();
  svtkTypeMacro(svtkVtkJSSceneGraphSerializer, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Empty the contents of the scene and the reset the unique id generator.
   */
  void Reset();
  //@}

  //@{
  /**
   * Access the Json description of the constructed scene. The returned object
   * is valid for the lifetime of this class.
   */
  const Json::Value& GetRoot() const;
  //@}

  //@{
  /**
   * Access the data objects referenced in the constructed scene.
   */
  svtkIdType GetNumberOfDataObjects() const;
  Json::ArrayIndex GetDataObjectId(svtkIdType) const;
  svtkDataObject* GetDataObject(svtkIdType) const;
  //@}

  //@{
  /**
   * Access the data arrays referenced in the constructed scene.
   */
  svtkIdType GetNumberOfDataArrays() const;
  std::string GetDataArrayId(svtkIdType) const;
  svtkDataArray* GetDataArray(svtkIdType) const;
  //@}

  //@{
  /**
   * Add a scene graph node and its corresponding renderable to the scene.
   */
  virtual void Add(svtkViewNode*, svtkActor*);
  virtual void Add(svtkViewNode*, svtkCompositePolyDataMapper*);
  virtual void Add(svtkViewNode*, svtkCompositePolyDataMapper2*);
  virtual void Add(svtkViewNode*, svtkGlyph3DMapper*);
  virtual void Add(svtkViewNode*, svtkMapper*);
  virtual void Add(svtkViewNode*, svtkRenderer*);
  virtual void Add(svtkViewNode*, svtkRenderWindow*);
  //@}

protected:
  svtkVtkJSSceneGraphSerializer();
  ~svtkVtkJSSceneGraphSerializer() override;

  //@{
  /**
   * Translate from a SVTK renderable to a svtk-js renderable.
   */
  virtual Json::Value ToJson(svtkDataArray*);
  virtual Json::Value ToJson(Json::Value&, svtkAlgorithm*, svtkDataObject*);
  virtual Json::Value ToJson(Json::Value&, svtkActor*, bool newPropertyId = false);
  virtual Json::Value ToJson(Json::Value&, Json::ArrayIndex, svtkGlyph3DMapper*);
  virtual Json::Value ToJson(Json::Value&, svtkCamera*);
  virtual Json::Value ToJson(Json::Value&, svtkAlgorithm*, svtkImageData*);
  virtual Json::Value ToJson(Json::Value&, svtkLight*);
  virtual Json::Value ToJson(Json::Value&, svtkLookupTable*);
  virtual Json::Value ToJson(Json::Value&, Json::ArrayIndex, svtkMapper*, bool newLUTId = false);
  virtual Json::Value ToJson(Json::Value&, svtkRenderer*);
  virtual Json::Value ToJson(Json::Value&, svtkAlgorithm*, svtkPolyData*);
  virtual Json::Value ToJson(Json::Value&, svtkProperty*);
  virtual Json::Value ToJson(Json::Value&, svtkTexture*);
  virtual Json::Value ToJson(Json::Value&, svtkTransform*);
  virtual Json::Value ToJson(svtkRenderWindow*);
  //@}

  //@{
  /**
   * Associate a unique id with a given object. Subsequent calls to this method
   * with the same object will return the same unique id.
   */
  Json::ArrayIndex UniqueId(void* ptr = nullptr);
  //@}

  struct Internal;
  Internal* Internals;

private:
  svtkVtkJSSceneGraphSerializer(const svtkVtkJSSceneGraphSerializer&) = delete;
  void operator=(const svtkVtkJSSceneGraphSerializer&) = delete;

  virtual void Add(Json::Value*, svtkAlgorithm*);

  template <typename CompositeMapper>
  void Add(svtkViewNode* node, svtkDataObject* dataObject, CompositeMapper* mapper);

  void extractRequiredFields(Json::Value& extractedFields, svtkMapper* mapper, svtkDataSet* dataSet);
};

#endif
