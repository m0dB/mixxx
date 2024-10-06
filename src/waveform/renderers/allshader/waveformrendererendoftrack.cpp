#include "waveform/renderers/allshader/waveformrendererendoftrack.h"

#include <QDomNode>
#include <QSGNode>
#include <QVector4D>
#include <memory>

#include "control/controlproxy.h"
#include "rendergraph/geometry.h"
#include "rendergraph/material/rgbamaterial.h"
#include "rendergraph/vertexupdaters/rgbavertexupdater.h"
#include "util/colorcomponents.h"
#include "waveform/renderers/allshader/matrixforwidgetgeometry.h"
#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveform/waveformwidgetfactory.h"
#include "widget/wskincolor.h"

namespace {

constexpr int kBlinkingPeriodMillis = 1000;

} // anonymous namespace

using namespace rendergraph;

namespace allshader {

WaveformRendererEndOfTrack::WaveformRendererEndOfTrack(
        WaveformWidgetRenderer* waveformWidget, QColor color)
        : ::WaveformRendererAbstract(waveformWidget),
          m_pEndOfTrackControl(nullptr),
          m_pTimeRemainingControl(nullptr),
          m_color(color) {
    initForRectangles<RGBAMaterial>(0);
    setUsePreprocess(true);
}

void WaveformRendererEndOfTrack::draw(QPainter* painter, QPaintEvent* event) {
    Q_UNUSED(painter);
    Q_UNUSED(event);
    DEBUG_ASSERT(false);
}

bool WaveformRendererEndOfTrack::init() {
    m_timer.restart();

    return true;
}

void WaveformRendererEndOfTrack::setup(const QDomNode& node, const SkinContext& context) {
    m_color = QColor(200, 25, 20);
    const QString endOfTrackColorName = context.selectString(node, "EndOfTrackColor");
    if (!endOfTrackColorName.isNull()) {
        m_color = QColor(endOfTrackColorName);
        m_color = WSkinColor::getCorrectColor(m_color);
    }
}

void WaveformRendererEndOfTrack::preprocess() {
    if (!m_pEndOfTrackControl) {
        m_pEndOfTrackControl = std::make_unique<ControlProxy>(
                m_waveformRenderer->getGroup(), "end_of_track");
    }
    if (!m_pTimeRemainingControl) {
        m_pTimeRemainingControl = std::make_unique<ControlProxy>(
                m_waveformRenderer->getGroup(), "time_remaining");
    }

    const int elapsedTotal = m_timer.elapsed().toIntegerMillis();
    const int elapsed = elapsedTotal % kBlinkingPeriodMillis;

    if (elapsedTotal >= m_lastFrameCountLogged + 1000) {
        if (elapsedTotal >= m_lastFrameCountLogged + 2000) {
            m_lastFrameCountLogged = elapsedTotal;
        }
        m_lastFrameCountLogged += 1000;
        qDebug() << m_frameCount;
        m_frameCount = 0;
    }
    m_frameCount++;

    const double blinkIntensity = (double)(2 * abs(elapsed - kBlinkingPeriodMillis / 2)) /
            kBlinkingPeriodMillis;

    const double remainingTime = m_pTimeRemainingControl->get();
    const double remainingTimeTriggerSeconds =
            WaveformWidgetFactory::instance()->getEndOfTrackWarningTime();
    const double criticalIntensity = static_cast<double>(elapsed) /
            static_cast<double>(
                    kBlinkingPeriodMillis); //(remainingTimeTriggerSeconds -
                                            // remainingTime) /
                                            // remainingTimeTriggerSeconds;

    const double alpha = std::max(0.0, std::min(1.0, criticalIntensity * blinkIntensity));

    if (alpha != 0.0) {
        float r, g, b, a;
        getRgbF(m_color, &r, &g, &b, &a);

        const QRectF& rect = m_waveformRenderer->getRect();

        const float posx0 = static_cast<float>(rect.x());
        const float posx1 = static_cast<float>(rect.x() + rect.width() / 2.0);
        const float posx2 = static_cast<float>(rect.x() + rect.width());
        const float posy1 = static_cast<float>(rect.y());
        const float posy2 = static_cast<float>(rect.y() + rect.height());

        float minAlpha = 0.5f * static_cast<float>(alpha);
        float maxAlpha = 0.83f * static_cast<float>(alpha);

        geometry().allocate(6 * 2);
        RGBAVertexUpdater vertexUpdater{geometry().vertexDataAs<Geometry::RGBAColoredPoint2D>()};
        vertexUpdater.addRectangleHGradient(
                {posx0, posy1}, {posx1, posy2}, {r, g, b, minAlpha}, {r, g, b, minAlpha});
        vertexUpdater.addRectangleHGradient(
                {posx1, posy1}, {posx2, posy2}, {r, g, b, minAlpha}, {r, g, b, maxAlpha});

        QMatrix4x4 matrix = matrixForWidgetGeometry(m_waveformRenderer, false);
        material().setUniform(0, matrix);

        markDirtyGeometry();
        markDirtyMaterial();
    } else {
        geometry().allocate(0);
    }
}

bool WaveformRendererEndOfTrack::isSubtreeBlocked() const {
    return false;
    // return !(!m_pEndOfTrackControl || m_pEndOfTrackControl->toBool());
}

} // namespace allshader
