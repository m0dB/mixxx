#include "widget/wspinnyglsl.h"

#include "util/assert.h"
#include "util/texture.h"

bool WSpinnyGLSL::sSkipTextures = false;
bool WSpinnyGLSL::sSkipVinylQuality = false;

WSpinnyGLSL::WSpinnyGLSL(
        QWidget* parent,
        const QString& group,
        UserSettingsPointer pConfig,
        VinylControlManager* pVCMan,
        BaseTrackPlayer* pPlayer)
        : WSpinnyBase(parent, group, pConfig, pVCMan, pPlayer) {
}

WSpinnyGLSL::~WSpinnyGLSL() {
    cleanupGL();
}

void WSpinnyGLSL::cleanupGL() {
    makeCurrentIfNeeded();
    m_pBgTexture.reset();
    m_pMaskTexture.reset();
    m_pFgTextureScaled.reset();
    m_pGhostTextureScaled.reset();
    m_pLoadedCoverTextureScaled.reset();
    m_pQTexture.reset();
    doneCurrent();
}

void WSpinnyGLSL::coverChanged() {
    if (sSkipTextures)
        return;
    if (isContextValid()) {
        makeCurrentIfNeeded();
        m_pLoadedCoverTextureScaled.reset(createTexture(m_loadedCoverScaled));
        doneCurrent();
    }
    // otherwise this will happen in initializeGL
}

void WSpinnyGLSL::draw() {
    if (shouldRender()) {
        makeCurrentIfNeeded();
        paintGL();
        doneCurrent();
    }
}

void WSpinnyGLSL::resizeGL(int w, int h) {
    Q_UNUSED(w);
    Q_UNUSED(h);
    // The images were resized in WSpinnyBase::resizeEvent.
    if (sSkipTextures)
        return;
    updateTextures();
}

void WSpinnyGLSL::updateTextures() {
    if (sSkipTextures)
        return;
    m_pBgTexture.reset(createTexture(m_pBgImage));
    m_pMaskTexture.reset(createTexture(m_pMaskImage));
    m_pFgTextureScaled.reset(createTexture(m_fgImageScaled));
    m_pGhostTextureScaled.reset(createTexture(m_ghostImageScaled));
    m_pLoadedCoverTextureScaled.reset(createTexture(m_loadedCoverScaled));
}

void WSpinnyGLSL::setupVinylSignalQuality() {
}

void WSpinnyGLSL::updateVinylSignalQualityImage(
        const QColor& qual_color, const unsigned char* data) {
    if (sSkipVinylQuality)
        return;
    m_vinylQualityColor = qual_color;
    if (m_pQTexture) {
        makeCurrentIfNeeded();
        m_pQTexture->bind();
        // Using a texture of one byte per pixel so we can store the vinyl
        // signal quality data directly. The VinylQualityShader will draw this
        // colorized with alpha transparency.
        glTexSubImage2D(GL_TEXTURE_2D,
                0,
                0,
                0,
                m_iVinylScopeSize,
                m_iVinylScopeSize,
                GL_RED,
                GL_UNSIGNED_BYTE,
                data);
        m_pQTexture->release();
        doneCurrent();
    }
}

