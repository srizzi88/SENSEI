/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataRepresentation.h

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
 * @class   svtkDataRepresentation
 * @brief   The superclass for all representations
 *
 *
 * svtkDataRepresentation the superclass for representations of data objects.
 * This class itself may be instantiated and used as a representation
 * that simply holds a connection to a pipeline.
 *
 * If there are multiple representations present in a view, you should use
 * a subclass of svtkDataRepresentation.  The representation is
 * responsible for taking the input pipeline connection and converting it
 * to an object usable by a view.  In the most common case, the representation
 * will contain the pipeline necessary to convert a data object into an actor
 * or set of actors.
 *
 * The representation has a concept of a selection.  If the user performs a
 * selection operation on the view, the view forwards this on to its
 * representations.  The representation is responsible for displaying that
 * selection in an appropriate way.
 *
 * Representation selections may also be linked.  The representation shares
 * the selection by converting it into a view-independent format, then
 * setting the selection on its svtkAnnotationLink.  Other representations
 * sharing the same selection link instance will get the same selection
 * from the selection link when the view is updated.  The application is
 * responsible for linking representations as appropriate by setting the
 * same svtkAnnotationLink on each linked representation.
 */

#ifndef svtkDataRepresentation_h
#define svtkDataRepresentation_h

#include "svtkPassInputTypeAlgorithm.h"
#include "svtkViewsCoreModule.h" // For export macro

class svtkAlgorithmOutput;
class svtkAnnotationLayers;
class svtkAnnotationLink;
class svtkDataObject;
class svtkSelection;
class svtkStringArray;
class svtkTrivialProducer;
class svtkView;
class svtkViewTheme;

