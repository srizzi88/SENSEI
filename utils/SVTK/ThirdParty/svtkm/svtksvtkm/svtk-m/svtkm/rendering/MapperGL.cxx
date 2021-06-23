//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/MapperGL.h>

#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/TryExecute.h>
#include <svtkm/rendering/internal/OpenGLHeaders.h>
#include <svtkm/rendering/internal/RunTriangulator.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/exec/ColorTable.h>

#include <vector>

namespace svtkm
{
namespace rendering
{

namespace
{

using Id4Type = TypeListId4;

class MapColorAndVertices : public svtkm::worklet::WorkletMapField
{
public:
  const svtkm::exec::ColorTableBase* ColorTable;
  const svtkm::Float32 SMin, SDiff;

  SVTKM_CONT
  MapColorAndVertices(const svtkm::exec::ColorTableBase* table,
                      svtkm::Float32 sMin,
                      svtkm::Float32 sDiff)
    : ColorTable(table)
    , SMin(sMin)
    , SDiff(sDiff)
  {
  }
  using ControlSignature = void(FieldIn vertexId,
                                WholeArrayIn indices,
                                WholeArrayIn scalar,
                                WholeArrayIn verts,
                                WholeArrayOut out_color,
                                WholeArrayOut out_vertices);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);

  template <typename InputArrayIndexPortalType,
            typename InputArrayPortalType,
            typename InputArrayV3PortalType,
            typename OutputArrayPortalType>
  SVTKM_EXEC void operator()(const svtkm::Id& i,
                            InputArrayIndexPortalType& indices,
                            const InputArrayPortalType& scalar,
                            const InputArrayV3PortalType& verts,
                            OutputArrayPortalType& c_array,
                            OutputArrayPortalType& v_array) const
  {
    svtkm::Id4 idx = indices.Get(i);
    svtkm::Id i1 = idx[1];
    svtkm::Id i2 = idx[2];
    svtkm::Id i3 = idx[3];

    svtkm::Vec3f_32 p1 = verts.Get(idx[1]);
    svtkm::Vec3f_32 p2 = verts.Get(idx[2]);
    svtkm::Vec3f_32 p3 = verts.Get(idx[3]);

    svtkm::Float32 s;
    svtkm::Vec<float, 3> color1;
    svtkm::Vec<float, 3> color2;
    svtkm::Vec<float, 3> color3;

    if (SDiff == 0)
    {
      s = 0;
      color1 = ColorTable->MapThroughColorSpace(s);
      color2 = ColorTable->MapThroughColorSpace(s);
      color3 = ColorTable->MapThroughColorSpace(s);
    }
    else
    {
      s = scalar.Get(i1);
      s = (s - SMin) / SDiff;
      color1 = ColorTable->MapThroughColorSpace(s);

      s = scalar.Get(i2);
      s = (s - SMin) / SDiff;
      color2 = ColorTable->MapThroughColorSpace(s);

      s = scalar.Get(i3);
      s = (s - SMin) / SDiff;
      color3 = ColorTable->MapThroughColorSpace(s);
    }

    const svtkm::Id offset = 9;

    v_array.Set(i * offset, p1[0]);
    v_array.Set(i * offset + 1, p1[1]);
    v_array.Set(i * offset + 2, p1[2]);
    c_array.Set(i * offset, color1[0]);
    c_array.Set(i * offset + 1, color1[1]);
    c_array.Set(i * offset + 2, color1[2]);

    v_array.Set(i * offset + 3, p2[0]);
    v_array.Set(i * offset + 4, p2[1]);
    v_array.Set(i * offset + 5, p2[2]);
    c_array.Set(i * offset + 3, color2[0]);
    c_array.Set(i * offset + 4, color2[1]);
    c_array.Set(i * offset + 5, color2[2]);

    v_array.Set(i * offset + 6, p3[0]);
    v_array.Set(i * offset + 7, p3[1]);
    v_array.Set(i * offset + 8, p3[2]);
    c_array.Set(i * offset + 6, color3[0]);
    c_array.Set(i * offset + 7, color3[1]);
    c_array.Set(i * offset + 8, color3[2]);
  }
};

template <typename PtType>
struct MapColorAndVerticesInvokeFunctor
{
  svtkm::cont::ArrayHandle<svtkm::Id4> TriangleIndices;
  svtkm::cont::ColorTable ColorTable;
  const svtkm::cont::ArrayHandle<svtkm::Float32> Scalar;
  const svtkm::Range ScalarRange;
  const PtType Vertices;
  svtkm::Float32 SMin;
  svtkm::Float32 SDiff;
  svtkm::cont::ArrayHandle<svtkm::Float32> OutColor;
  svtkm::cont::ArrayHandle<svtkm::Float32> OutVertices;

