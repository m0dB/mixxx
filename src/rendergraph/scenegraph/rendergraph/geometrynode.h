#pragma once

#include <QSGGeometryNode>

#include "rendergraph/geometry.h"
#include "rendergraph/nodebase.h"

namespace rendergraph {
class GeometryNode;
class Geometry;
class Material;
} // namespace rendergraph

class rendergraph::GeometryNode : public QSGGeometryNode, public rendergraph::NodeBase {
  public:
    GeometryNode();
    ~GeometryNode();

    template<class T_Material>
    void initForRectangles(int numRectangles) {
        setGeometry(std::make_unique<Geometry>(T_Material::attributes(), numRectangles * 6));
        setMaterial(std::make_unique<T_Material>());
        geometry().setDrawingMode(Geometry::DrawingMode::Triangles);
    }

    void setMaterial(std::unique_ptr<Material> material);
    void setGeometry(std::unique_ptr<Geometry> geometry);

    Geometry& geometry() const;
    Material& material() const;

  private:
    std::unique_ptr<Material> m_material;
    std::unique_ptr<Geometry> m_geometry;
};
