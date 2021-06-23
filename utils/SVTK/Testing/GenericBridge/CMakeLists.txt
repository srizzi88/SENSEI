set(classes
  svtkBridgeAttribute
  svtkBridgeCell
  svtkBridgeCellIterator
  svtkBridgeCellIteratorOnCellBoundaries
  svtkBridgeCellIteratorOnCellList
  svtkBridgeCellIteratorOnDataSet
  svtkBridgeCellIteratorOne
  svtkBridgeCellIteratorStrategy
  svtkBridgeDataSet
  svtkBridgePointIterator
  svtkBridgePointIteratorOnCell
  svtkBridgePointIteratorOnDataSet
  svtkBridgePointIteratorOne)

set(headers
  svtkBridgeExport.h)

svtk_module_add_module(SVTK::TestingGenericBridge
  CLASSES ${classes}
  HEADERS ${headers})