  SVTKM_CONT
  MapColorAndVerticesInvokeFunctor(const svtkm::cont::ArrayHandle<svtkm::Id4>& indices,
                                   const svtkm::cont::ColorTable& colorTable,
                                   const svtkm::cont::ArrayHandle<Float32>& scalar,
                                   const svtkm::Range& scalarRange,
                                   const PtType& vertices,
                                   svtkm::Float32 s_min,
                                   svtkm::Float32 s_max,
                                   svtkm::cont::ArrayHandle<Float32>& out_color,
                                   svtkm::cont::ArrayHandle<Float32>& out_vertices)
    : TriangleIndices(indices)
    , ColorTable(colorTable)
    , Scalar(scalar)
    , ScalarRange(scalarRange)
    , Vertices(vertices)
    , SMin(s_min)
    , SDiff(s_max - s_min)
    , OutColor(out_color)
    , OutVertices(out_vertices)
  {
  }

  template <typename Device>
  SVTKM_CONT bool operator()(Device device) const
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);


    MapColorAndVertices worklet(
      this->ColorTable.PrepareForExecution(device), this->SMin, this->SDiff);
    svtkm::worklet::DispatcherMapField<MapColorAndVertices, Device> dispatcher(worklet);

    svtkm::cont::ArrayHandleIndex indexArray(this->TriangleIndices.GetNumberOfValues());
    dispatcher.Invoke(indexArray,
                      this->TriangleIndices,
                      this->Scalar,
                      this->Vertices,
                      this->OutColor,
                      this->OutVertices);
    return true;
  }
};

template <typename PtType>
SVTKM_CONT void RenderStructuredLineSegments(svtkm::Id numVerts,
                                            const PtType& verts,
                                            const svtkm::cont::ArrayHandle<svtkm::Float32>& scalar,
                                            svtkm::cont::ColorTable ct,
                                            bool logY)
{
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glLineWidth(1);
  svtkm::UInt8 r, g, b, a;

  //This is horrible as the color table API isn't designed for users to query
  //on a single value basis. We use the GetPoint and GetPointAlpha escape hatches
  //and manually convert from float to uchar
  svtkm::Vec<double, 4> data;
  ct.GetPoint(0, data);
  r = static_cast<svtkm::UInt8>(data[1] * 255.0 + 0.5);
  g = static_cast<svtkm::UInt8>(data[2] * 255.0 + 0.5);
  b = static_cast<svtkm::UInt8>(data[3] * 255.0 + 0.5);
  ct.GetPointAlpha(0, data);
  a = static_cast<svtkm::UInt8>(data[1] * 255.0 + 0.5);

  glColor4ub(r, g, b, a);
  glBegin(GL_LINE_STRIP);
  for (int i = 0; i < numVerts; i++)
  {
    svtkm::Vec3f_32 pt = verts.GetPortalConstControl().Get(i);
    svtkm::Float32 s = scalar.GetPortalConstControl().Get(i);
    if (logY)
      s = svtkm::Float32(log10(s));
    glVertex3f(pt[0], s, 0.0f);
  }
  glEnd();
}

