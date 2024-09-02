#include "waveform/renderers/allshader/waveformrendermark.h"

#include <QPainterPath>

#include "rendergraph/context.h"
#include "rendergraph/geometry.h"
#include "rendergraph/geometrynode.h"
#include "rendergraph/material/rgbamaterial.h"
#include "rendergraph/material/texturematerial.h"
#include "rendergraph/texture.h"
#include "rgbavertexupdater.h"
#include "texturedvertexupdater.h"
#include "track/track.h"
#include "util/colorcomponents.h"
#include "waveform/renderers/allshader/digitsrenderer.h"
#include "waveform/renderers/allshader/matrixforwidgetgeometry.h"
#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveform/waveformwidgetfactory.h"

using namespace rendergraph;

// On the use of QPainter:
//
// The renderers in this folder are optimized to use GLSL shaders and refrain
// from using QPainter on the QOpenGLWindow, which causes degredated performance.
//
// This renderer does use QPainter (indirectly, in WaveformMark::generateImage), but
// only to draw on a QImage. This is only done once when needed and the images are
// then used as textures to be drawn with a GLSL shader.

class WaveformMarkNode : public rendergraph::GeometryNode {
  public:
    WaveformMark* m_pOwner{};

    WaveformMarkNode(WaveformMark* pOwner, const QImage& image)
            : m_pOwner(pOwner) {
        initForRectangles<TextureMaterial>(1);
        updateTexture(image);
    }
    void updateTexture(const QImage& image) {
        Context context;
        dynamic_cast<TextureMaterial&>(material())
                .setTexture(std::make_unique<Texture>(context, image));
        m_textureWidth = image.width();
        m_textureHeight = image.height();
    }
    void update(const QMatrix4x4& matrix, float x, float y, float devicePixelRatio) {
        material().setUniform(0, matrix);

        TexturedVertexUpdater vertexUpdater{
                geometry().vertexDataAs<Geometry::TexturedPoint2D>()};
        vertexUpdater.addRectangle({x, y},
                {x + m_textureWidth / devicePixelRatio,
                        y + m_textureHeight / devicePixelRatio},
                {0.f, 0.f},
                {1.f, 1.f});
    }
    float textureWidth() const {
        return m_textureWidth;
    }
    float textureHeight() const {
        return m_textureHeight;
    }

  public:
    float m_textureWidth{};
    float m_textureHeight{};
};

class WaveformMarkNodeGraphics : public WaveformMark::Graphics {
  public:
    WaveformMarkNodeGraphics(WaveformMark* pOwner, const QImage& image)
            : m_pNode(std::make_unique<WaveformMarkNode>(pOwner, image)) {
    }
    void updateTexture(const QImage& image) {
        if (!m_pNode) {
            return;
        }
        waveformMarkNode()->updateTexture(image);
    }
    void update(const QMatrix4x4& matrix, float x, float y, float devicePixelRatio) {
        if (!m_pNode) {
            return;
        }
        waveformMarkNode()->update(matrix, x, y, devicePixelRatio);
    }
    float textureWidth() const {
        if (!m_pNode) {
            return 0.0;
        }
        return waveformMarkNode()->textureWidth();
    }
    float textureHeight() const {
        if (!m_pNode) {
            return 0.0;
        }
        return waveformMarkNode()->textureHeight();
    }
    void setNode(std::unique_ptr<Node>&& pNode) {
        m_pNode = std::move(pNode);
    }
    void moveNodeToChildrenOf(Node* pParent) {
        pParent->appendChildNode(std::move(m_pNode));
    }

  private:
    WaveformMarkNode* waveformMarkNode() const {
        return static_cast<WaveformMarkNode*>(m_pNode.get());
    }

    std::unique_ptr<rendergraph::Node> m_pNode;
};

