# svtkPlotPoints::GetNearestPoint API Change

svtkPlotPoints::GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tol, svtkVector2f* location)
has changed and has now a new optional argument segmentId.
The new signature is as follows :
svtkPlotPoints::GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tol, svtkVector2f* location, svtkIdType* segmentId))

With SVTK_LEGACY_REMOVE=OFF, calls and override of the old api still works for the time being.