template <typename PtType>
SVTKM_CONT void RenderExplicitLineSegments(svtkm::Id numVerts,
                                          const PtType& verts,
                                          const svtkm::cont::ArrayHandle<svtkm::Float32>& scalar,
                                          svtkm::cont::ColorTable ct,
                                          bool logY)
{
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glLineWidth(1);
  svtkm::UInt8 r, g, b, a;
  //This is horrible as the color table API isn't designed for users to query
  //on a single value basis. We use the GetPoint and GetPointAlpha escape hatches
  //and manually convert from float to uchar
  svtkm::Vec<double, 4> data;
  ct.GetPoint(0, data);
  r = static_cast<svtkm::UInt8>(data[1] * 255.0 + 0.5);
  g = static_cast<svtkm::UInt8>(data[2] * 255.0 + 0.5);
  b = static_cast<svtkm::UInt8>(data[3] * 255.0 + 0.5);
  ct.GetPointAlpha(0, data);
  a = static_cast<svtkm::UInt8>(data[1] * 255.0 + 0.5);

  glColor4ub(r, g, b, a);
  glBegin(GL_LINE_STRIP);
  for (int i = 0; i < numVerts; i++)
  {
    svtkm::Vec3f_32 pt = verts.GetPortalConstControl().Get(i);
    svtkm::Float32 s = scalar.GetPortalConstControl().Get(i);
    if (logY)
      s = svtkm::Float32(log10(s));
    glVertex3f(pt[0], s, 0.0f);
  }
  glEnd();
}

