#pragma once

#include <QRect>
#include <memory>
#include <vector>

#include "rendergraph/node.h"

namespace rendergraph {
class Engine;
} // namespace rendergraph

class rendergraph::Engine {
  public:
    Engine(std::unique_ptr<TreeNode> pNode);
    ~Engine();
    void initialize();
    void render();
    void resize(const QRectF&);
    void preprocess();
    void addToEngine(TreeNode* pNode);

  private:
    void initialize(TreeNode* pNode);
    void render(TreeNode* pNode);
    void resize(TreeNode* pNode, const QRectF&);

    // Cannot be const as it needs to be release on SceneGraph since the
    // ownership is transmitted to QSG
    std::unique_ptr<TreeNode> m_pTopNode;
    std::vector<TreeNode*> m_pPreprocessNodes;
    std::vector<TreeNode*> m_pInitializeNodes;
};