// Both allshader::WaveformRenderMark and the non-GL ::WaveformRenderMark derive
// from WaveformRenderMarkBase. The base-class takes care of updating the marks
// when needed and flagging them when their image needs to be updated (resizing,
// cue changes, position changes)
//
// While in the case of ::WaveformRenderMark those images can be updated immediately,
// in the case of allshader::WaveformRenderMark we need to do that when we have an
// openGL context, as we create new textures.
//
// The boolean argument for the WaveformRenderMarkBase constructor indicates
// that updateMarkImages should not be called immediately.

namespace {
QString timeSecToString(double timeSec) {
    int hundredths = std::lround(timeSec * 100.0);
    int seconds = hundredths / 100;
    hundredths -= seconds * 100;
    int minutes = seconds / 60;
    seconds -= minutes * 60;

    return QString::asprintf("%d:%02d.%02d", minutes, seconds, hundredths);
}

} // namespace

allshader::WaveformRenderMark::WaveformRenderMark(
        WaveformWidgetRenderer* waveformWidget,
        ::WaveformRendererAbstract::PositionSource type)
        : ::WaveformRenderMarkBase(waveformWidget, false),
          m_beatsUntilMark(0),
          m_timeUntilMark(0.0),
          m_pTimeRemainingControl(nullptr),
          m_isSlipRenderer(type == ::WaveformRendererAbstract::Slip) {
    appendChildNode(std::make_unique<Node>());
    m_pRangeNodesParent = lastChild();

    appendChildNode(std::make_unique<Node>());
    m_pMarkNodesParent = lastChild();

    appendChildNode(std::make_unique<DigitsRenderNode>());
    m_pDigitsRenderNode = static_cast<DigitsRenderNode*>(lastChild());

    appendChildNode(std::make_unique<GeometryNode>());
    m_pPlayPosNode = static_cast<GeometryNode*>(lastChild());
    m_pPlayPosNode->initForRectangles<TextureMaterial>(1);
}

void allshader::WaveformRenderMark::draw(QPainter* painter, QPaintEvent* event) {
    Q_UNUSED(painter);
    Q_UNUSED(event);
    DEBUG_ASSERT(false);
}

bool allshader::WaveformRenderMark::init() {
    m_pTimeRemainingControl = std::make_unique<ControlProxy>(
            m_waveformRenderer->getGroup(), "time_remaining");
    return true;
}

void allshader::WaveformRenderMark::initialize() {
    // Will create textures so requires OpenGL context
    updateMarkImages();
    updatePlayPosMarkTexture();
    const auto untilMarkTextPointSize =
            WaveformWidgetFactory::instance()->getUntilMarkTextPointSize();
    m_pDigitsRenderNode->updateTexture(untilMarkTextPointSize,
            getMaxHeightForText(),
            m_waveformRenderer->getDevicePixelRatio());
    Node::initialize();
}

void allshader::WaveformRenderMark::updateRangeNode(GeometryNode* pNode,
        const QMatrix4x4& matrix,
        const QRectF& rect,
        QColor color) {
    // draw a gradient towards transparency at the upper and lower 25% of the waveform view

    const float qh = static_cast<float>(std::floor(rect.height() * 0.25));
    const float posx1 = static_cast<float>(rect.x());
    const float posx2 = static_cast<float>(rect.x() + rect.width());
    const float posy1 = static_cast<float>(rect.y());
    const float posy2 = static_cast<float>(rect.y()) + qh;
    const float posy3 = static_cast<float>(rect.y() + rect.height()) - qh;
    const float posy4 = static_cast<float>(rect.y() + rect.height());

    float r, g, b, a;

    getRgbF(color, &r, &g, &b, &a);

    RGBAVertexUpdater vertexUpdater{pNode->geometry().vertexDataAs<Geometry::RGBAColoredPoint2D>()};
    vertexUpdater.addRectangleVGradient(
            {posx1, posy1}, {posx2, posy2}, {r, g, b, a}, {r, g, b, 0.f});
    vertexUpdater.addRectangleVGradient(
            {posx1, posy4}, {posx2, posy3}, {r, g, b, a}, {r, g, b, 0.f});

    pNode->material().setUniform(0, matrix);
}

