#include "widget/wspinnyglsl.h"

#include <QOpenGLTexture>
#include <array>

#include "moc_wspinnyglsl.cpp"
#include "rendergraph/context.h"
#include "rendergraph/engine.h"
#include "rendergraph/geometrynode.h"
#include "rendergraph/material/texturematerial.h"
#include "rendergraph/node.h"
#include "rendergraph/opacitynode.h"
#include "rendergraph/openglnode.h"
#include "rendergraph/vertexupdaters/texturedvertexupdater.h"

using namespace rendergraph;

namespace {
class ClearNode : public OpenGLNode {
  public:
    void paintGL() override {
        glClearColor(0.f, 1.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
};
} // namespace

WSpinnyGLSL::WSpinnyGLSL(
        QWidget* parent,
        const QString& group,
        UserSettingsPointer pConfig,
        VinylControlManager* pVCMan,
        BaseTrackPlayer* pPlayer)
        : WSpinnyBase(parent, group, pConfig, pVCMan, pPlayer) {
    auto pTopNode = std::make_unique<Node>();

    pTopNode->appendChildNode(std::make_unique<ClearNode>());
    m_pBgNode = createTextureNode(pTopNode.get());
    m_pLoadedCoverNode = createTextureNode(pTopNode.get());
    m_pMaskNode = createTextureNode(pTopNode.get());
    m_pGhostNode = createTextureNode(pTopNode.get());
    m_pFgNode = createTextureNode(pTopNode.get());
    m_pVinylQualityNode = createTextureNode(pTopNode.get());
    m_pEngine = std::make_unique<Engine>(std::move(pTopNode));
}

WSpinnyGLSL::~WSpinnyGLSL() {
    // cleanupGL();
    makeCurrentIfNeeded();
    // destruction of nodes needs to happen within the opengl context
    m_pEngine.reset();
    doneCurrent();
}

// void WSpinnyGLSL::cleanupGL() {
//     makeCurrentIfNeeded();
//     m_bgTexture.destroy();
//     m_maskTexture.destroy();
//     m_fgTextureScaled.destroy();
//     m_ghostTextureScaled.destroy();
//     m_loadedCoverTextureScaled.destroy();
//     m_qTexture.destroy();
//     doneCurrent();
// }

void WSpinnyGLSL::coverChanged() {
    m_bLoadedCoverNodeUpdatePending = true;
}

GeometryNode* WSpinnyGLSL::createTextureNode(Node* pParentNode) {
    auto pNode = std::make_unique<GeometryNode>();
    auto pResult = pNode.get();
    pNode->initForRectangles<TextureMaterial>(0);
    pParentNode->appendChildNode(std::move(pNode));
    return pResult;
}

void WSpinnyGLSL::draw() {
    if (shouldRender()) {
        makeCurrentIfNeeded();
        paintGL();
        doneCurrent();
    }
}

void WSpinnyGLSL::resizeGL(int w, int h) {
    w = static_cast<int>(std::lround(static_cast<qreal>(w) / devicePixelRatioF()));
    h = static_cast<int>(std::lround(static_cast<qreal>(h) / devicePixelRatioF()));
    m_pEngine->resize(w, h);
    updateTextureNodes();
}

void WSpinnyGLSL::updateTextureNodes() {
    updateTextureNode(m_pBgNode, m_pBgImage ? *m_pBgImage.get() : QImage{});
    updateTextureNode(m_pLoadedCoverNode, m_loadedCoverScaled.toImage());
    updateTextureNode(m_pMaskNode, m_pMaskImage ? *m_pMaskImage.get() : QImage{});
    updateTextureNode(m_pFgNode, m_fgImageScaled);
    updateTextureNode(m_pGhostNode, m_ghostImageScaled);

    m_bDrawingVinylQuality = shouldDrawVinylQuality();
    updateTextureNode(m_pVinylQualityNode, m_bDrawingVinylQuality ? m_vinylQualityImage : QImage{});

    m_bLoadedCoverNodeUpdatePending = false;
}

void WSpinnyGLSL::setupVinylSignalQuality() {
    m_vinylQualityImage = QImage{m_iVinylScopeSize,
            m_iVinylScopeSize,
            QImage::Format_RGBA8888_Premultiplied};
    m_vinylQualityImage.fill(0);
}

void WSpinnyGLSL::updateVinylSignalQualityImage(
        const QColor& qual_color, const unsigned char* data) {
    // Convert vinyl-quality data to rgba with premultiplied alpha,
    // to be used as texture.
    unsigned char* ptr = m_vinylQualityImage.bits();
    unsigned char* end = ptr + m_iVinylScopeSize * m_iVinylScopeSize * 4;
    const unsigned int r = qual_color.red();
    const unsigned int g = qual_color.green();
    const unsigned int b = qual_color.blue();
    while (ptr != end) {
        unsigned int a = *data++;
        a = (a + a + a) >> 2; // 75%
        *ptr++ = (r * a) >> 8;
        *ptr++ = (g * a) >> 8;
        *ptr++ = (b * a) >> 8;
        *ptr++ = a;
    }
}

void WSpinnyGLSL::updateTextureNode(GeometryNode* pNode, const QImage& image) {
    if (image.isNull()) {
        // Will effectively not draw anything
        pNode->geometry().allocate(0);
    } else {
        const int numVertices = 6; // two triangles
        pNode->geometry().allocate(numVertices);
        dynamic_cast<TextureMaterial&>(pNode->material())
                .setTexture(std::make_unique<Texture>(
                        getContext(), image));

        TexturedVertexUpdater vertexUpdater{
                pNode->geometry().vertexDataAs<Geometry::TexturedPoint2D>()};
        vertexUpdater.addRectangle({0.f, 0.f},
                {static_cast<float>(width()), static_cast<float>(height())},
                {0.f, 0.f},
                {1.f, 1.f});
    }
    pNode->markDirtyMaterial();
    pNode->markDirtyGeometry();
}

void WSpinnyGLSL::paintGL() {
    if (m_bLoadedCoverNodeUpdatePending) {
        updateTextureNode(m_pLoadedCoverNode, m_loadedCoverScaled.toImage());
        m_bLoadedCoverNodeUpdatePending = false;
    }

    if (m_bDrawingVinylQuality != shouldDrawVinylQuality()) {
        m_bDrawingVinylQuality = shouldDrawVinylQuality();
        updateTextureNode(m_pVinylQualityNode,
                m_bDrawingVinylQuality ? m_vinylQualityImage : QImage{});
    } else if (m_bDrawingVinylQuality) {
        auto& material = dynamic_cast<TextureMaterial&>(m_pVinylQualityNode->material());
        material.texture(0)->setData(m_vinylQualityImage);
    }

    {
        // TODO use scenegraph semantics for transformation
        auto matrix = m_pEngine->matrix();
        QMatrix4x4 rotate;
        rotate.rotate(m_fAngle, 0, 0, -1);
        m_pFgNode->material().setUniform(0, rotate * matrix);
    }
    {
        // TODO use scenegraph semantics for transformation
        auto matrix = m_pEngine->matrix();
        QMatrix4x4 rotate;
        rotate.rotate(m_fGhostAngle, 0, 0, -1);
        m_pGhostNode->material().setUniform(0, rotate * matrix);
    }

    m_pEngine->preprocess();
    m_pEngine->render();
}

void WSpinnyGLSL::initializeGL() {
    updateTextureNodes();
}
