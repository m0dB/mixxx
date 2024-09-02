#include "qml/qmlwaveformdisplay.h"

#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGSimpleRectNode>
#include <QSGVertexColorMaterial>
#include <cmath>

#include "mixer/basetrackplayer.h"
#include "moc_qmlwaveformdisplay.cpp"
#include "qml/qmlplayerproxy.h"
#include "rendergraph/shadercache.h"
#include "waveform/renderers/allshader/waveformrenderbackground.h"
#include "waveform/renderers/allshader/waveformrenderbeat.h"
#include "waveform/renderers/allshader/waveformrendererendoftrack.h"
#include "waveform/renderers/allshader/waveformrendererfiltered.h"
#include "waveform/renderers/allshader/waveformrendererhsv.h"
#include "waveform/renderers/allshader/waveformrendererpreroll.h"
#include "waveform/renderers/allshader/waveformrendererrgb.h"
#include "waveform/renderers/allshader/waveformrenderersimple.h"
#include "waveform/renderers/allshader/waveformrendererslipmode.h"
#include "waveform/renderers/allshader/waveformrendererstem.h"
#include "waveform/renderers/allshader/waveformrenderertextured.h"
#include "waveform/renderers/allshader/waveformrendermark.h"
#include "waveform/renderers/allshader/waveformrendermarkrange.h"

namespace {
void appendChildTo(std::unique_ptr<rendergraph::Node>& pNode, rendergraph::Node* pChild) {
    pNode->appendChildNode(std::unique_ptr<rendergraph::Node>(pChild));
}
void appendChildTo(std::unique_ptr<rendergraph::OpacityNode>& pNode, rendergraph::Node* pChild) {
    pNode->appendChildNode(std::unique_ptr<rendergraph::Node>(pChild));
}
} // namespace