bool allshader::WaveformRenderMark::isSubtreeBlocked() const {
    return m_isSlipRenderer && !m_waveformRenderer->isSlipActive();
}

void allshader::WaveformRenderMark::update() {
    if (isSubtreeBlocked()) {
        return;
    }

    // For each WaveformMark we create a GeometryNode with Texture
    // (in updateMarkImage). Of these GeometryNodes, we append the
    // the ones that need to be shown on screen as children to
    // m_pMarkNodesParent (transferring ownership).
    //
    // At the beginning of a new frame, we remove all the child nodes
    // from m_pMarkNodesParent and store each with their mark
    // (transferring ownership). Later in this function we move the
    // visible nodes back to m_pMarkNodesParent children.
    while (auto pChild = m_pMarkNodesParent->firstChild()) {
        // Pop child from front of m_pMarkNodesParent
        auto pRemoved = m_pMarkNodesParent->removeChildNode(pChild);
        // Determine its WaveformMark
        auto pMarkNode = static_cast<WaveformMarkNode*>(pRemoved.get());
        auto pMark = pMarkNode->m_pOwner;
        auto pGraphics = static_cast<WaveformMarkNodeGraphics*>(pMark->m_pGraphics.get());
        // Store the node with the WaveformMark
        pGraphics->setNode(std::move(pRemoved));
    }

    auto positionType = m_isSlipRenderer ? ::WaveformRendererAbstract::Slip
                                         : ::WaveformRendererAbstract::Play;
    bool slipActive = m_waveformRenderer->isSlipActive();

    const float devicePixelRatio = m_waveformRenderer->getDevicePixelRatio();
    QList<WaveformWidgetRenderer::WaveformMarkOnScreen> marksOnScreen;

    for (const auto& pMark : std::as_const(m_marks)) {
        pMark->setBreadth(slipActive ? m_waveformRenderer->getBreadth() / 2
                                     : m_waveformRenderer->getBreadth());
    }

    // Generate initial node or update its texture if needed for each of
    // the WaveformMarks (in which case updateMarkImage is called)
    // (Will create textures so requires OpenGL context)
    updateMarkImages();

    QMatrix4x4 matrix = matrixForWidgetGeometry(m_waveformRenderer, false);

    const double playPosition = m_waveformRenderer->getTruePosSample(positionType);
    double nextMarkPosition = std::numeric_limits<double>::max();

    Node* pRangeChild = m_pRangeNodesParent->firstChild();

    for (const auto& pMark : std::as_const(m_marks)) {
        if (!pMark->isValid()) {
            continue;
        }

        const double samplePosition = pMark->getSamplePosition();

        if (samplePosition == Cue::kNoPosition) {
            continue;
        }

        auto pMarkGraphics = pMark->m_pGraphics.get();
        auto pMarkNodeGraphics = static_cast<WaveformMarkNodeGraphics*>(pMarkGraphics);
        if (!pMarkGraphics) // is this even possible?
            continue;

        const float currentMarkPoint =
                std::round(
                        static_cast<float>(
                                m_waveformRenderer
                                        ->transformSamplePositionInRendererWorld(
                                                samplePosition, positionType)) *
                        devicePixelRatio) /
                devicePixelRatio;
        if (pMark->isShowUntilNext() &&
                samplePosition >= playPosition + 1.0 &&
                samplePosition < nextMarkPosition) {
            nextMarkPosition = samplePosition;
        }
        const double sampleEndPosition = pMark->getSampleEndPosition();

        // Pixmaps are expected to have the mark stroke at the center,
        // and preferably have an odd width in order to have the stroke
        // exactly at the sample position.
        const float markHalfWidth = pMarkNodeGraphics->textureWidth() / devicePixelRatio / 2.f;
        const float drawOffset = currentMarkPoint - markHalfWidth;

        bool visible = false;
        // Check if the current point needs to be displayed.
        if (drawOffset > -markHalfWidth &&
                drawOffset < m_waveformRenderer->getLength() +
                                markHalfWidth) {
            pMarkNodeGraphics->update(matrix,
                    drawOffset,
                    !m_isSlipRenderer && slipActive
                            ? m_waveformRenderer->getBreadth() / 2
                            : 0,
                    devicePixelRatio);

            // transfer back to m_pMarkNodesParent children, for rendering
            pMarkNodeGraphics->moveNodeToChildrenOf(m_pMarkNodesParent);

            visible = true;
        }

        // Check if the range needs to be displayed.
        if (samplePosition != sampleEndPosition && sampleEndPosition != Cue::kNoPosition) {
            DEBUG_ASSERT(samplePosition < sampleEndPosition);
            const float currentMarkEndPoint = static_cast<
                    float>(
                    m_waveformRenderer
                            ->transformSamplePositionInRendererWorld(
                                    sampleEndPosition, positionType));

            if (visible || currentMarkEndPoint > 0) {
                QColor color = pMark->fillColor();
                color.setAlphaF(0.4f);

                // Reuse, or create new when needed
                if (!pRangeChild) {
                    m_pRangeNodesParent->appendChildNode(std::make_unique<GeometryNode>());
                    pRangeChild = m_pRangeNodesParent->lastChild();
                    static_cast<GeometryNode*>(pRangeChild)->initForRectangles<RGBAMaterial>(2);
                }

                updateRangeNode(static_cast<GeometryNode*>(pRangeChild),
                        matrix,
                        QRectF(QPointF(currentMarkPoint, 0),
                                QPointF(currentMarkEndPoint,
                                        m_waveformRenderer->getBreadth())),
                        color);

                visible = true;
                pRangeChild = pRangeChild->nextSibling();
            }
        }

        if (visible) {
            marksOnScreen.append(
                    WaveformWidgetRenderer::WaveformMarkOnScreen{
                            pMark, static_cast<int>(drawOffset)});
        }
    }

    // Remove unused nodes
    while (pRangeChild) {
        auto pNext = static_cast<GeometryNode*>(pRangeChild->nextSibling());
        m_pRangeNodesParent->removeChildNode(pRangeChild);
        pRangeChild = pNext;
    }

    m_waveformRenderer->setMarkPositions(marksOnScreen);

    const float currentMarkPoint =
            std::round(static_cast<float>(
                               m_waveformRenderer->getPlayMarkerPosition() *
                               m_waveformRenderer->getLength()) *
                    devicePixelRatio) /
            devicePixelRatio;

    {
        const float markHalfWidth = 11.f / 2.f;
        const float drawOffset = currentMarkPoint - markHalfWidth;

        m_pPlayPosNode->material().setUniform(0, matrix);

        TexturedVertexUpdater vertexUpdater{
                m_pPlayPosNode->geometry()
                        .vertexDataAs<Geometry::TexturedPoint2D>()};
        vertexUpdater.addRectangle({drawOffset, 0.f},
                {drawOffset + 11.f, static_cast<float>(m_waveformRenderer->getBreadth())},
                {0.f, 0.f},
                {1.f, 1.f});
    }

    if (WaveformWidgetFactory::instance()->getUntilMarkShowBeats() ||
            WaveformWidgetFactory::instance()->getUntilMarkShowTime()) {
        updateUntilMark(playPosition, nextMarkPosition);
        drawUntilMark(matrix, currentMarkPoint + 20);
    }
}

