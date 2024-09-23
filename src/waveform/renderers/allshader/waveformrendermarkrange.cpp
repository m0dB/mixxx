#include "waveform/renderers/allshader/waveformrendermarkrange.h"

#include "rendergraph/geometry.h"
#include "rendergraph/geometrynode.h"
#include "rendergraph/material/unicolormaterial.h"
#include "rendergraph/vertexupdaters/vertexupdater.h"
#include "skin/legacy/skincontext.h"
#include "waveform/renderers/allshader/matrixforwidgetgeometry.h"
#include "waveform/renderers/waveformwidgetrenderer.h"

using namespace rendergraph;

namespace allshader {

WaveformRenderMarkRange::WaveformRenderMarkRange(WaveformWidgetRenderer* waveformWidget)
        : ::WaveformRendererAbstract(waveformWidget) {
}

void WaveformRenderMarkRange::setup(const QDomNode& node, const SkinContext& context) {
    m_markRanges.clear();

    QDomNode child = node.firstChild();
    while (!child.isNull()) {
        if (child.nodeName() == "MarkRange") {
            m_markRanges.push_back(
                    WaveformMarkRange(
                            m_waveformRenderer->getGroup(),
                            child,
                            context,
                            *m_waveformRenderer->getWaveformSignalColors()));
        }
        child = child.nextSibling();
    }
}

void WaveformRenderMarkRange::draw(QPainter* painter, QPaintEvent* event) {
    Q_UNUSED(painter);
    Q_UNUSED(event);
    DEBUG_ASSERT(false);
}

void WaveformRenderMarkRange::update() {
    const QMatrix4x4 matrix = matrixForWidgetGeometry(m_waveformRenderer, false);

    TreeNode* pChild = firstChild();

    for (const auto& markRange : m_markRanges) {
        // If the mark range is not active we should not draw it.
        if (!markRange.active()) {
            continue;
        }

        // If the mark range is not visible we should not draw it.
        if (!markRange.visible()) {
            continue;
        }

        // Active mark ranges by definition have starts/ends that are not
        // disabled so no need to check.
        double startSample = markRange.start();
        double endSample = markRange.end();

        double startPosition =
                m_waveformRenderer->transformSamplePositionInRendererWorld(
                        startSample);
        double endPosition = m_waveformRenderer->transformSamplePositionInRendererWorld(endSample);

        startPosition = std::floor(startPosition);
        endPosition = std::floor(endPosition);

        // range not in the current display
        if (startPosition > m_waveformRenderer->getLength() || endPosition < 0) {
            continue;
        }

        QColor color = markRange.enabled() ? markRange.m_activeColor : markRange.m_disabledColor;
        color.setAlphaF(0.3f);

        if (!pChild) {
            appendChildNode(std::make_unique<GeometryNode>());
            pChild = lastChild();
            static_cast<GeometryNode*>(pChild)->initForRectangles<UniColorMaterial>(1);
        }

        updateNode(static_cast<GeometryNode*>(pChild),
                matrix,
                color,
                {static_cast<float>(startPosition), 0.f},
                {static_cast<float>(endPosition) + 1.f,
                        static_cast<float>(m_waveformRenderer->getBreadth())});

        pChild = static_cast<GeometryNode*>(pChild->nextSibling());
    }
    while (pChild) {
        auto pNext = pChild->nextSibling();
        removeChildNode(pChild);
        pChild = pNext;
    }
}

void WaveformRenderMarkRange::updateNode(GeometryNode* pChild,
        const QMatrix4x4& matrix,
        QColor color,
        QVector2D lt,
        QVector2D rb) {
    VertexUpdater vertexUpdater{pChild->geometry().vertexDataAs<Geometry::Point2D>()};
    vertexUpdater.addRectangle(lt, rb);
    pChild->material().setUniform(0, matrix);
    pChild->material().setUniform(1, color);
}

} // namespace allshader
