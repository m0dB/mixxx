#include "shaders/shader.h"

#include <rhi/qshader.h>

#include <QFile>

#include "util/assert.h"

using namespace mixxx;

Shader::Shader() = default;

Shader::~Shader() = default;

namespace {
QString resource(const QString& filename) {
    return QStringLiteral(":/shaders/rendergraph/%1.qsb").arg(filename);
}

QByteArray loadShaderCodeFromFile(const QString& path) {
    QFile file(path);
    file.open(QIODeviceBase::ReadOnly);
    QShader qsbShader = QShader::fromSerialized(file.readAll());
    QShaderKey key(QShader::GlslShader, 120);
    return qsbShader.shader(key).shader();
}
} // namespace

void Shader::load(const QString& vertexShaderCode, const QString& fragmentShaderCode) {
    VERIFY_OR_DEBUG_ASSERT(addShaderFromSourceCode(
            QOpenGLShader::Vertex, vertexShaderCode)) {
        return;
    }

    VERIFY_OR_DEBUG_ASSERT(addShaderFromSourceCode(
            QOpenGLShader::Fragment, fragmentShaderCode)) {
        return;
    }

    VERIFY_OR_DEBUG_ASSERT(link()) {
        return;
    }
}

void Shader::loadRendergraphFiles(const QString& vertexShaderFilename,
        const QString& fragmentShaderFilename) {
    const QString vertexShaderFileFullPath = resource(vertexShaderFilename);
    const QString fragmentShaderFileFullPath = resource(fragmentShaderFilename);

    addShaderFromSourceCode(QOpenGLShader::Vertex,
            loadShaderCodeFromFile(vertexShaderFileFullPath));
    addShaderFromSourceCode(QOpenGLShader::Fragment,
            loadShaderCodeFromFile(fragmentShaderFileFullPath));

    link();
}