void allshader::WaveformRenderMark::drawUntilMark(const QMatrix4x4& matrix, float x) {
    const bool untilMarkShowBeats = WaveformWidgetFactory::instance()->getUntilMarkShowBeats();
    const bool untilMarkShowTime = WaveformWidgetFactory::instance()->getUntilMarkShowTime();
    const auto untilMarkAlign = WaveformWidgetFactory::instance()->getUntilMarkAlign();

    const auto untilMarkTextPointSize =
            WaveformWidgetFactory::instance()->getUntilMarkTextPointSize();
    m_pDigitsRenderNode->updateTexture(untilMarkTextPointSize,
            getMaxHeightForText(),
            m_waveformRenderer->getDevicePixelRatio());

    if (m_timeUntilMark == 0.0) {
        m_pDigitsRenderNode->clear();
        return;
    }
    const float ch = m_pDigitsRenderNode->height();

    float y = untilMarkAlign == Qt::AlignTop ? 0.f
            : untilMarkAlign == Qt::AlignBottom
            ? m_waveformRenderer->getBreadth() - ch
            : m_waveformRenderer->getBreadth() / 2.f;

    bool multiLine = untilMarkShowBeats && untilMarkShowTime && ch * 2.f < getMaxHeightForText();

    if (multiLine) {
        if (untilMarkAlign != Qt::AlignTop) {
            y -= ch;
        }
    } else {
        if (untilMarkAlign != Qt::AlignTop && untilMarkAlign != Qt::AlignBottom) {
            // center
            y -= ch / 2.f;
        }
    }

    m_pDigitsRenderNode->update(matrix,
            x,
            y,
            multiLine,
            untilMarkShowBeats ? QString::number(m_beatsUntilMark) : QString{},
            untilMarkShowTime ? timeSecToString(m_timeUntilMark) : QString{});
}