class SVTKVIEWSCORE_EXPORT svtkDataRepresentation : public svtkPassInputTypeAlgorithm
{
public:
  static svtkDataRepresentation* New();
  svtkTypeMacro(svtkDataRepresentation, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Convenience override method for obtaining the input connection
   * without specifying the port or index.
   */
  svtkAlgorithmOutput* GetInputConnection(int port = 0, int index = 0)
  {
    return this->Superclass::GetInputConnection(port, index);
  }

  /**
   * The annotation link for this representation.
   * To link annotations, set the same svtkAnnotationLink object in
   * multiple representations.
   */
  svtkAnnotationLink* GetAnnotationLink() { return this->AnnotationLinkInternal; }
  void SetAnnotationLink(svtkAnnotationLink* link);

  /**
   * Apply a theme to this representation.
   * Subclasses should override this method.
   */
  virtual void ApplyViewTheme(svtkViewTheme* svtkNotUsed(theme)) {}

  /**
   * The view calls this method when a selection occurs.
   * The representation takes this selection and converts it into
   * a selection on its data by calling ConvertSelection,
   * then calls UpdateSelection with the converted selection.
   * Subclasses should not override this method, but should instead
   * override ConvertSelection.
   * The optional third argument specifies whether the selection should be
   * added to the previous selection on this representation.
   */
  void Select(svtkView* view, svtkSelection* selection) { this->Select(view, selection, false); }
  void Select(svtkView* view, svtkSelection* selection, bool extend);

  /**
   * Analogous to Select(). The view calls this method when it needs to
   * change the underlying annotations (some views might perform the
   * creation of annotations). The representation takes the annotations
   * and converts them into a selection on its data by calling ConvertAnnotations,
   * then calls UpdateAnnotations with the converted selection.
   * Subclasses should not override this method, but should instead
   * override ConvertSelection.
   * The optional third argument specifies whether the selection should be
   * added to the previous selection on this representation.
   */
  void Annotate(svtkView* view, svtkAnnotationLayers* annotations)
  {
    this->Annotate(view, annotations, false);
  }
  void Annotate(svtkView* view, svtkAnnotationLayers* annotations, bool extend);

  //@{
  /**
   * Whether this representation is able to handle a selection.
   * Default is true.
   */
  svtkSetMacro(Selectable, bool);
  svtkGetMacro(Selectable, bool);
  svtkBooleanMacro(Selectable, bool);
  //@}

  /**
   * Updates the selection in the selection link and fires a selection
   * change event. Subclasses should not override this method,
   * but should instead override ConvertSelection.
   * The optional second argument specifies whether the selection should be
   * added to the previous selection on this representation.
   */
  void UpdateSelection(svtkSelection* selection) { this->UpdateSelection(selection, false); }
  void UpdateSelection(svtkSelection* selection, bool extend);

  /**
   * Updates the selection in the selection link and fires a selection
   * change event. Subclasses should not override this method,
   * but should instead override ConvertSelection.
   * The optional second argument specifies whether the selection should be
   * added to the previous selection on this representation.
   */
  void UpdateAnnotations(svtkAnnotationLayers* annotations)
  {
    this->UpdateAnnotations(annotations, false);
  }
  void UpdateAnnotations(svtkAnnotationLayers* annotations, bool extend);

  /**
   * The output port that contains the annotations whose selections are
   * localized for a particular input data object.
   * This should be used when connecting the internal pipelines.
   */
  virtual svtkAlgorithmOutput* GetInternalAnnotationOutputPort()
  {
    return this->GetInternalAnnotationOutputPort(0);
  }
  virtual svtkAlgorithmOutput* GetInternalAnnotationOutputPort(int port)
  {
    return this->GetInternalAnnotationOutputPort(port, 0);
  }
  virtual svtkAlgorithmOutput* GetInternalAnnotationOutputPort(int port, int conn);

  /**
   * The output port that contains the selection associated with the
   * current annotation (normally the interactive selection).
   * This should be used when connecting the internal pipelines.
   */
  virtual svtkAlgorithmOutput* GetInternalSelectionOutputPort()
  {
    return this->GetInternalSelectionOutputPort(0);
  }
  virtual svtkAlgorithmOutput* GetInternalSelectionOutputPort(int port)
  {
    return this->GetInternalSelectionOutputPort(port, 0);
  }
  virtual svtkAlgorithmOutput* GetInternalSelectionOutputPort(int port, int conn);

  /**
   * Retrieves an output port for the input data object at the specified port
   * and connection index. This may be connected to the representation's
   * internal pipeline.
   */
  virtual svtkAlgorithmOutput* GetInternalOutputPort() { return this->GetInternalOutputPort(0); }
  virtual svtkAlgorithmOutput* GetInternalOutputPort(int port)
  {
    return this->GetInternalOutputPort(port, 0);
  }
  virtual svtkAlgorithmOutput* GetInternalOutputPort(int port, int conn);

  //@{
  /**
   * Set the selection type produced by this view.
   * This should be one of the content type constants defined in
   * svtkSelectionNode.h. Common values are
   * svtkSelectionNode::INDICES
   * svtkSelectionNode::PEDIGREEIDS
   * svtkSelectionNode::VALUES
   */
  svtkSetMacro(SelectionType, int);
  svtkGetMacro(SelectionType, int);
  //@}

  //@{
  /**
   * If a VALUES selection, the arrays used to produce a selection.
   */
  virtual void SetSelectionArrayNames(svtkStringArray* names);
  svtkGetObjectMacro(SelectionArrayNames, svtkStringArray);
  //@}

  //@{
  /**
   * If a VALUES selection, the array used to produce a selection.
   */
  virtual void SetSelectionArrayName(const char* name);
  virtual const char* GetSelectionArrayName();
  //@}

  /**
   * Convert the selection to a type appropriate for sharing with other
   * representations through svtkAnnotationLink, possibly using the view.
   * For the superclass, we just return the same selection.
   * Subclasses may do something more fancy, like convert the selection
   * from a frustrum to a list of pedigree ids.  If the selection cannot
   * be applied to this representation, return nullptr.
   */
  virtual svtkSelection* ConvertSelection(svtkView* view, svtkSelection* selection);

protected:
  svtkDataRepresentation();
  ~svtkDataRepresentation() override;

  /**
   * Subclasses should override this to connect inputs to the internal pipeline
   * as necessary. Since most representations are "meta-filters" (i.e. filters
   * containing other filters), you should create shallow copies of your input
   * before connecting to the internal pipeline. The convenience method
   * GetInternalOutputPort will create a cached shallow copy of a specified
   * input for you. The related helper functions GetInternalAnnotationOutputPort,
   * GetInternalSelectionOutputPort should be used to obtain a selection or
   * annotation port whose selections are localized for a particular input data object.
   */
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override
  {
    return 1;
  }

  /**
   * Clear the input shallow copy caches if the algorithm is in "release data" mode.
   */
  virtual void ProcessEvents(svtkObject* caller, unsigned long eventId, void* callData);

  //@{
  /**
   * The annotation link for this representation.
   */
  virtual void SetAnnotationLinkInternal(svtkAnnotationLink* link);
  svtkAnnotationLink* AnnotationLinkInternal;
  //@}

  // Whether its representation can handle a selection.
  bool Selectable;

  /**
   * The selection type created by the view.
   */
  int SelectionType;

  /**
   * If a VALUES selection, the array names used in the selection.
   */
  svtkStringArray* SelectionArrayNames;

  friend class svtkView;
  friend class svtkRenderView;
  class Command;
  friend class Command;
  Command* Observer;

  // ------------------------------------------------------------------------
  // Methods to override in subclasses
  // ------------------------------------------------------------------------

  /**
   * Adds the representation to the view.  This is called from
   * svtkView::AddRepresentation().  Subclasses should override this method.
   * Returns true if the addition succeeds.
   */
  virtual bool AddToView(svtkView* svtkNotUsed(view)) { return true; }

  /**
   * Removes the representation to the view.  This is called from
   * svtkView::RemoveRepresentation().  Subclasses should override this method.
   * Returns true if the removal succeeds.
   */
  virtual bool RemoveFromView(svtkView* svtkNotUsed(view)) { return true; }

  /**
   * Analogous to ConvertSelection(), allows subclasses to manipulate annotations
   * before passing them off to svtkAnnotationLink.  If the annotations cannot
   * be applied to this representation, return nullptr.
   */
  virtual svtkAnnotationLayers* ConvertAnnotations(svtkView* view, svtkAnnotationLayers* annotations);

  svtkTrivialProducer* GetInternalInput(int port, int conn);
  void SetInternalInput(int port, int conn, svtkTrivialProducer* producer);

private:
  svtkDataRepresentation(const svtkDataRepresentation&) = delete;
  void operator=(const svtkDataRepresentation&) = delete;

  class Internals;
  Internals* Implementation;
};

#endif
