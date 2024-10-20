#pragma once

#include "backend/baseopacitynode.h"
#include "rendergraph/treenode.h"

namespace rendergraph {
class OpacityNode;
} // namespace rendergraph

class rendergraph::OpacityNode : public rendergraph::BaseOpacityNode,
                                 public rendergraph::TreeNode {
  public:
    using rendergraph::TreeNode::appendChildNode;
    using rendergraph::TreeNode::firstChild;
    using rendergraph::TreeNode::lastChild;
    using rendergraph::TreeNode::nextSibling;
    using rendergraph::TreeNode::removeChildNode;
    OpacityNode();
};
