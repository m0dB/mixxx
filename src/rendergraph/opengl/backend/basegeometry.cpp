#include "backend/basegeometry.h"

#include "backend/baseattributeset.h"
#include "rendergraph/geometry.h"

using namespace rendergraph;

namespace {
// to mimic sg default
constexpr int defaultDrawingMode = static_cast<int>(Geometry::DrawingMode::TriangleStrip);
} // namespace

BaseGeometry::BaseGeometry(
        const BaseAttributeSet& attributeSet, int vertexCount)
        : m_pAttributes(attributeSet.attributes().data()),
          m_attributeCount(attributeSet.attributes().size()),
          m_sizeOfVertex(attributeSet.sizeOfVertex()),
          m_drawingMode(defaultDrawingMode),
          m_vertexCount(vertexCount),
          m_vertexData(m_vertexCount * m_sizeOfVertex / sizeof(float)) {
}