template <typename PtType>
SVTKM_CONT void RenderTriangles(MapperGL& mapper,
                               svtkm::Id numTri,
                               const PtType& verts,
                               const svtkm::cont::ArrayHandle<svtkm::Id4>& indices,
                               const svtkm::cont::ArrayHandle<svtkm::Float32>& scalar,
                               const svtkm::cont::ColorTable& ct,
                               const svtkm::Range& scalarRange,
                               const svtkm::rendering::Camera& camera)
{
  if (!mapper.loaded)
  {
    // The glewExperimental global switch can be  turned on by setting it to
    // GL_TRUE before calling glewInit(), which  ensures that all extensions
    // with valid entry points will be exposed. This is needed as the glut
    // context that is being created is not a valid 'core' context but
    // instead a 'compatibility' context
    //
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();
    if (GlewInitResult)
      std::cerr << "ERROR: " << glewGetErrorString(GlewInitResult) << std::endl;
    mapper.loaded = true;

    svtkm::Float32 sMin = svtkm::Float32(scalarRange.Min);
    svtkm::Float32 sMax = svtkm::Float32(scalarRange.Max);
    svtkm::cont::ArrayHandle<svtkm::Float32> out_vertices, out_color;
    out_vertices.Allocate(9 * indices.GetNumberOfValues());
    out_color.Allocate(9 * indices.GetNumberOfValues());

    svtkm::cont::TryExecute(MapColorAndVerticesInvokeFunctor<PtType>(
      indices, ct, scalar, scalarRange, verts, sMin, sMax, out_color, out_vertices));


    svtkm::Id vtx_cnt = out_vertices.GetNumberOfValues();
    svtkm::Float32* v_ptr = out_vertices.GetStorage().GetArray();
    svtkm::Float32* c_ptr = out_color.GetStorage().GetArray();

    svtkm::Id floatSz = static_cast<svtkm::Id>(sizeof(svtkm::Float32));
    GLsizeiptr sz = static_cast<GLsizeiptr>(vtx_cnt * floatSz);

    GLuint points_vbo = 0;
    glGenBuffers(1, &points_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
    glBufferData(GL_ARRAY_BUFFER, sz, v_ptr, GL_STATIC_DRAW);

    GLuint colours_vbo = 0;
    glGenBuffers(1, &colours_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, colours_vbo);
    glBufferData(GL_ARRAY_BUFFER, sz, c_ptr, GL_STATIC_DRAW);

    mapper.vao = 0;
    glGenVertexArrays(1, &mapper.vao);
    glBindVertexArray(mapper.vao);
    glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, colours_vbo);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    const char* vertex_shader = "#version 120\n"
                                "attribute vec3 vertex_position;"
                                "attribute vec3 vertex_color;"
                                "varying vec3 ourColor;"
                                "uniform mat4 mv_matrix;"
                                "uniform mat4 p_matrix;"

                                "void main() {"
                                "  gl_Position = p_matrix*mv_matrix * vec4(vertex_position, 1.0);"
                                "  ourColor = vertex_color;"
                                "}";
    const char* fragment_shader = "#version 120\n"
                                  "varying vec3 ourColor;"
                                  "void main() {"
                                  "  gl_FragColor = vec4 (ourColor, 1.0);"
                                  "}";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, nullptr);
    glCompileShader(vs);
    GLint isCompiled = 0;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE)
    {
      GLint maxLength = 0;
      glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &maxLength);

      std::string msg;
      if (maxLength <= 0)
        msg = "No error message";
      else
      {
        // The maxLength includes the nullptr character
        GLchar* strInfoLog = new GLchar[maxLength + 1];
        glGetShaderInfoLog(vs, maxLength, &maxLength, strInfoLog);
        msg = std::string(strInfoLog);
        delete[] strInfoLog;
      }
      throw svtkm::cont::ErrorBadValue("Shader compile error:" + msg);
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE)
    {
      GLint maxLength = 0;
      glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &maxLength);

      std::string msg;
      if (maxLength <= 0)
        msg = "No error message";
      else
      {
        // The maxLength includes the nullptr character
        GLchar* strInfoLog = new GLchar[maxLength + 1];
        glGetShaderInfoLog(vs, maxLength, &maxLength, strInfoLog);
        msg = std::string(strInfoLog);
        delete[] strInfoLog;
      }
      throw svtkm::cont::ErrorBadValue("Shader compile error:" + msg);
    }

    mapper.shader_programme = glCreateProgram();
    if (mapper.shader_programme > 0)
    {
      glAttachShader(mapper.shader_programme, fs);
      glAttachShader(mapper.shader_programme, vs);
      glBindAttribLocation(mapper.shader_programme, 0, "vertex_position");
      glBindAttribLocation(mapper.shader_programme, 1, "vertex_color");

      glLinkProgram(mapper.shader_programme);
      GLint linkStatus;
      glGetProgramiv(mapper.shader_programme, GL_LINK_STATUS, &linkStatus);
      if (!linkStatus)
      {
        char log[2048];
        GLsizei len;
        glGetProgramInfoLog(mapper.shader_programme, 2048, &len, log);
        std::string msg = std::string("Shader program link failed: ") + std::string(log);
        throw svtkm::cont::ErrorBadValue(msg);
      }
    }
  }

  if (mapper.shader_programme > 0)
  {
    svtkm::Id width = mapper.GetCanvas()->GetWidth();
    svtkm::Id height = mapper.GetCanvas()->GetWidth();
    svtkm::Matrix<svtkm::Float32, 4, 4> viewM = camera.CreateViewMatrix();
    svtkm::Matrix<svtkm::Float32, 4, 4> projM = camera.CreateProjectionMatrix(width, height);

    MatrixHelpers::CreateOGLMatrix(viewM, mapper.mvMat);
    MatrixHelpers::CreateOGLMatrix(projM, mapper.pMat);

    glUseProgram(mapper.shader_programme);
    GLint mvID = glGetUniformLocation(mapper.shader_programme, "mv_matrix");
    glUniformMatrix4fv(mvID, 1, GL_FALSE, mapper.mvMat);
    GLint pID = glGetUniformLocation(mapper.shader_programme, "p_matrix");
    glUniformMatrix4fv(pID, 1, GL_FALSE, mapper.pMat);
    glBindVertexArray(mapper.vao);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(numTri * 3));
    glUseProgram(0);
  }
}

} // anonymous namespace

MapperGL::MapperGL()
  : Canvas(nullptr)
  , loaded(false)
{
}

MapperGL::~MapperGL()
{
}

