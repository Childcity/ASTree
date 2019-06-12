#ifndef ASTREE_H
#define ASTREE_H

#include <memory>
#include <list>
#include <type_traits>
#include <algorithm> //std::next

//#define DEBUGAST(msg)
#include <iostream>
#define DEBUGAST(msg) std::cout <<__LINE__ <<"\t[" <<__func__ <<"]\t" <<msg <<std::endl;

namespace ASTree {

/*  AST-дерево представляет собой структурное
    представление исходной программы, очищенное от элементов конкретного
    синтаксиса (в рассматриваемом примере в AST-дерево не попал «разделитель»,
    т.к. он не имеет отношения непосредственно к семантике данного фрагмента
    программы, а лишь к конкретному синтаксису языка). В качестве узлов в AST-
    дереве выступают операторы, к которым присоединяются их аргументы,
    которые в свою очередь также могут быть составными узлами. Часто узлы
    AST-дерева получаются из лексем, выделенных на этапе лексического анализа,
    однако могут встречаться и узлы, которым ни одна лексема не соответствует
*/

static int GlobalIdForASTNode = 0;

template <typename NodeType, typename = typename std::enable_if< std::is_enum<NodeType>::value >::type>
class ASTNode: public std::enable_shared_from_this<ASTNode<NodeType>> {
public:
    using WeakPtr = std::weak_ptr<ASTNode<NodeType>>;
    using SharedPtr = std::shared_ptr<ASTNode<NodeType>>;

private:
    explicit ASTNode() noexcept
        : ASTNode(static_cast<NodeType>(0), std::string())
    {}

    explicit ASTNode(NodeType type, std::string text) noexcept
        : type_(std::move(type))
        , text_(std::move(text))
        , parent_(SharedPtr())
    {
        DEBUGAST(std::string("ASTNode(): '") <<toString())
        GlobalIdForASTNode++;
    }

    // следующая структура необходима для того, чтобы сработал std::make_shared с приватным
    // конструктором ASTNode()
    template <typename NodeType_>
    struct MakeSharedEnabler: public ASTNode<NodeType_> {
        explicit MakeSharedEnabler(NodeType_ type, std::string text)
                : ASTNode(std::move(type), std::move(text)){}
    };

public:

    ASTNode(const ASTNode &) = delete;

    ASTNode(ASTNode &&rVal) noexcept = delete;

    ~ASTNode(){
        DEBUGAST(std::string("~ASTNode(): '") <<toString())
    }

    static SharedPtr GetNewInstance(NodeType type, std::string text, const SharedPtr &child1, const SharedPtr &child2){
        const SharedPtr instance(std::make_shared<MakeSharedEnabler<NodeType>>(std::move(type), std::move(text)));
        DEBUGAST("creating instance")
        if(child1){
            DEBUGAST("child ok 1")
            instance->addChild(child1);
        }
        if(child2){
            DEBUGAST("child ok 2")
            instance->addChild(child2);
        }
        return instance;
    }

    static SharedPtr GetNewInstance(NodeType type, const SharedPtr &child1, const SharedPtr &child2){
        return GetNewInstance(std::move(type), std::to_string(type), child1, child2);
    }

    static SharedPtr GetNewInstance(NodeType type, std::string text, const SharedPtr &child1){
        return GetNewInstance(std::move(type), std::move(text), child1, SharedPtr());
    }

    static SharedPtr GetNewInstance(NodeType type, const SharedPtr &child1){
        return GetNewInstance(std::move(type), std::to_string(type), child1);
    }

    static SharedPtr GetNewInstance(NodeType type, std::string text){
        return GetNewInstance(std::move(type), std::move(text), SharedPtr(), SharedPtr());
    }

    static SharedPtr GetNewInstance(NodeType type){
        return GetNewInstance(std::move(type), std::to_string(static_cast<int>(type)));
    }

    void addChild(const SharedPtr child){
        DEBUGAST("adding child: " <<child->toString() <<" to root: " <<toString())

        if (child->isParentValid()) {
            DEBUGAST("Parent valid in child: "<<child->toString())
            child->getParent().lock()->removeChild(child);
        }

        DEBUGAST("Count of childs in " <<toString() <<" " <<childs_.size())
        auto self = this->shared_from_this(); //shared_from_this() we cannot use here,
                                    //because this object has not been initialized
                                    //yet and there isn't any shared_ptr that point to it
        child->setParent(self);

        childs_.remove(child); //если уже есть child в списке, удалить его
        childs_.emplace_back(std::move(child));
    }

