#include "astree.hpp"
#include "test_support.hpp"

#include <initializer_list>
#include <iostream>
#include <string>
#include <type_traits>

namespace {

enum NodeType { Root, Branch, Leaf };
using Node = ASTree::ASTNode<NodeType>;
using Walker = ASTree::ASTNodeWalker<NodeType>;
using test::require;

static_assert(!std::is_copy_constructible_v<Node>, "Nodes must not be copy constructed");
static_assert(!std::is_copy_assignable_v<Node>, "Nodes must not be copy assigned");
static_assert(std::is_signed_v<Node::Index>, "Node indices must support the -1 sentinel");

void requireNode(const Node::SharedPtr &node, NodeType type, const std::string &text,
                 std::initializer_list<Node::SharedPtr> children = {}) {
    require(node != nullptr, "The factory must return a node");
    require(node->getType() == type, "The factory must preserve the requested type");
    require(node->getText() == text, "The factory must preserve or generate the requested text");
    require(node->getChildsCount() == children.size(),
            "The child count must match the factory input");
    Node::Index index = 0;
    for (const auto &child : children) {
        require(node->getChild(index++) == child, "Factories must preserve child order");
        require(child->getParent().lock() == node, "Factories must set the children's parent");
    }
}

void testFactories() {
    auto leaf = Node::GetNewInstance(Leaf);
    requireNode(leaf, Leaf, "2");
    require(leaf->toString() == "2 (type: 2)", "Default node text must appear in toString");

    auto namedLeaf = Node::GetNewInstance(Leaf, "");
    requireNode(namedLeaf, Leaf, "");
    require(namedLeaf->toString() == " (type: 2)", "Explicit empty text must be preserved");

    auto branch = Node::GetNewInstance(Branch, leaf);
    requireNode(branch, Branch, "1", {leaf});
    auto namedBranch = Node::GetNewInstance(Branch, "branch", namedLeaf);
    requireNode(namedBranch, Branch, "branch", {namedLeaf});

    auto root = Node::GetNewInstance(Root, branch, namedBranch);
    requireNode(root, Root, "0", {branch, namedBranch});
    auto otherLeaf = Node::GetNewInstance(Leaf, "right");
    auto namedRoot = Node::GetNewInstance(Root, "root", root, otherLeaf);
    requireNode(namedRoot, Root, "root", {root, otherLeaf});
    require(namedRoot->toString() == "root (type: 0)", "Explicit text must appear in toString");
}

void testNullableFactoryChildren() {
    const Node::SharedPtr empty;
    requireNode(Node::GetNewInstance(Root, empty), Root, "0");
    requireNode(Node::GetNewInstance(Root, "empty", empty), Root, "empty");
    requireNode(Node::GetNewInstance(Root, empty, empty), Root, "0");
    requireNode(Node::GetNewInstance(Root, "empty", empty, empty), Root, "empty");

    auto child = Node::GetNewInstance(Leaf, "leaf");
    auto firstOnly = Node::GetNewInstance(Root, "first", child, empty);
    requireNode(firstOnly, Root, "first", {child});
    auto secondOnly = Node::GetNewInstance(Root, empty, child);
    requireNode(secondOnly, Root, "0", {child});
    require(firstOnly->getChildsCount() == 0,
            "Factories must detach a child from its previous parent");

    auto duplicated = Node::GetNewInstance(Root, "duplicates", child, child);
    requireNode(duplicated, Root, "duplicates", {child});
    require(secondOnly->getChildsCount() == 0, "Moving the child must update its previous parent");
}

void testChildAccessAndIndices() {
    auto parent = Node::GetNewInstance(Root, "root");
    auto first = Node::GetNewInstance(Leaf, "first");
    auto second = Node::GetNewInstance(Leaf, "second");
    auto absent = Node::GetNewInstance(Leaf, "absent");
    require(parent->getChilds().empty(), "A new node must have an empty child list");
    require(parent->getChildIndex(first) == std::make_pair(false, Node::Index{-1}),
            "Lookup in an empty child list must return the missing sentinel");
    parent->addChild(first);
    parent->addChild(second);

    require(parent->getChild(0) == first && parent->getChild(1) == second,
            "Indexed access must preserve insertion order");
    require(parent->getChildIndex(first) == std::make_pair(true, Node::Index{0}),
            "The first child's index must be zero");
    require(parent->getChildIndex(second) == std::make_pair(true, Node::Index{1}),
            "The last child's index must be found");
    require(parent->getChildIndex(absent) == std::make_pair(false, Node::Index{-1}),
            "Lookup of a missing child must return the missing sentinel");
    require(second->getChildIndexInParent() == std::make_pair(true, Node::Index{1}),
            "A child must be able to find its index in its parent");
    require(parent->getChildIndexInParent() == std::make_pair(false, Node::Index{-1}),
            "A root must report that it has no index in a parent");

    auto snapshot = parent->getChilds();
    require(snapshot == std::list<Node::SharedPtr>{first, second},
            "getChilds must return all children in order");
    snapshot.clear();
    require(parent->getChildsCount() == 2, "Modifying a child list copy must not modify the node");
}

void testReparentingAndDuplicates() {
    auto firstParent = Node::GetNewInstance(Root, "first parent");
    auto secondParent = Node::GetNewInstance(Root, "second parent");
    auto first = Node::GetNewInstance(Leaf, "first");
    auto second = Node::GetNewInstance(Leaf, "second");
    firstParent->addChild(first);
    firstParent->addChild(second);
    firstParent->addChild(first);
    requireNode(firstParent, Root, "first parent", {second, first});
    require(first->getChildIndexInParent() == std::make_pair(true, Node::Index{1}),
            "Adding an existing child must move it to the end without duplicating it");

    secondParent->addChild(first);
    requireNode(firstParent, Root, "first parent", {second});
    requireNode(secondParent, Root, "second parent", {first});
    require(firstParent->getChildIndex(first) == std::make_pair(false, Node::Index{-1}),
            "A reparented child must no longer be found in its old parent");
}

void testRemoval() {
    auto parent = Node::GetNewInstance(Root, "root");
    auto child = Node::GetNewInstance(Leaf, "child");
    auto foreignParent = Node::GetNewInstance(Root, "foreign");
    auto foreignChild = Node::GetNewInstance(Leaf, "foreign child");
    parent->addChild(child);
    foreignParent->addChild(foreignChild);

    parent->removeChild(foreignChild);
    requireNode(parent, Root, "root", {child});
    requireNode(foreignParent, Root, "foreign", {foreignChild});

    parent->removeChild(child);
    require(parent->getChildsCount() == 0, "Removing a child must update the parent's child list");
    require(child->getParent().expired(), "Removing a child must clear its parent pointer");
    require(child->getChildIndexInParent() == std::make_pair(false, Node::Index{-1}),
            "A removed child must no longer have an index in a parent");
    parent->removeChild(child);
    require(parent->getChildsCount() == 0, "Removing an absent child must be harmless");
}

void testParentAccess() {
    auto node = Node::GetNewInstance(Leaf, "node");
    auto firstParent = Node::GetNewInstance(Root, "first");
    auto secondParent = Node::GetNewInstance(Root, "second");
    require(!node->isParentValid() && node->getParent().expired(),
            "A new node must have no parent");

    node->setParent(firstParent);
    require(node->isParentValid(), "A live assigned parent must be valid");
    require(node->getParent().lock() == firstParent, "getParent must return the assigned parent");
    require(node->getChildIndexInParent() == std::make_pair(false, Node::Index{-1}),
            "An assigned parent without a matching child entry must return the missing sentinel");

    node->setParent(secondParent);
    require(node->getParent().lock() == secondParent,
            "setParent must replace the stored weak pointer");
    node->setParent(Node::WeakPtr{});
    require(!node->isParentValid(), "Clearing the weak parent pointer must invalidate the parent");

    node->setParent(secondParent);
    secondParent.reset();
    require(!node->isParentValid() && node->getParent().expired(),
            "A parent pointer must not own its node");
}

void testSetAsParent() {
    auto child = Node::GetNewInstance(Leaf, "child");
    auto firstParent = Node::GetNewInstance(Root, "first");
    auto secondParent = Node::GetNewInstance(Root, "second");
    child->setAsParent(firstParent);
    requireNode(firstParent, Root, "first", {child});
    child->setAsParent(secondParent);
    requireNode(firstParent, Root, "first");
    requireNode(secondParent, Root, "second", {child});
}

void testUniqueNames() {
    auto first = Node::GetNewInstance(Leaf, "same text");
    auto second = Node::GetNewInstance(Leaf, "same text");
    const auto name = first->getUniqueName();
    require(name.find("node_") == 0, "Node names must use the node_ prefix");
    require(name != second->getUniqueName(), "Different nodes must have different names");
    second->addChild(first);
    require(first->getUniqueName() == name, "Reparenting must preserve a node's name");
    const auto oldName = second->getUniqueName();
    second.reset();
    auto replacement = Node::GetNewInstance(Root, "replacement");
    require(replacement->getUniqueName() != oldName,
            "New nodes must not reuse a destroyed node's name");
}

void testLifetimes() {
    Node::WeakPtr weakChild;
    Node::WeakPtr weakLeaf;
    {
        auto root = Node::GetNewInstance(Root, "root");
        auto child = Node::GetNewInstance(Branch, "child");
        auto leaf = Node::GetNewInstance(Leaf, "leaf");
        weakChild = child;
        weakLeaf = leaf;
        root->addChild(child);
        child->addChild(leaf);
        child.reset();
        leaf.reset();
        require(!weakChild.expired() && !weakLeaf.expired(), "Parents must own their descendants");
    }
    require(weakChild.expired() && weakLeaf.expired(),
            "Destroying a tree must release its descendants");

    auto parent = Node::GetNewInstance(Root, "parent");
    auto child = Node::GetNewInstance(Leaf, "survivor");
    parent->addChild(child);
    parent.reset();
    require(!child->isParentValid(),
            "An externally owned child must survive with an expired parent");

    parent = Node::GetNewInstance(Root, "parent");
    parent->addChild(child);
    weakChild = child;
    child.reset();
    parent->removeChild(weakChild.lock());
    require(weakChild.expired(), "Removing the last owning reference must destroy the child");
}

std::size_t countOccurrences(const std::string &text, const std::string &fragment) {
    std::size_t count = 0;
    for (auto position = text.find(fragment); position != std::string::npos;
         position = text.find(fragment, position + fragment.size())) {
        ++count;
    }
    return count;
}

std::string nodeDeclaration(const Node::SharedPtr &node) {
    return "\t\tn" + node->getUniqueName() + "[label=\"" + node->getText() + "\"];\n";
}

std::string edgeDeclaration(const Node::SharedPtr &parent, const Node::SharedPtr &child) {
    return "\t\tn" + parent->getUniqueName() + "--n" + child->getUniqueName() + ";\n";
}

void testWalkerSingleNode() {
    auto root = Node::GetNewInstance(Root, "root");
    Walker walker(root);
    require(walker.getDotFormat().empty(), "DOT output must initially be empty");
    walker.buildDotFormat();
    const auto output = walker.getDotFormat();
    require(output.find("graph {\n") == 0, "DOT output must declare an undirected graph");
    require(countOccurrences(output, nodeDeclaration(root)) == 1, "The root must be declared once");
    require(output.find("--") == std::string::npos,
            "A single-node graph must not contain any edges");
    require(output.size() >= 5 && output.substr(output.size() - 5) == "\t}\n}\n",
            "DOT output must close the subgraph and graph");
    walker.buildDotFormat();
    require(walker.getDotFormat() == output, "Repeated builds must produce identical output");
}

void testWalkerTree() {
    auto root = Node::GetNewInstance(Root, "root");
    auto branch = Node::GetNewInstance(Branch, "branch");
    auto firstLeaf = Node::GetNewInstance(Leaf, "same label");
    auto secondLeaf = Node::GetNewInstance(Leaf, "same label");
    root->addChild(branch);
    branch->addChild(firstLeaf);
    root->addChild(secondLeaf);
    Walker walker(root);
    root.reset();
    root = branch->getParent().lock();
    require(root != nullptr, "The walker must keep its root alive");
    walker.buildDotFormat();
    const auto output = walker.getDotFormat();
    for (const auto &node : {root, branch, firstLeaf, secondLeaf}) {
        require(countOccurrences(output, nodeDeclaration(node)) == 1,
                "Each node must have exactly one DOT declaration");
    }
    require(countOccurrences(output, edgeDeclaration(root, branch)) == 1,
            "The root must connect to its branch");
    require(countOccurrences(output, edgeDeclaration(branch, firstLeaf)) == 1,
            "Nested edges must be traversed");
    require(countOccurrences(output, edgeDeclaration(root, secondLeaf)) == 1,
            "Sibling edges must be traversed");
    require(countOccurrences(output, "--") == 3,
            "DOT output must contain exactly the tree's edges");
}

void testWalkerRebuild() {
    auto root = Node::GetNewInstance(Root, "root");
    auto child = Node::GetNewInstance(Leaf, "child");
    Walker walker(root);
    walker.buildDotFormat();
    const auto originalOutput = walker.getDotFormat();
    root->addChild(child);
    require(walker.getDotFormat() == originalOutput, "DOT output must be a snapshot until rebuilt");
    walker.buildDotFormat();
    require(countOccurrences(walker.getDotFormat(), nodeDeclaration(child)) == 1,
            "Rebuilding must include new nodes");
    require(countOccurrences(walker.getDotFormat(), edgeDeclaration(root, child)) == 1,
            "Rebuilding must include new edges");
    root->removeChild(child);
    walker.buildDotFormat();
    require(walker.getDotFormat() == originalOutput,
            "Rebuilding must remove stale nodes and edges");
}

struct TestCase {
    const char *name;
    void (*run)();
};

const TestCase testCases[] = {
    {"factories", testFactories},
    {"nullable_factory_children", testNullableFactoryChildren},
    {"child_access_and_indices", testChildAccessAndIndices},
    {"reparenting_and_duplicates", testReparentingAndDuplicates},
    {"removal", testRemoval},
    {"parent_access", testParentAccess},
    {"set_as_parent", testSetAsParent},
    {"unique_names", testUniqueNames},
    {"lifetimes", testLifetimes},
    {"walker_single_node", testWalkerSingleNode},
    {"walker_tree", testWalkerTree},
    {"walker_rebuild", testWalkerRebuild},
};

} // namespace

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Expected one test case name.\n";
        return 1;
    }
    for (const auto &testCase : testCases) {
        if (testCase.name != std::string(argv[1])) {
            continue;
        }
        try {
            testCase.run();
            std::cout << testCase.name << " passed.\n";
            return 0;
        } catch (const std::exception &error) {
            std::cerr << testCase.name << " failed: " << error.what() << '\n';
            return 1;
        }
    }
    std::cerr << "Unknown test case: " << argv[1] << '\n';
    return 1;
}
