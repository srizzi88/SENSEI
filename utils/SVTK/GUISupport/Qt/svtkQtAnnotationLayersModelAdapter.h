/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtAnnotationLayersModelAdapter.h

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
 * @class   svtkQtAnnotationLayersModelAdapter
 * @brief   Adapts annotations to a Qt item model.
 *
 *
 * svtkQtAnnotationLayersModelAdapter is a QAbstractItemModel with a
 *    svtkAnnotationLayers as its underlying data model.
 *
 * @sa
 * svtkQtAbstractModelAdapter svtkQtTableModelAdapter
 */

#ifndef svtkQtAnnotationLayersModelAdapter_h
#define svtkQtAnnotationLayersModelAdapter_h

#include "svtkConfigure.h"
#include "svtkGUISupportQtModule.h" // For export macro
#include "svtkQtAbstractModelAdapter.h"

class svtkAnnotationLayers;
class svtkSelection;

class SVTKGUISUPPORTQT_EXPORT svtkQtAnnotationLayersModelAdapter : public svtkQtAbstractModelAdapter
{
  Q_OBJECT

public:
  svtkQtAnnotationLayersModelAdapter(QObject* parent = nullptr);
  svtkQtAnnotationLayersModelAdapter(svtkAnnotationLayers* ann, QObject* parent = nullptr);
  ~svtkQtAnnotationLayersModelAdapter() override;

  //@{
  /**
   * Set/Get the SVTK data object as input to this adapter
   */
  void SetSVTKDataObject(svtkDataObject* data) override;
  svtkDataObject* GetSVTKDataObject() const override;
  //@}

  //@{
  /**
   * Selection conversion from SVTK land to Qt land
   */
  virtual svtkAnnotationLayers* QModelIndexListToSVTKAnnotationLayers(
    const QModelIndexList qmil) const;
  virtual QItemSelection SVTKAnnotationLayersToQItemSelection(svtkAnnotationLayers* svtkann) const;
  svtkSelection* QModelIndexListToSVTKIndexSelection(const QModelIndexList qmil) const override;
  QItemSelection SVTKIndexSelectionToQItemSelection(svtkSelection* svtksel) const override;
  //@}

  void SetKeyColumnName(const char* name) override;
  void SetColorColumnName(const char* name) override;

  //@{
  /**
   * Set up the model based on the current table.
   */
  void setAnnotationLayers(svtkAnnotationLayers* annotations);
  svtkAnnotationLayers* annotationLayers() const { return this->Annotations; }
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;
  QVariant headerData(
    int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex& index) const override;
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  /*
    Qt::DropActions supportedDropActions() const;
    Qt::DropActions supportedDragActions() const;
    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());
    bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex());
    virtual bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column,
    const QModelIndex & parent) ; virtual QMimeData * mimeData ( const QModelIndexList & indexes )
    const; virtual QStringList mimeTypes () const ;
  */
private:
  //@}

  bool noAnnotationsCheck() const;

  svtkAnnotationLayers* Annotations;

  svtkQtAnnotationLayersModelAdapter(const svtkQtAnnotationLayersModelAdapter&) = delete;
  void operator=(const svtkQtAnnotationLayersModelAdapter&) = delete;
};

#endif
// SVTK-HeaderTest-Exclude: svtkQtAnnotationLayersModelAdapter.h