// Generate the texture used to draw the play position marker.
// Note that in the legacy waveform widgets this is drawn directly
// in the WaveformWidgetRenderer itself. Doing it here is cleaner.
void allshader::WaveformRenderMark::updatePlayPosMarkTexture() {
    float imgwidth;
    float imgheight;

    const float height = m_waveformRenderer->getBreadth();
    const float devicePixelRatio = m_waveformRenderer->getDevicePixelRatio();

    const float lineX = 5.5f;

    imgwidth = 11.f;
    imgheight = height;

    QImage image(static_cast<int>(imgwidth * devicePixelRatio),
            static_cast<int>(imgheight * devicePixelRatio),
            QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(devicePixelRatio);
    image.fill(QColor(0, 0, 0, 0).rgba());

    // See comment on use of QPainter at top of file
    QPainter painter;

    painter.begin(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setWorldMatrixEnabled(false);

    const QColor fgColor{m_waveformRenderer->getWaveformSignalColors()->getPlayPosColor()};
    const QColor bgColor{m_waveformRenderer->getWaveformSignalColors()->getBgColor()};

    // draw dim outlines to increase playpos/waveform contrast
    painter.setPen(bgColor);
    painter.setOpacity(0.5);
    // lines next to playpos
    // Note: don't draw lines where they would overlap the triangles,
    // otherwise both translucent strokes add up to a darker tone.
    painter.drawLine(QLineF(lineX + 1.f, 4.f, lineX + 1.f, height));
    painter.drawLine(QLineF(lineX - 1.f, 4.f, lineX - 1.f, height));

    // triangle at top edge
    // Increase line/waveform contrast
    painter.setOpacity(0.8);
    {
        QPointF baseL = QPointF(lineX - 5.f, 0.f);
        QPointF baseR = QPointF(lineX + 5.f, 0.f);
        QPointF tip = QPointF(lineX, 5.f);
        drawTriangle(&painter, bgColor, baseL, baseR, tip);
    }
    // draw colored play position indicators
    painter.setPen(fgColor);
    painter.setOpacity(1.0);
    // play position line
    painter.drawLine(QLineF(lineX, 0.f, lineX, height));
    // triangle at top edge
    {
        QPointF baseL = QPointF(lineX - 4.f, 0.f);
        QPointF baseR = QPointF(lineX + 4.f, 0.f);
        QPointF tip = QPointF(lineX, 4.f);
        drawTriangle(&painter, fgColor, baseL, baseR, tip);
    }
    painter.end();

    Context context;
    dynamic_cast<TextureMaterial&>(m_pPlayPosNode->material())
            .setTexture(std::make_unique<Texture>(context, image));
}

void allshader::WaveformRenderMark::drawTriangle(QPainter* painter,
        const QBrush& fillColor,
        QPointF baseL,
        QPointF baseR,
        QPointF tip) {
    QPainterPath triangle;
    painter->setPen(Qt::NoPen);
    triangle.moveTo(baseL);
    triangle.lineTo(tip);
    triangle.lineTo(baseR);
    triangle.closeSubpath();
    painter->fillPath(triangle, fillColor);
}

void allshader::WaveformRenderMark::resize(int, int) {
    // Will create textures so requires OpenGL context
    updateMarkImages();
    updatePlayPosMarkTexture();
}

void allshader::WaveformRenderMark::updateMarkImage(WaveformMarkPointer pMark) {
    if (!pMark->m_pGraphics) {
        pMark->m_pGraphics =
                std::make_unique<WaveformMarkNodeGraphics>(pMark.get(),
                        pMark->generateImage(
                                m_waveformRenderer->getDevicePixelRatio()));
    } else {
        auto pGraphics = static_cast<WaveformMarkNodeGraphics*>(pMark->m_pGraphics.get());
        pGraphics->updateTexture(pMark->generateImage(m_waveformRenderer->getDevicePixelRatio()));
    }
}

void allshader::WaveformRenderMark::updateUntilMark(
        double playPosition, double nextMarkPosition) {
    m_beatsUntilMark = 0;
    m_timeUntilMark = 0.0;
    if (nextMarkPosition == std::numeric_limits<double>::max()) {
        return;
    }

    TrackPointer trackInfo = m_waveformRenderer->getTrackInfo();

    if (!trackInfo) {
        return;
    }

    const double endPosition = m_waveformRenderer->getTrackSamples();
    const double remainingTime = m_pTimeRemainingControl->get();

    mixxx::BeatsPointer trackBeats = trackInfo->getBeats();
    if (!trackBeats) {
        return;
    }

    auto itA = trackBeats->iteratorFrom(
            mixxx::audio::FramePos::fromEngineSamplePos(playPosition));
    auto itB = trackBeats->iteratorFrom(
            mixxx::audio::FramePos::fromEngineSamplePos(nextMarkPosition));

    // itB is the beat at or after the nextMarkPosition.
    if (itB->toEngineSamplePos() > nextMarkPosition) {
        // if itB is after nextMarkPosition, the previous beat might be closer
        // and it the one we are interested in
        if (nextMarkPosition - (itB - 1)->toEngineSamplePos() <
                itB->toEngineSamplePos() - nextMarkPosition) {
            itB--;
        }
    }

    if (std::abs(itA->toEngineSamplePos() - playPosition) < 1) {
        m_currentBeatPosition = itA->toEngineSamplePos();
        m_beatsUntilMark = std::distance(itA, itB);
        itA++;
        m_nextBeatPosition = itA->toEngineSamplePos();
    } else {
        m_nextBeatPosition = itA->toEngineSamplePos();
        itA--;
        m_currentBeatPosition = itA->toEngineSamplePos();
        m_beatsUntilMark = std::distance(itA, itB);
    }
    // As endPosition - playPosition corresponds with remainingTime,
    // we calculate the proportional part of nextMarkPosition - playPosition
    m_timeUntilMark = std::max(0.0,
            remainingTime * (nextMarkPosition - playPosition) /
                    (endPosition - playPosition));
}

float allshader::WaveformRenderMark::getMaxHeightForText() const {
    return m_waveformRenderer->getBreadth() / 3;
}