void WSpinnyGLSL::paintGL() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (m_state == 0) {
        glClearColor(1.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }
    if (m_state == 1) {
        glClearColor(0.f, 1.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }
    if (m_state == 2) {
        glClearColor(0.f, 0.f, 1.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }

    if (!sSkipTextures) {
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (m_state == 3) {
            drawTextureFromWaveformRenderMark(0.f, 0.f, m_testTexture.get());
            return;
        }

        m_textureShader.bind();

        int matrixLocation = m_textureShader.matrixLocation();
        int samplerLocation = m_textureShader.samplerLocation();
        int positionLocation = m_textureShader.positionLocation();
        int texcoordLocation = m_textureShader.texcoordLocation();

        QMatrix4x4 matrix;
        m_textureShader.setUniformValue(matrixLocation, matrix);

        m_textureShader.enableAttributeArray(positionLocation);
        m_textureShader.enableAttributeArray(texcoordLocation);

        m_textureShader.setUniformValue(samplerLocation, 0);

        if (m_pBgTexture) {
            drawTexture(m_pBgTexture.get());
        }

        if (m_bShowCover && m_pLoadedCoverTextureScaled) {
            drawTexture(m_pLoadedCoverTextureScaled.get());
        }

        if (m_pMaskTexture) {
            drawTexture(m_pMaskTexture.get());
        }
    }

    if (!sSkipVinylQuality) {
        // Overlay the signal quality drawing if vinyl is active
        if (shouldDrawVinylQuality()) {
            if (!sSkipTextures)
                m_textureShader.release();
            drawVinylQuality();
            if (!sSkipTextures)
                m_textureShader.bind();
        }
    }

    // To rotate the foreground image around the center of the image,
    // we use the classic trick of translating the coordinate system such that
    // the origin is at the center of the image. We then rotate the coordinate system,
    // and draw the image at the corner.
    // p.translate(width() / 2, height() / 2);

    if (!sSkipTextures) {
        bool paintGhost = m_bGhostPlayback && m_pGhostTextureScaled;
        int matrixLocation = m_textureShader.matrixLocation();

        if (paintGhost) {
            QMatrix4x4 rotate;
            rotate.rotate(m_fGhostAngle, 0, 0, -1);
            m_textureShader.setUniformValue(matrixLocation, rotate);

            drawTexture(m_pGhostTextureScaled.get());
        }

        if (m_pFgTextureScaled) {
            QMatrix4x4 rotate;
            rotate.rotate(m_fAngle, 0, 0, -1);
            m_textureShader.setUniformValue(matrixLocation, rotate);

            drawTexture(m_pFgTextureScaled.get());
        }

        m_textureShader.release();
    }
}

void WSpinnyGLSL::initializeGL() {
    if (!sSkipTextures) {
        updateTextures();

        m_pQTexture.reset(new QOpenGLTexture(QOpenGLTexture::Target2D));
        m_pQTexture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        m_pQTexture->setSize(m_iVinylScopeSize, m_iVinylScopeSize);
        m_pQTexture->setFormat(QOpenGLTexture::R8_UNorm);
        m_pQTexture->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);

        generateTestTexture();

        m_textureShader.init();
    }
    if (!sSkipVinylQuality) {
        m_vinylQualityShader.init();
    }
}

void WSpinnyGLSL::drawTexture(QOpenGLTexture* texture) {
    const float texx1 = 0.f;
    const float texy1 = 1.0;
    const float texx2 = 1.f;
    const float texy2 = 0.f;

    const float tw = texture->width();
    const float th = texture->height();

    // fill centered
    const float posx2 = tw >= th ? 1.f : tw / th;
    const float posy2 = th >= tw ? 1.f : th / tw;
    const float posx1 = -posx2;
    const float posy1 = -posy2;

    const float posarray[] = {posx1, posy1, posx2, posy1, posx1, posy2, posx2, posy2};
    const float texarray[] = {texx1, texy1, texx2, texy1, texx1, texy2, texx2, texy2};

    int positionLocation = m_textureShader.positionLocation();
    int texcoordLocation = m_textureShader.texcoordLocation();

    m_textureShader.setAttributeArray(
            positionLocation, GL_FLOAT, posarray, 2);
    m_textureShader.setAttributeArray(
            texcoordLocation, GL_FLOAT, texarray, 2);

    texture->bind();

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    texture->release();
}

void WSpinnyGLSL::drawVinylQuality() {
    const float texx1 = 0.f;
    const float texy1 = 1.f;
    const float texx2 = 1.f;
    const float texy2 = 0.f;

    const float posx2 = 1.f;
    const float posy2 = 1.f;
    const float posx1 = -1.f;
    const float posy1 = -1.f;

    const float posarray[] = {posx1, posy1, posx2, posy1, posx1, posy2, posx2, posy2};
    const float texarray[] = {texx1, texy1, texx2, texy1, texx1, texy2, texx2, texy2};

    m_vinylQualityShader.bind();
    int matrixLocation = m_vinylQualityShader.matrixLocation();
    int colorLocation = m_vinylQualityShader.colorLocation();
    int samplerLocation = m_vinylQualityShader.samplerLocation();
    int positionLocation = m_vinylQualityShader.positionLocation();
    int texcoordLocation = m_vinylQualityShader.texcoordLocation();

    QMatrix4x4 matrix;
    m_vinylQualityShader.setUniformValue(matrixLocation, matrix);
    m_vinylQualityShader.setUniformValue(colorLocation, m_vinylQualityColor);

    m_vinylQualityShader.enableAttributeArray(positionLocation);
    m_vinylQualityShader.enableAttributeArray(texcoordLocation);

    m_vinylQualityShader.setUniformValue(samplerLocation, 0);

    m_vinylQualityShader.setAttributeArray(
            positionLocation, GL_FLOAT, posarray, 2);
    m_vinylQualityShader.setAttributeArray(
            texcoordLocation, GL_FLOAT, texarray, 2);

    m_pQTexture->bind();

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_pQTexture->release();

    m_vinylQualityShader.release();
}

void WSpinnyGLSL::mouseMoveEvent(QMouseEvent*) {
}

void WSpinnyGLSL::mousePressEvent(QMouseEvent*) {
    m_state++;
    if (m_state == 5) {
        m_state = 0;
    }
}

void WSpinnyGLSL::mouseReleaseEvent(QMouseEvent*) {
}

void WSpinnyGLSL::drawTextureFromWaveformRenderMark(float x, float y, QOpenGLTexture* texture) {
    const float devicePixelRatio = devicePixelRatioF();
    const float texx1 = 0.f;
    const float texy1 = 0.f;
    const float texx2 = 1.f;
    const float texy2 = 1.f;

    const float posx1 = x;
    const float posx2 = x + static_cast<float>(texture->width() / devicePixelRatio);
    const float posy1 = y;
    const float posy2 = y + static_cast<float>(texture->height() / devicePixelRatio);

    const float posarray[] = {posx1, posy1, posx2, posy1, posx1, posy2, posx2, posy2};
    const float texarray[] = {texx1, texy1, texx2, texy1, texx1, texy2, texx2, texy2};

    QMatrix4x4 matrix;
    matrix.ortho(QRectF(0.0f,
            0.0f,
            width() * devicePixelRatioF(),
            height() * devicePixelRatioF()));

    m_textureShader.bind();

    int matrixLocation = m_textureShader.uniformLocation("matrix");
    int samplerLocation = m_textureShader.uniformLocation("sampler");
    int positionLocation = m_textureShader.attributeLocation("position");
    int texcoordLocation = m_textureShader.attributeLocation("texcoor");

    m_textureShader.setUniformValue(matrixLocation, matrix);

    m_textureShader.enableAttributeArray(positionLocation);
    m_textureShader.setAttributeArray(
            positionLocation, GL_FLOAT, posarray, 2);
    m_textureShader.enableAttributeArray(texcoordLocation);
    m_textureShader.setAttributeArray(
            texcoordLocation, GL_FLOAT, texarray, 2);

    m_textureShader.setUniformValue(samplerLocation, 0);

    texture->bind();

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    texture->release();

    m_textureShader.disableAttributeArray(positionLocation);
    m_textureShader.disableAttributeArray(texcoordLocation);
    m_textureShader.release();
}

void WSpinnyGLSL::generateTestTexture() {
    const float devicePixelRatio = devicePixelRatioF();

    float imgwidth = 32.f;
    float imgheight = 32.f;

    QImage image(static_cast<int>(imgwidth * devicePixelRatio),
            static_cast<int>(imgheight * devicePixelRatio),
            QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(devicePixelRatio);
    image.fill(QColor(0, 0, 0, 0).rgba());

    QPainter painter;
    painter.begin(&image);
    painter.setWorldMatrixEnabled(false);
    painter.setPen(QColor("white"));
    QBrush bgFill = QColor("white");
    // lines next to playpos
    // Note: don't draw lines where they would overlap the triangles,
    // otherwise both translucent strokes add up to a darker tone.
    painter.drawLine(QLineF(0.f, 0.f, imgwidth, imgheight));
    painter.drawLine(QLineF(imgwidth, 0.f, 0.f, imgheight));

    painter.end();

    m_testTexture.reset(new QOpenGLTexture(image));
    m_testTexture->setMinificationFilter(QOpenGLTexture::Linear);
    m_testTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_testTexture->setWrapMode(QOpenGLTexture::ClampToBorder);
}
