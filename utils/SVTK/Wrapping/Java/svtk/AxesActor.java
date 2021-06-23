package svtk;

/**
 * Axis actor created in the Java world
 *
 * @author Kitware
 */
public class AxesActor extends svtkAssembly {

  private svtkRenderer ren;
  private double axisLength = 0.8;
  private svtkTextActor xactor, yactor, zactor;

  public AxesActor(svtkRenderer _ren) {
    super();
    ren = _ren;
    createAxes();
  }

  public void createAxes() {
    svtkAxes axes = new svtkAxes();
    axes.SetOrigin(0, 0, 0);
    axes.SetScaleFactor(axisLength);

    xactor = new svtkTextActor();
    yactor = new svtkTextActor();
    zactor = new svtkTextActor();

    xactor.SetInput("X");
    yactor.SetInput("Y");
    zactor.SetInput("Z");

    xactor.GetPositionCoordinate().SetCoordinateSystemToWorld();
    yactor.GetPositionCoordinate().SetCoordinateSystemToWorld();
    zactor.GetPositionCoordinate().SetCoordinateSystemToWorld();

    xactor.GetPositionCoordinate().SetValue(axisLength, 0.0, 0.0);
    yactor.GetPositionCoordinate().SetValue(0.0, axisLength, 0.0);
    zactor.GetPositionCoordinate().SetValue(0.0, 0.0, axisLength);

    xactor.GetTextProperty().SetColor(1.0, 1.0, 1.0);
    xactor.GetTextProperty().ShadowOn();
    xactor.GetTextProperty().ItalicOn();
    xactor.GetTextProperty().BoldOff();

    yactor.GetTextProperty().SetColor(1.0, 1.0, 1.0);
    yactor.GetTextProperty().ShadowOn();
    yactor.GetTextProperty().ItalicOn();
    yactor.GetTextProperty().BoldOff();

    zactor.GetTextProperty().SetColor(1.0, 1.0, 1.0);
    zactor.GetTextProperty().ShadowOn();
    zactor.GetTextProperty().ItalicOn();
    zactor.GetTextProperty().BoldOff();

    xactor.SetMaximumLineHeight(0.25);
    yactor.SetMaximumLineHeight(0.25);
    zactor.SetMaximumLineHeight(0.25);

    svtkTubeFilter tube = new svtkTubeFilter();
    tube.SetInputConnection(axes.GetOutputPort());
    tube.SetRadius(0.05);
    tube.SetNumberOfSides(8);

    svtkPolyDataMapper tubeMapper = new svtkPolyDataMapper();
    tubeMapper.SetInputConnection(tube.GetOutputPort());

    svtkActor tubeActor = new svtkActor();
    tubeActor.SetMapper(tubeMapper);
    tubeActor.PickableOff();

    int coneRes = 12;
    double coneScale = 0.3;

    // --- x-Cone
    svtkConeSource xcone = new svtkConeSource();
    xcone.SetResolution(coneRes);
    svtkPolyDataMapper xconeMapper = new svtkPolyDataMapper();
    xconeMapper.SetInputConnection(xcone.GetOutputPort());
    svtkActor xconeActor = new svtkActor();
    xconeActor.SetMapper(xconeMapper);
    xconeActor.GetProperty().SetColor(1, 0, 0);
    xconeActor.SetScale(coneScale, coneScale, coneScale);
    xconeActor.SetPosition(axisLength, 0.0, 0.0);

    // --- y-Cone
    svtkConeSource ycone = new svtkConeSource();
    ycone.SetResolution(coneRes);
    svtkPolyDataMapper yconeMapper = new svtkPolyDataMapper();
    yconeMapper.SetInputConnection(ycone.GetOutputPort());
    svtkActor yconeActor = new svtkActor();
    yconeActor.SetMapper(yconeMapper);
    yconeActor.GetProperty().SetColor(1, 1, 0);
    yconeActor.RotateZ(90);
    yconeActor.SetScale(coneScale, coneScale, coneScale);
    yconeActor.SetPosition(0.0, axisLength, 0.0);

    // --- z-Cone
    svtkConeSource zcone = new svtkConeSource();
    zcone.SetResolution(coneRes);
    svtkPolyDataMapper zconeMapper = new svtkPolyDataMapper();
    zconeMapper.SetInputConnection(zcone.GetOutputPort());
    svtkActor zconeActor = new svtkActor();
    zconeActor.SetMapper(zconeMapper);
    zconeActor.GetProperty().SetColor(0, 1, 0);
    zconeActor.RotateY(-90);
    zconeActor.SetScale(coneScale, coneScale, coneScale);
    zconeActor.SetPosition(0.0, 0.0, axisLength);

    ren.AddActor2D(xactor);
    ren.AddActor2D(yactor);
    ren.AddActor2D(zactor);

    this.AddPart(tubeActor);
    this.AddPart(xconeActor);
    this.AddPart(yconeActor);
    this.AddPart(zconeActor);

    ren.AddActor(this);
  }

  public void setAxesVisibility(boolean ison) {
    this.SetVisibility(ison ? 1 : 0);
    xactor.SetVisibility(ison ? 1 : 0);
    yactor.SetVisibility(ison ? 1 : 0);
    zactor.SetVisibility(ison ? 1 : 0);
  }
}
