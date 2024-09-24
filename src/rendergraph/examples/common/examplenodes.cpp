#include "examplenodes.h"

#include <QColor>
#include <QMatrix4x4>
#include <QPainter>
#include <QPainterPath>
#include <QVector2D>

#include "rendergraph/geometry.h"
#include "rendergraph/material/endoftrackmaterial.h"
#include "rendergraph/material/patternmaterial.h"
#include "rendergraph/material/texturematerial.h"
#include "rendergraph/vertexupdaters/texturedvertexupdater.h"

using namespace rendergraph;

namespace {
QImage drawPrerollImage(float markerLength,
        float markerBreadth,
        float devicePixelRatio,
        QColor color) {
    const int imagePixelW = static_cast<int>(markerLength * devicePixelRatio + 0.5f);
    const int imagePixelH = static_cast<int>(markerBreadth * devicePixelRatio + 0.5f);
    const float imageW = static_cast<float>(imagePixelW) / devicePixelRatio;
    const float imageH = static_cast<float>(imagePixelH) / devicePixelRatio;

    QImage image(imagePixelW, imagePixelH, QImage::Format_RGBA8888_Premultiplied);
    image.setDevicePixelRatio(devicePixelRatio);

    const float penWidth = 1.5f;
    const float offset = penWidth / 2.f;

    image.fill(Qt::transparent);
    QPainter painter;
    painter.begin(&image);

    painter.setWorldMatrixEnabled(false);

    QPen pen(color);
    pen.setWidthF(penWidth);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.setRenderHints(QPainter::Antialiasing);
    // Draw base the right, tip to the left
    QPointF p0{imageW - offset, offset};
    QPointF p1{imageW - offset, imageH - offset};
    QPointF p2{offset, imageH / 2.f};
    QPainterPath path;
    path.moveTo(p2);
    path.lineTo(p1);
    path.lineTo(p0);
    path.closeSubpath();
    QColor fillColor = color;
    fillColor.setAlphaF(0.25f);
    painter.fillPath(path, QBrush(fillColor));

    painter.drawPath(path);
    painter.end();

    return image;
}
} // anonymous namespace

ExampleNode1::ExampleNode1() {
    setMaterial(std::make_unique<EndOfTrackMaterial>());
    setGeometry(std::make_unique<Geometry>(EndOfTrackMaterial::attributes(), 4));

    geometry().setAttributeValues(0, positionArray, 4);
    geometry().setAttributeValues(1, horizontalGradientArray, 4);

    QColor color("red");
    material().setUniform(0,
            QVector4D{color.redF(),
                    color.greenF(),
                    color.blueF(),
                    color.alphaF()});
}

ExampleNode2::ExampleNode2() {
    setMaterial(std::make_unique<EndOfTrackMaterial>());
    setGeometry(std::make_unique<Geometry>(EndOfTrackMaterial::attributes(), 4));

    geometry().setAttributeValues(0, positionArray, 4);
    geometry().setAttributeValues(1, horizontalGradientArray, 4);

    QColor color("blue");
    material().setUniform(0,
            QVector4D{color.redF(),
                    color.greenF(),
                    color.blueF(),
                    color.alphaF()});
}

ExampleNode3::ExampleNode3() {
    setMaterial(std::make_unique<TextureMaterial>());
    setGeometry(std::make_unique<Geometry>(TextureMaterial::attributes(), 4));

    geometry().setAttributeValues(0, positionArray, 4);
    geometry().setAttributeValues(1, texcoordArray, 4);

    QMatrix4x4 matrix;

    matrix.scale(0.3);
    material().setUniform(0, matrix);
}

void ExampleNode3::setTexture(std::unique_ptr<Texture> texture) {
    dynamic_cast<TextureMaterial&>(material()).setTexture(std::move(texture));
}

ExampleNode4::ExampleNode4() {
    setMaterial(std::make_unique<PatternMaterial>());
    setGeometry(std::make_unique<Geometry>(PatternMaterial::attributes(), 0));
    geometry().setDrawingMode(Geometry::DrawingMode::Triangles);
}

void ExampleNode4::createTexture(rendergraph::Context& context) {
    dynamic_cast<PatternMaterial&>(material())
            .setTexture(std::make_unique<Texture>(context,
                    drawPrerollImage(40,
                            60,
                            1.0,
                            QColor(0, 255, 0))));
    geometry().allocate(6);
    TexturedVertexUpdater vertexUpdater{geometry().vertexDataAs<Geometry::TexturedPoint2D>()};

    vertexUpdater.addRectangle({400, 15},
            {0, 45},
            {0.f, 0.f},
            {10.0f, 1.f});

    QMatrix4x4 matrix;
    matrix.ortho(QRectF(0.0f,
            0.0f,
            1280.f,
            1920.f));
    material().setUniform(0, matrix);
}

ExampleTopNode::ExampleTopNode(rendergraph::Context& context) {
    TreeNode::appendChildNode(std::make_unique<rendergraph::ExampleNode1>());
    TreeNode::appendChildNode(std::make_unique<rendergraph::ExampleNode2>());
    TreeNode::appendChildNode(std::make_unique<rendergraph::ExampleNode3>());

    {
        QImage img(":/example/images/test.png");
        static_cast<rendergraph::ExampleNode3*>(TreeNode::lastChild())
                ->setTexture(
                        std::make_unique<rendergraph::Texture>(context, img));
    }

    TreeNode::appendChildNode(std::make_unique<rendergraph::ExampleNode4>());
    static_cast<rendergraph::ExampleNode4*>(TreeNode::lastChild())
            ->createTexture(context);
}
