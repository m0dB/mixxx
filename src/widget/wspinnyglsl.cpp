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
    m_bCoverUpdatePending = true;
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
    updateTextures();
}

void WSpinnyGLSL::updateTextures() {
    updateTexture(m_pBgNode, m_pBgImage ? *m_pBgImage.get() : QImage{});
    updateTexture(m_pLoadedCoverNode, m_loadedCoverScaled.toImage());
    updateTexture(m_pMaskNode, m_pMaskImage ? *m_pMaskImage.get() : QImage{});
    updateTexture(m_pFgNode, m_fgImageScaled);
    updateTexture(m_pGhostNode, m_ghostImageScaled);
    m_bCoverUpdatePending = false;
}

void WSpinnyGLSL::setupVinylSignalQuality() {
}

void WSpinnyGLSL::updateVinylSignalQualityImage(
        const QColor& qual_color, const unsigned char* /*data*/) {
    m_vinylQualityColor = qual_color;
    m_vinylQualityColor.setAlphaF(0.75f);
    //     if (m_qTexture.isStorageAllocated()) {
    //         makeCurrentIfNeeded();
    //         m_qTexture.bind();
    //         // Using a texture of one byte per pixel so we can store the vinyl
    //         // signal quality data directly. The VinylQualityShader will draw this
    //         // colorized with alpha transparency.
    //         glTexSubImage2D(GL_TEXTURE_2D,
    //                 0,
    //                 0,
    //                 0,
    //                 m_iVinylScopeSize,
    //                 m_iVinylScopeSize,
    //                 GL_RED,
    //                 GL_UNSIGNED_BYTE,
    //                 data);
    //         m_qTexture.release();
    //         doneCurrent();
    //     }
}

void WSpinnyGLSL::updateTexture(GeometryNode* pNode, const QImage& image) {
    if (image.isNull()) {
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
    if (m_bCoverUpdatePending) {
        updateTexture(m_pLoadedCoverNode, m_loadedCoverScaled.toImage());
        m_bCoverUpdatePending = false;
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
    //     glDisable(GL_DEPTH_TEST);
    //     glEnable(GL_BLEND);
    //     glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //
    //     glClearColor(0.f, 0.f, 0.f, 1.f);
    //     glClear(GL_COLOR_BUFFER_BIT);
    //
    // m_textureShader.bind();

    //     int matrixLocation = m_textureShader.matrixLocation();
    //     int textureLocation = m_textureShader.textureLocation();
    //     int positionLocation = m_textureShader.positionLocation();
    //     int texcoordLocation = m_textureShader.texcoordLocation();
    //
    //     QMatrix4x4 matrix;
    //     m_textureShader.setUniformValue(matrixLocation, matrix);
    //
    //     m_textureShader.enableAttributeArray(positionLocation);
    //     m_textureShader.enableAttributeArray(texcoordLocation);
    //
    //     m_textureShader.setUniformValue(textureLocation, 0);
    //
    //     if (m_bgTexture.isStorageAllocated()) {
    //         drawTexture(&m_bgTexture);
    //     }
    //
    //     if (m_bShowCover && m_loadedCoverTextureScaled.isStorageAllocated()) {
    //         drawTexture(&m_loadedCoverTextureScaled);
    //     }
    //
    //     if (m_maskTexture.isStorageAllocated()) {
    //         drawTexture(&m_maskTexture);
    //     }
    //
    //     // Overlay the signal quality drawing if vinyl is active
    //     if (shouldDrawVinylQuality()) {
    //         m_textureShader.release();
    //         drawVinylQuality();
    //         m_textureShader.bind();
    //     }
    //
    //     // To rotate the foreground image around the center of the image,
    //     // we use the classic trick of translating the coordinate system such that
    //     // the origin is at the center of the image. We then rotate the coordinate system,
    //     // and draw the image at the corner.
    //     // p.translate(width() / 2, height() / 2);
    //
    //     bool paintGhost = m_bGhostPlayback && m_ghostTextureScaled.isStorageAllocated();
    //
    //     if (paintGhost) {
    //         QMatrix4x4 rotate;
    //         rotate.rotate(m_fGhostAngle, 0, 0, -1);
    //         m_textureShader.setUniformValue(matrixLocation, rotate);
    //
    //         drawTexture(&m_ghostTextureScaled);
    //     }
    //
    //     if (m_fgTextureScaled.isStorageAllocated()) {
    //         QMatrix4x4 rotate;
    //         rotate.rotate(m_fAngle, 0, 0, -1);
    //         m_textureShader.setUniformValue(matrixLocation, rotate);
    //
    //         drawTexture(&m_fgTextureScaled);
    //     }
    //
    //     m_textureShader.release();
}

void WSpinnyGLSL::initializeGL() {
    updateTextures();
}

// void WSpinnyGLSL::drawVinylQuality() {
//     const float texx1 = 0.f;
//     const float texy1 = 1.f;
//     const float texx2 = 1.f;
//     const float texy2 = 0.f;
//
//     const float posx2 = 1.f;
//     const float posy2 = 1.f;
//     const float posx1 = -1.f;
//     const float posy1 = -1.f;
//
//     const std::array<float, 8> posarray = {posx1, posy1, posx2, posy1, posx1,
//     posy2, posx2, posy2}; const std::array<float, 8> texarray = {texx1,
//     texy1, texx2, texy1, texx1, texy2, texx2, texy2};
//
//     m_vinylQualityShader.bind();
//     int matrixLocation = m_vinylQualityShader.matrixLocation();
//     int colorLocation = m_vinylQualityShader.colorLocation();
//     int textureLocation = m_vinylQualityShader.textureLocation();
//     int positionLocation = m_vinylQualityShader.positionLocation();
//     int texcoordLocation = m_vinylQualityShader.texcoordLocation();
//
//     QMatrix4x4 matrix;
//     m_vinylQualityShader.setUniformValue(matrixLocation, matrix);
//     m_vinylQualityShader.setUniformValue(colorLocation, m_vinylQualityColor);
//
//     m_vinylQualityShader.enableAttributeArray(positionLocation);
//     m_vinylQualityShader.enableAttributeArray(texcoordLocation);
//
//     m_vinylQualityShader.setUniformValue(textureLocation, 0);
//
//     m_vinylQualityShader.setAttributeArray(
//             positionLocation, GL_FLOAT, posarray.data(), 2);
//     m_vinylQualityShader.setAttributeArray(
//             texcoordLocation, GL_FLOAT, texarray.data(), 2);
//
//     m_qTexture.bind();
//
//     glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//
//     m_qTexture.release();
//
//     m_vinylQualityShader.release();
// }
