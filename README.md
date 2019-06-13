# ASTree
Abstract syntax tree (AST), or just syntax tree. C++ realization

If you would like to write a language parser, you need to store operations by some way.
One of way is to use ASTree. 
I wrote my own realization for using in my future project. 
This realization based on smart pointers (std::shared_ptr), so you don't need to release memory by yourself. 

Try it and you will see, this is convenient!

# Little Example for creating ASTree(full in ./example/main.cpp)
```cpp
    #include "astree.hpp"
    
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

# Little Example for showing ASTree(full in ./example/main.cpp)
```cpp
    #include "astree.hpp"

    /* create syntaxTree here */

    using ASTNodeWalker = ASTree::ASTNodeWalker<ASTNodeType>;
    ASTNodeWalker walker(syntaxTree);
    walker.buildDotFormat();

    std::cout <<walker.getDotFormat() <<std::endl;

    // write to file for dot utility
    std::ofstream{"dot_format.txt", std::ios::trunc | std::ios::out} << walker.getDotFormat();

    // run dot utility, that generate PNG image from dot_format.txt and save it to 'current directory'
    system("dot -O -Tpng dot_format.txt");
    
    
    /* Produce next output
     *   graph {
     *       node[fontsize=10,margin=0,width=".4", height=".3"];
     *       subgraph cluster17{
     *           nnode_0[label="HEAD_NODE"];
     *           nnode_3[label="POWER_NODE"];
     *           nnode_0--nnode_3;
     *           nnode_1[label="2"];
     *           nnode_3--nnode_1;
     *           nnode_2[label="5"];
     *           nnode_3--nnode_2;
     *           nnode_6[label="MULT_NODE"];
     *           nnode_0--nnode_6;
     *           nnode_5[label="7"];
     *           nnode_6--nnode_5;
     *           nnode_4[label="8"];
     *           nnode_6--nnode_4;
     *       }
     *   }
     */
```
