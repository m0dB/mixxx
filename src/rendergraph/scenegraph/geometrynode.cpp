#include "rendergraph/geometrynode.h"

#include <QSGNode>

using namespace rendergraph;

GeometryNode::GeometryNode()
        : TreeNode(this) {
}

void GeometryNode::setGeometry(std::unique_ptr<Geometry> pGeometry) {
    m_pGeometry = std::move(pGeometry);
    QSGGeometryNode::setGeometry(m_pGeometry.get());
}

void GeometryNode::setMaterial(std::unique_ptr<Material> pMaterial) {
    m_pMaterial = std::move(pMaterial);
    QSGGeometryNode::setMaterial(m_pMaterial.get());
}

Geometry& GeometryNode::geometry() const {
    return *m_pGeometry;
}

Material& GeometryNode::material() const {
    return *m_pMaterial;
}

void GeometryNode::markDirtyMaterial() {
    markDirty(QSGNode::DirtyMaterial);
}

void GeometryNode::markDirtyGeometry() {
    markDirty(QSGNode::DirtyGeometry);
}
