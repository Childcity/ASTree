//
// Created by childcity on 20.05.19.
//

#include "../src/astree.hpp"

#include <iostream>

int main()
{
    enum ASTNodeType{
        Default, Add, Mult, Power, Cycle
    };

    using ASTNode = ASTree::ASTNode<ASTNodeType>;



    
    auto syntaxTree = ASTNode::GetNewInstance(ASTNodeType::Add, "ADD_NODE");

    syntaxTree->addChild(ASTNode::GetNewInstance(ASTNodeType::Mult, "POWER_NODE"));

    auto mult = ASTNode::GetNewInstance(ASTNodeType::Mult, "MULT_NODE");
    syntaxTree->addChild(mult);
    syntaxTree->addChild(mult); // при повторном добавлении одного и того же узла узел сначала
                                // удалиться, а затем снова добавиться к syntaxTree




    // Найдем индекс узла mult в родительском узле
    const auto foundSelfInParentResult = mult->getChildIndexInParent();
    if(foundSelfInParentResult.first){
        std::cout <<"Found at index: " <<foundSelfInParentResult.second <<std::endl;
    }else{
        std::cout <<"Not found: " <<foundSelfInParentResult.second <<std::endl;
    }


    // Найдем индекс узла add в узле syntaxTree
    const auto foundChildResult = syntaxTree->getChildIndex(mult);
    if(foundChildResult.first){
        std::cout <<"Found child at index: " <<foundChildResult.second <<std::endl;
        const auto child = syntaxTree->getChild(foundChildResult.second);
        std::cout <<"Child contents: " <<child->toString() <<std::endl;
    }else{
        std::cout <<"Not found: " <<foundChildResult.second <<std::endl;
    }

    return 0;
}