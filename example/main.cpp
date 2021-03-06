//
// Created by childcity on 20.05.19.
//

#include "../src/astree.hpp"
#include <iostream>
#include <fstream>

int main()
{
    enum ASTNodeType{
        Default, Head, Add, Mult, Power, Number
    };

    using ASTNode = ASTree::ASTNode<ASTNodeType>;




    auto syntaxTree = ASTNode::GetNewInstance(ASTNodeType::Head, "HEAD_NODE");

    const auto leftOp = ASTNode::GetNewInstance(ASTNodeType::Number, "2");
    const auto rightOp = ASTNode::GetNewInstance(ASTNodeType::Number, "5");

    syntaxTree->addChild(ASTNode::GetNewInstance(ASTNodeType::Power, "POWER_NODE", leftOp, rightOp));

    auto mult = ASTNode::GetNewInstance(ASTNodeType::Mult, "MULT_NODE"
                                                           , ASTNode::GetNewInstance(ASTNodeType::Number, "7")
                                                            , ASTNode::GetNewInstance(ASTNodeType::Number, "8") );
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



    //////////////////////////////////  Отображение ASTree на png изображении  /////////////////////

    using ASTNodeWalker = ASTree::ASTNodeWalker<ASTNodeType>;
    ASTNodeWalker walker(syntaxTree);
    walker.buildDotFormat();

    std::cout <<walker.getDotFormat() <<std::endl;

    // запишем в файл dot_format.txt дерево в формате dot
    std::ofstream{"dot_format.txt", std::ios::trunc | std::ios::out} << walker.getDotFormat();

    // скормим сгенерированый файл утилите dot, которая сгенерирует png изображение
    system("dot -O -Tpng dot_format.txt");

    return 0;
}