#pragma once

#include <algorithm> // std::next
#include <cassert>
#include <sstream>   // std::stringstream
#include <memory>
#include <list>
#include <type_traits>

#define DEBUGAST(msg)
// #include <iostream>
// #define DEBUGAST(msg) std::cout << __LINE__ << "\t[" << __func__ << "]\t" << msg << std::endl;

namespace ASTree {

/*
 * An abstract syntax tree (AST) is a structural representation of a source
 * program with elements specific to its concrete syntax removed. For example,
 * a separator is omitted because it relates to syntax rather than the semantics
 * of the program fragment. AST nodes are operators with their arguments, which
 * may themselves be composite nodes. AST nodes are often derived from tokens
 * produced during lexical analysis, though some nodes may not correspond to any
 * token.
 */

static int GlobalIdForASTNode = 0; // Simple node ID counter for this translation unit.

template <typename NodeType,
          typename = typename std::enable_if<std::is_enum<NodeType>::value>::type>
class ASTNode : public std::enable_shared_from_this<ASTNode<NodeType>> {
  public:
    using WeakPtr = std::weak_ptr<ASTNode<NodeType>>;
    using SharedPtr = std::shared_ptr<ASTNode<NodeType>>;
    using Index = typename std::list<SharedPtr>::difference_type;

  private:
    explicit ASTNode() noexcept : ASTNode(static_cast<NodeType>(0), std::string()) {}

    explicit ASTNode(NodeType type, std::string text) noexcept
        : type_(std::move(type)), text_(std::move(text)), parent_(SharedPtr()) {
        DEBUGAST(std::string("ASTNode(): '") << toString())
        GlobalIdForASTNode++;
    }

    // This helper allows std::make_shared to call ASTNode's private constructor.
    template <typename NodeType_> struct MakeSharedEnabler : public ASTNode<NodeType_> {
        explicit MakeSharedEnabler(NodeType_ type, std::string text)
            : ASTNode(std::move(type), std::move(text)) {}
    };

  public:
    ASTNode(const ASTNode &) = delete;

    ASTNode(ASTNode &&) = delete;

    ~ASTNode() {
        DEBUGAST(std::string("~ASTNode(): '") << toString())
    }

    static SharedPtr GetNewInstance(NodeType type, std::string text, const SharedPtr &child1,
                                    const SharedPtr &child2) {
        const SharedPtr instance(
            std::make_shared<MakeSharedEnabler<NodeType>>(std::move(type), std::move(text)));
        if (child1) {
            instance->addChild(child1);
        }
        if (child2) {
            instance->addChild(child2);
        }
        return instance;
    }

    static SharedPtr GetNewInstance(NodeType type, const SharedPtr &child1,
                                    const SharedPtr &child2) {
        return GetNewInstance(std::move(type), std::to_string(type), child1, child2);
    }

    static SharedPtr GetNewInstance(NodeType type, std::string text, const SharedPtr &child1) {
        return GetNewInstance(std::move(type), std::move(text), child1, SharedPtr());
    }

    static SharedPtr GetNewInstance(NodeType type, const SharedPtr &child1) {
        return GetNewInstance(std::move(type), std::to_string(type), child1);
    }

    static SharedPtr GetNewInstance(NodeType type, std::string text) {
        return GetNewInstance(std::move(type), std::move(text), SharedPtr(), SharedPtr());
    }

    static SharedPtr GetNewInstance(NodeType type) {
        return GetNewInstance(std::move(type), std::to_string(type));
    }

    // Add a child node to this node.
    // Throw std::invalid_argument if the child is null or would create a cycle.
    void addChild(const SharedPtr child) {
        auto self = this->shared_from_this();
        for (auto ancestor = self; ancestor; ancestor = ancestor->parent_.lock()) {
            if (ancestor == child) {
                assert(false && "Adding this child would create a cycle. It is mistake");
                return;
            }
        }

        DEBUGAST("adding child: " << child->toString() << " to root: " << toString())

        if (child->isParentValid()) {
            DEBUGAST("Parent valid in child: " << child->toString())
            child->getParent().lock()->removeChild(child);
        }

        DEBUGAST("Count of childs in " << toString() << " " << childs_.size())
        child->setParent(self);

        childs_.remove(child); // Remove the child first if it is already in the list.
        childs_.emplace_back(std::move(child));
    }