void MapperGL::RenderCells(const svtkm::cont::DynamicCellSet& cellset,
                           const svtkm::cont::CoordinateSystem& coords,
                           const svtkm::cont::Field& scalarField,
                           const svtkm::cont::ColorTable& colorTable,
                           const svtkm::rendering::Camera& camera,
                           const svtkm::Range& scalarRange)
{
  svtkm::cont::ArrayHandle<svtkm::Float32> sf;
  sf = scalarField.GetData().Cast<svtkm::cont::ArrayHandle<svtkm::Float32>>();
  auto dcoords = coords.GetData();
  svtkm::Id numVerts = coords.GetNumberOfPoints();

  //Handle 1D cases.
  if (cellset.IsSameType(svtkm::cont::CellSetStructured<1>()))
  {
    svtkm::cont::ArrayHandleUniformPointCoordinates verts;
    verts = dcoords.Cast<svtkm::cont::ArrayHandleUniformPointCoordinates>();
    RenderStructuredLineSegments(numVerts, verts, sf, colorTable, this->LogarithmY);
  }
  else if (cellset.IsSameType(svtkm::cont::CellSetSingleType<>()) &&
           cellset.Cast<svtkm::cont::CellSetSingleType<>>().GetCellShapeAsId() ==
             svtkm::CELL_SHAPE_LINE)
  {
    svtkm::cont::ArrayHandle<svtkm::Vec3f_32> verts;
    verts = dcoords.Cast<svtkm::cont::ArrayHandle<svtkm::Vec3f_32>>();
    RenderExplicitLineSegments(numVerts, verts, sf, colorTable, this->LogarithmY);
  }
  else
  {
    svtkm::cont::ArrayHandle<svtkm::Id4> indices;
    svtkm::Id numTri;
    svtkm::rendering::internal::RunTriangulator(cellset, indices, numTri);

    svtkm::cont::ArrayHandleUniformPointCoordinates uVerts;
    svtkm::cont::ArrayHandle<svtkm::Vec3f_32> eVerts;

    if (dcoords.IsType<svtkm::cont::ArrayHandleUniformPointCoordinates>())
    {
      uVerts = dcoords.Cast<svtkm::cont::ArrayHandleUniformPointCoordinates>();
      RenderTriangles(*this, numTri, uVerts, indices, sf, colorTable, scalarRange, camera);
    }
    else if (dcoords.IsType<svtkm::cont::ArrayHandle<svtkm::Vec3f_32>>())
    {
      eVerts = dcoords.Cast<svtkm::cont::ArrayHandle<svtkm::Vec3f_32>>();
      RenderTriangles(*this, numTri, eVerts, indices, sf, colorTable, scalarRange, camera);
    }
    else if (dcoords.IsType<svtkm::cont::ArrayHandleCartesianProduct<
               svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
               svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
               svtkm::cont::ArrayHandle<svtkm::FloatDefault>>>())
    {
      svtkm::cont::ArrayHandleCartesianProduct<svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
                                              svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
                                              svtkm::cont::ArrayHandle<svtkm::FloatDefault>>
        rVerts;
      rVerts = dcoords.Cast<
        svtkm::cont::ArrayHandleCartesianProduct<svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
                                                svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
                                                svtkm::cont::ArrayHandle<svtkm::FloatDefault>>>();
      RenderTriangles(*this, numTri, rVerts, indices, sf, colorTable, scalarRange, camera);
    }
  }
  glFinish();
  glFlush();
}

void MapperGL::StartScene()
{
  // Nothing needs to be done.
}

void MapperGL::EndScene()
{
  // Nothing needs to be done.
}

void MapperGL::SetCanvas(svtkm::rendering::Canvas* c)
{
  if (c != nullptr)
  {
    this->Canvas = dynamic_cast<svtkm::rendering::CanvasGL*>(c);
    if (this->Canvas == nullptr)
      throw svtkm::cont::ErrorBadValue("Bad canvas type for MapperGL. Must be CanvasGL");
  }
}

svtkm::rendering::Canvas* MapperGL::GetCanvas() const
{
  return this->Canvas;
}

svtkm::rendering::Mapper* MapperGL::NewCopy() const
{
  return new svtkm::rendering::MapperGL(*this);
}
}
} // namespace svtkm::rendering
