# ASTree
Abstract syntax tree (AST), or just syntax tree. C++ realization

If you would like to write a language parser, you need to store operations by some way.
One of way is to use ASTree. 
I wrote my own realization for using in my future project. 
This realization based on smart pointers (std::shared_ptr), so you don't need to release memory by yourself. 

Try it and you will see, this is convenient!

#Little Example
```cpp
    enum ASTNodeType{
        Default, Head, Add, Mult, Power, Number
    };

    using ASTNode = ASTree::ASTNode<ASTNodeType>;


    // create root node
    auto syntaxTree = ASTNode::GetNewInstance(ASTNodeType::Head, "HEAD_NODE");

    const auto leftOp = ASTNode::GetNewInstance(ASTNodeType::Number, "2");
    const auto rightOp = ASTNode::GetNewInstance(ASTNodeType::Number, "5");

    // add children to root
    syntaxTree->addChild(ASTNode::GetNewInstance(ASTNodeType::Power, "POWER_NODE", leftOp, rightOp));
```