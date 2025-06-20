#pragma once

#ifdef MIXXX_USE_QOPENGL
#include <QOpenGLShaderProgram>
#else
#include <QGLShaderProgram>
#endif

#include "util/class.h"

namespace mixxx {
class Shader;
}

class mixxx::Shader
#ifdef MIXXX_USE_QOPENGL
        : public QOpenGLShaderProgram {
#else
        : public QGLShaderProgram {
#endif
  public:
    Shader();
    ~Shader();

  protected:
    void load(const QString& fragmentShader, const QString& vertexShader);
    void loadRendergraphFiles(const QString& vertexShaderFilename,
            const QString& fragmentShaderFilename);

  private:
    DISALLOW_COPY_AND_ASSIGN(Shader)
};