namespace mixxx {
namespace qml {

QmlWaveformDisplay::QmlWaveformDisplay(QQuickItem* parent)
        : QQuickItem(parent),
          m_pPlayer(nullptr) {

    setFlag(QQuickItem::ItemHasContents, true);

    connect(this, &QmlWaveformDisplay::windowChanged, this, &QmlWaveformDisplay::slotWindowChanged);

    auto pTopNode = std::make_unique<rendergraph::Node>();
    auto pOpacityNode = std::make_unique<rendergraph::OpacityNode>();

    appendChildTo(pTopNode, addRenderer<WaveformRenderBackground>());
    appendChildTo(pOpacityNode, addRenderer<WaveformRendererEndOfTrack>());
    appendChildTo(pOpacityNode, addRenderer<WaveformRendererPreroll>());
    m_pWaveformRenderMarkRange = addRenderer<WaveformRenderMarkRange>();
    appendChildTo(pOpacityNode, m_pWaveformRenderMarkRange);

#ifdef __STEM__
    // The following two renderers work in tandem: if the rendered waveform is
    // for a stem track, WaveformRendererSignalBase will skip rendering and let
    // WaveformRendererStem do the rendering, and vice-versa.
    appendChildTo(pOpacityNode, addRenderer<WaveformRendererStem>());
#endif
    allshader::WaveformRendererSignalBase* waveformSignalRenderer =
            addWaveformSignalRenderer(
                    type, options, ::WaveformRendererAbstract::Play);
    appendChildTo(pOpacityNode, waveformSignalRenderer);

    appendChildTo(pOpacityNode, addRenderer<WaveformRenderBeat>());
    m_pWaveformRenderMark = addRenderer<WaveformRenderMark>();
    appendChildTo(pOpacityNode, m_pWaveformRenderMark);

    // if the signal renderer supports slip, we add it again, now for slip, together with the
    // other slip renderers
    if (waveformSignalRenderer && waveformSignalRenderer->supportsSlip()) {
        // The following renderer will add an overlay waveform if a slip is in progress
        appendChildTo(pOpacityNode, addRenderer<WaveformRendererSlipMode>());
        appendChildTo(pOpacityNode,
                addRenderer<WaveformRendererPreroll>(
                        ::WaveformRendererAbstract::Slip));
#ifdef __STEM__
        appendChildTo(pOpacityNode,
                addRenderer<WaveformRendererStem>(
                        ::WaveformRendererAbstract::Slip));
#endif
        appendChildTo(pOpacityNode,
                addWaveformSignalRenderer(
                        type, options, ::WaveformRendererAbstract::Slip));
        appendChildTo(pOpacityNode,
                addRenderer<WaveformRenderBeat>(
                        ::WaveformRendererAbstract::Slip));
        appendChildTo(pOpacityNode,
                addRenderer<WaveformRenderMark>(
                        ::WaveformRendererAbstract::Slip));
    }

    m_pOpacityNode = pOpacityNode.get();
    pTopNode->appendChildNode(std::move(pOpacityNode));

    m_pGraph = std::move(pTopNode);
}

QmlWaveformDisplay::~QmlWaveformDisplay() {
    m_pGraph.reset();
    rendergraph::ShaderCache::purge();
}

void QmlWaveformDisplay::slotWindowChanged(QQuickWindow* window) {
    connect(window, &QQuickWindow::frameSwapped, this, &QmlWaveformDisplay::slotFrameSwapped);
    m_timer.restart();
}

void QmlWaveformDisplay::slotFrameSwapped() {
    m_timer.restart();
}

void QmlWaveformDisplay::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) {
    m_geometryChanged = true;
    update();
    QQuickItem::geometryChange(newGeometry, oldGeometry);
}

QSGNode* QmlWaveformDisplay::updatePaintNode(QSGNode* old, QQuickItem::UpdatePaintNodeData*) {
    QSGRectangleNode* clipNode;

    if (!old) {
        // clipNode = window()->createRectangleNode();
        // clipNode->setColor(QColor(0, 0, 0, 255));
        // clipNode->setRect(boundingRect());

        // m_node = std::make_unique<rendergraph::Node>();
        // m_node->appendChildNode(std::make_unique<rendergraph::ExampleNode1>());
        // m_node->appendChildNode(std::make_unique<rendergraph::ExampleNode2>());
        // m_node->appendChildNode(std::make_unique<rendergraph::ExampleNode3>());

        // {
        //     QImage img(":/example/images/test.png");
        //     auto context = rendergraph::createSgContext(window());
        //     static_cast<rendergraph::ExampleNode3*>(m_node->lastChild())
        //             ->setTexture(std::make_unique<rendergraph::Texture>(
        //                     *context, img));
        // }

        // clipNode->appendChildNode(rendergraph::sgNode(m_node.get()));
        m_pOpacityNode->setOpacity(shouldOnlyDrawBackground() ? 0.f : 1.f);

        m_pWaveformRenderMark->update();
        m_pWaveformRenderMarkRange->update();

        m_pGraph->preprocess();
        m_pGraph->render();

        old = clipNode;
    } else {
        clipNode = static_cast<QSGRectangleNode*>(old);
    }

    if (m_geometryChanged) {
        clipNode->setRect(boundingRect());
        m_geometryChanged = false;
    }

    return clipNode;
}

QmlPlayerProxy* QmlWaveformDisplay::getPlayer() const {
    return m_pPlayer;
}

void QmlWaveformDisplay::setPlayer(QmlPlayerProxy* pPlayer) {
    if (m_pPlayer == pPlayer) {
        return;
    }

    if (m_pPlayer != nullptr) {
        m_pPlayer->internalTrackPlayer()->disconnect(this);
    }

    m_pPlayer = pPlayer;

    if (m_pPlayer != nullptr) {
        setCurrentTrack(m_pPlayer->internalTrackPlayer()->getLoadedTrack());
        connect(m_pPlayer->internalTrackPlayer(),
                &BaseTrackPlayer::newTrackLoaded,
                this,
                &QmlWaveformDisplay::slotTrackLoaded);
        connect(m_pPlayer->internalTrackPlayer(),
                &BaseTrackPlayer::loadingTrack,
                this,
                &QmlWaveformDisplay::slotTrackLoading);
        connect(m_pPlayer->internalTrackPlayer(),
                &BaseTrackPlayer::playerEmpty,
                this,
                &QmlWaveformDisplay::slotTrackUnloaded);
    }

    emit playerChanged();
    update();
}

void QmlWaveformDisplay::setGroup(const QString& group) {
    if (m_group == group) {
        return;
    }

    m_group = group;
    emit groupChanged(group);

    // TODO m0dB unique_ptr ?
    delete m_pWaveformDisplayRange;
    m_pWaveformDisplayRange = new WaveformDisplayRange(m_group);
    m_pWaveformDisplayRange->init();
}

const QString& QmlWaveformDisplay::getGroup() const {
    return m_group;
}

void QmlWaveformDisplay::slotTrackLoaded(TrackPointer pTrack) {
    // TODO: Investigate if it's a bug that this debug assertion fails when
    // passing tracks on the command line
    // DEBUG_ASSERT(m_pCurrentTrack == pTrack);
    setCurrentTrack(pTrack);
}

void QmlWaveformDisplay::slotTrackLoading(TrackPointer pNewTrack, TrackPointer pOldTrack) {
    Q_UNUSED(pOldTrack); // only used in DEBUG_ASSERT
    DEBUG_ASSERT(m_pCurrentTrack == pOldTrack);
    setCurrentTrack(pNewTrack);
}

void QmlWaveformDisplay::slotTrackUnloaded() {
    setCurrentTrack(nullptr);
}

void QmlWaveformDisplay::setCurrentTrack(TrackPointer pTrack) {
    // TODO: Check if this is actually possible
    if (m_pCurrentTrack == pTrack) {
        return;
    }

    if (m_pCurrentTrack != nullptr) {
        disconnect(m_pCurrentTrack.get(), nullptr, this, nullptr);
    }

    m_pCurrentTrack = pTrack;
    if (pTrack != nullptr) {
        connect(pTrack.get(),
                &Track::waveformSummaryUpdated,
                this,
                &QmlWaveformDisplay::slotWaveformUpdated);
    }
    slotWaveformUpdated();

    if (m_pWaveformDisplayRange) {
        m_pWaveformDisplayRange->setTrack(m_pCurrentTrack);
    }
}

void QmlWaveformDisplay::slotWaveformUpdated() {
    update();
}

} // namespace qml
} // namespace mixxx