    // Remove a child node from this node.
    void removeChild(const SharedPtr &child) {
        childs_.remove(child);

        // Clear the child's parent if this node is currently its parent.
        if (child->getParent().lock() == this->shared_from_this()) {
            child->setParent(SharedPtr());
        }
    }

    // Get the number of children of this node.
    size_t getChildsCount() const {
        return childs_.size();
    }

    // Check whether this node has a parent.
    bool isParentValid() const {
        // return (! parent_.owner_before(WeakPtr{})) && (! WeakPtr{}.owner_before(parent_));
        return parent_.lock() != nullptr;
    }

    // Get a weak pointer to this node's parent.
    WeakPtr getParent() {
        return parent_;
    }

    // Set the given node as this node's parent.
    void setParent(const WeakPtr &parent) {
        parent_ = parent;
    }

    // Make the given node the parent of this node.
    void setAsParent(const SharedPtr &val) {
        auto self = this->shared_from_this();
        val->addChild(self);
    }

    // Get a child node by index.
    SharedPtr getChild(Index index) const {
        return *(std::next(childs_.begin(), index));
    }

    // Get a child's index. The pair contains whether it was found and its index, or -1.
    std::pair<bool, Index> getChildIndex(const SharedPtr &child) const {
        // Find the given child in the list.
        const auto &it = std::find(childs_.begin(), childs_.end(), child);

        if (it == childs_.end()) {
            return std::pair(false, -1);
        }

        return std::pair(true, std::distance(childs_.begin(), it));
    }

    // Get this node's index among its parent's children, if it has a parent.
    // The pair contains whether it was found and its index, or -1.
    std::pair<bool, Index> getChildIndexInParent() {
        if (!isParentValid()) {
            return std::pair(false, -1);
        }

        const auto self = this->shared_from_this();
        return parent_.lock()->getChildIndex(self);
    }

    // Get all children of this node.
    std::list<SharedPtr> getChilds() const {
        return childs_;
    }

    // Get this node's unique name.
    std::string getUniqueName() const {
        return uniqName_;
    }

    // Get this node's type.
    NodeType getType() const {
        return type_;
    }

    // Get the text assigned to this node.
    std::string getText() const {
        return text_;
    }

    // Get a string representation of this node.
    std::string toString() const {
        return text_ + " (type: " + std::to_string(type_) + ")";
    }

  private:
    // The node's unique name.
    std::string uniqName_ = "node_" + std::to_string(GlobalIdForASTNode);

    // The node's type.
    NodeType type_;

    // Text associated with the node.
    std::string text_;

    // Parent node in the tree.
    WeakPtr parent_;

    // Child nodes (branches) of this node.
    std::list<SharedPtr> childs_;
};

template <typename NodeType,
          typename = typename std::enable_if<std::is_enum<NodeType>::value>::type>
class ASTNodeWalker {
  public:
    explicit ASTNodeWalker(std::shared_ptr<ASTNode<NodeType>> head) noexcept
        : head_(std::move(head)) {}

    // Generate Graphviz DOT output that can be used to render the tree.
    void buildDotFormat() {
        std::stringstream stream;
        stream << "graph {" << std::endl;
        stream << "\tnode[fontsize=10,margin=0,width=\".4\", height=\".3\"];" << std::endl;
        stream << "\tsubgraph cluster17{" << std::endl;

        ASTNodeWalker::GetStringSubTree(head_, stream);

        stream << "\t}\n}" << std::endl;
        dotFormat_ = stream.str();
    }

    std::string getDotFormat() const {
        return dotFormat_;
    }

  private:
    static void GetStringSubTree(const std::shared_ptr<ASTNode<NodeType>> &node,
                                 std::stringstream &stream) {

        if (!node->getParent().lock()) {
            stream << "\t\tn" << node->getUniqueName() << "[label=\"" << node->getText() << "\"];"
                   << std::endl;
        } else {
            stream << "\t\tn" << node->getUniqueName() << "[label=\"" << node->getText() << "\"];"
                   << std::endl;
            stream << "\t\tn" << node->getParent().lock()->getUniqueName() << "--n"
                   << node->getUniqueName() << ";" << std::endl;
        }

        for (int i = 0; i < node->getChildsCount(); i++) {
            GetStringSubTree(node->getChild(i), stream);
        }
    }

  private:
    const std::shared_ptr<ASTNode<NodeType>> head_;
    std::string dotFormat_;
};

} // namespace ASTree
