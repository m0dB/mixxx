#pragma once

class QQuickWindow;

namespace rendergraph {
class Context;
}

class rendergraph::Context {
  public:
    QQuickWindow* window() const {
        return nullptr;
    }
};