    void removeChild(const SharedPtr &child){
        childs_.remove(child);

        // если текущий узел является отцом у дочернего, нужно это убрать, сделав отцом пустой SharedPtr
        if(child->getParent().lock() == this->shared_from_this()){
            child->setParent(SharedPtr());
        }
    }

    size_t getChildsCount() const { return childs_.size(); }

    bool isParentValid () const {
        //return (! parent_.owner_before(WeakPtr{})) && (! WeakPtr{}.owner_before(parent_));
        return parent_.lock() != nullptr;
    }

    WeakPtr getParent() { return parent_; }

    // сделать отцом parent
    void setParent( const WeakPtr &parent) { parent_ = parent; }

    // сделаться отцом у val
    void setAsParent(const SharedPtr &val) {
        auto self = this->shared_from_this();
        val->addChild(self);
    }

    SharedPtr getChild(long index) const {
        return *(std::next(childs_.begin(), index));
    }

    std::pair<bool, long> getChildIndex(const SharedPtr &child) const {
        // Find given element in list
        const auto &it = std::find(childs_.begin(), childs_.end(), child);

        if (it == childs_.end()) {
            return std::pair(false, -1);
        }

        return std::pair(true, std::distance(childs_.begin(), it));
    }

    std::pair<bool, long> getChildIndexInParent() {
        if(! isParentValid()){
            return std::pair(false, -1);
        }

        const auto self = this->shared_from_this();
        return parent_.lock()->getChildIndex(self);
    }

    std::list<SharedPtr> getChilds() const { return childs_; }

    std::string getUniqueName() const { return uniqName_; }

    NodeType getType() const { return type_; }

    std::string getText() const { return text_; }

    std::string toString() const { return text_ + " (type: " + std::to_string(type_) + ")"; }

private:

    // уникальное имя узла
    std::string uniqName_ = "node_" + std::to_string(GlobalIdForASTNode);
    
    // тип узла
     NodeType type_;

     // текст, связанный с узлом
     std::string text_;

     // родительский узел для данного узла дерева
     WeakPtr parent_;

     // потомки (ветви) данного узла дерева
     std::list<SharedPtr> childs_;
};

//class ASTNodeWalker{
//public:
//
//    static void ShowASTTree(const ASTNode::SharedPtr &root, QGraphicsScene &scene,  QGraphicsView &view){
//
//        QByteArray ASTTreeSource;
//        QTextStream stream(&ASTTreeSource, QIODevice::ReadWrite);
//        stream << "graph {" << endl;
//        stream << "\tnode[fontsize=10,margin=0,width=\".4\", height=\".3\"];" << endl;
//        stream << "\tsubgraph cluster17{" << endl;
//
//        GetStringSubTree(root, stream);
//
//        stream << "\t}\n}" << endl;
//        stream.flush();
//
//        QProcess* p = new QProcess();
//
//        p->setProcessChannelMode(QProcess::MergedChannels);
//        p->start("dot", std::stringList() << "-Tpng");
//        p->write(ASTTreeSource);
//
//        QByteArray data;
//        QPixmap pixmap = QPixmap();
//
//        while(p->waitForReadyRead(200)){
//            data.append(p->readAll());
//        }
//
//        pixmap.loadFromData(data);
//
//        scene.addPixmap(pixmap);
//        view.show();
//    }
//
//private:
//    static void GetStringSubTree(const ASTNode::SharedPtr &node, QTextStream &stream){
//
//        if(! node->getParent().lock()){
//            stream << "\t\tn" << node->getUniqueName() << "[label=\"" << node->getText() <<"\"];" << endl;
//
//        }else {
//            stream << "\t\tn" << node->getUniqueName() << "[label=\"" << node->getText() <<"\"];" << endl;
//            stream << "\t\tn" <<node->getParent().lock()->getUniqueName() << "--" <<"n" <<node->getUniqueName() << ";" << endl;
//        }
//
//        for (int i = 0; i < node->getChildsCount(); i++) {
//            GetStringSubTree(node->getChild(i), stream);
//        }
//    }
//
//};


}

#endif // ASTREE_H
