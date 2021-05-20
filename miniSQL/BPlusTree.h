#pragma once
#include "BPlusTree.cpp"

/*                 前向声明                 */

template<typename KeyType, typename DataType, int rank>
class BPlusInternalNode;

/*                                          */
/*                                          */
/*                  异常                    */
/*                                          */
/*                                          */

enum class BPlusTreeException { DuplicateKey };

/*                                          */
/*                                          */
/*                B+树结点                  */
/*                                          */
/*                                          */

template<typename KeyType, typename DataType, int rank>
class BPlusNode {
public:
    BPlusNode() : keyNum(0), parent(nullptr) {}
    ~BPlusNode() = default;
    BPlusInternalNode<KeyType, DataType, rank> *getParent() { return parent; }
    void setParent(BPlusInternalNode<KeyType, DataType, rank> *parent) { this->parent = parent; }
    virtual bool findData(const KeyType &guideKey) const = 0;
    virtual void insertData(const KeyType &newKey, const DataType &newData) = 0;
    virtual void splitNode() = 0;
    virtual void print() const = 0;

protected:
    int keyNum;
    KeyType key[rank + 1];
    BPlusInternalNode<KeyType, DataType, rank> *parent;
};

template<typename KeyType, typename DataType, int rank>
class BPlusLeafNode : public BPlusNode<KeyType, DataType, rank> {
public:
    BPlusLeafNode() : BPlusNode<KeyType, DataType, rank>(), prevLeaf(nullptr), nextLeaf(nullptr) {}
    ~BPlusLeafNode() = default;

    bool findData(const KeyType &guideKey) const override;
    void insertData(const KeyType &newKey, const DataType &newData) override;
    void splitNode() override;
    void print() const override { 
        std::cout << this->keyNum << "-Leaf:";
        for (int i = 0; i < this->keyNum; i++) std::cout << "[" << this->key[i] << "," << data[i] << "]";
        std::cout << std::endl;
    }

private:
    BPlusLeafNode<KeyType, DataType, rank> *nextLeaf;
    BPlusLeafNode<KeyType, DataType, rank> *prevLeaf;
    DataType data[rank + 1];
};

template<typename KeyType, typename DataType, int rank>
class BPlusInternalNode : public BPlusNode<KeyType, DataType, rank> {
public:
    BPlusInternalNode(BPlusNode<KeyType, DataType, rank> *firstChild) : BPlusNode<KeyType, DataType, rank>() { child[0] = firstChild; firstChild->setParent(this); }
    ~BPlusInternalNode() = default;

    void addKey(const KeyType &newKey, BPlusNode<KeyType, DataType, rank> *newChild);
    int findNextPath(const KeyType &guideKey) const;

    bool findData(const KeyType &guideKey) const override;
    void insertData(const KeyType &newKey, const DataType &newData) override;
    void splitNode() override;
    void print() const override {
        std::cout << this->keyNum << "-Internal:";
        for (int i = 0; i < this->keyNum; i++) std::cout << "[" << this->key[i] << "]";
        std::cout << std::endl;
        for (int i = 0; i <= this->keyNum; i++) child[i]->print();
    }

private:
    BPlusNode<KeyType, DataType, rank> *child[rank + 1];
};

/*                                          */
/*                                          */
/*            B+树叶子结点实现              */
/*                                          */
/*                                          */

template<typename KeyType, typename DataType, int rank>
bool BPlusLeafNode<KeyType, DataType, rank>::findData(const KeyType &guideKey) const {
    int left = 0, right = this->keyNum - 1;
    while (left <= right) {
        int mid = (left + right) / 2;
        if (guideKey == this->key[mid]) return true;
        else if (guideKey < this->key[mid]) right = mid - 1;
        else left = mid + 1;
    }
    return false;
}

template<typename KeyType, typename DataType, int rank>
void BPlusLeafNode<KeyType, DataType, rank>::insertData(const KeyType &newKey, const DataType &newData) {
    if (findData(newKey)) throw BPlusTreeException::DuplicateKey;
    int i;
    for (i = this->keyNum; i > 0 && this->key[i - 1] > newKey; i--) {
        this->key[i] = this->key[i - 1];
        data[i] = data[i - 1];
    }
    this->key[i] = newKey;
    data[i] = newData;

    if (++this->keyNum > rank) splitNode();
}

template<typename KeyType, typename DataType, int rank>
void BPlusLeafNode<KeyType, DataType, rank>::splitNode() {
    int leftKeyNum = this->keyNum / 2;
    BPlusLeafNode *newNode = new BPlusLeafNode();

    if (this->nextLeaf) this->nextLeaf->prevLeaf = newNode;
    newNode->nextLeaf = this->nextLeaf;
    newNode->prevLeaf = this;
    this->nextLeaf = newNode;

    for (int i = leftKeyNum; i < this->keyNum; i++) newNode->insertData(this->key[i], data[i]);
    this->keyNum = leftKeyNum;
    if (!this->parent) this->parent = new BPlusInternalNode<KeyType, DataType, rank>(this);
    newNode->parent = this->parent;
    this->parent->addKey(this->key[leftKeyNum], newNode);
}

/*                                          */
/*                                          */
/*            B+树内部结点实现              */
/*                                          */
/*                                          */

template<typename KeyType, typename DataType, int rank>
void BPlusInternalNode<KeyType, DataType, rank>::addKey(const KeyType &newKey, BPlusNode<KeyType, DataType, rank> *newChild) {
    int i;
    for (i = this->keyNum; i > 0 && this->key[i - 1] > newKey; i--) {
        this->key[i] = this->key[i - 1];
        child[i + 1] = child[i];
    }
    this->key[i] = newKey;
    child[i + 1] = newChild;
    this->keyNum++;
}

template<typename KeyType, typename DataType, int rank>
int BPlusInternalNode<KeyType, DataType, rank>::findNextPath(const KeyType &guideKey) const {
    int left = 0, right = this->keyNum - 1;
    while (left != right) {
        int mid = (left + right) / 2;
        if (guideKey < this->key[mid]) right = mid;
        else left = mid + 1;
    }
    if (guideKey >= this->key[right]) right++;
    return right;
}

template<typename KeyType, typename DataType, int rank>
bool BPlusInternalNode<KeyType, DataType, rank>::findData(const KeyType &guideKey) const {
    int next = findNextPath(guideKey);
    return child[next]->findData(guideKey);
}

template<typename KeyType, typename DataType, int rank>
void BPlusInternalNode<KeyType, DataType, rank>::insertData(const KeyType &newKey, const DataType &newData) {
    int next = findNextPath(newKey);
    child[next]->insertData(newKey, newData);

    if (this->keyNum >= rank) splitNode();
}

template<typename KeyType, typename DataType, int rank>
void BPlusInternalNode<KeyType, DataType, rank>::splitNode() {
    int leftKeyNum = this->keyNum / 2;
    BPlusInternalNode *newNode = new BPlusInternalNode(child[leftKeyNum + 1]);

    for (int i = leftKeyNum + 1; i < this->keyNum; i++) {
        newNode->addKey(this->key[i], child[i + 1]);
        child[i + 1]->setParent(newNode);
    }
    this->keyNum = leftKeyNum;
    if (!this->parent) this->parent = new BPlusInternalNode<KeyType, DataType, rank>(this);
    newNode->parent = this->parent;
    this->parent->addKey(this->key[leftKeyNum], newNode);
}

/*                                          */
/*                                          */
/*                  B+树                    */
/*                                          */
/*                                          */

template<typename KeyType, typename DataType, int rank>
class BPlusTree {
public:
    BPlusTree() : root(new BPlusLeafNode<KeyType, DataType, rank>()) {}
    ~BPlusTree();
    void insertData(KeyType newKey, const DataType &newData) {
        root->insertData(newKey, newData);
        if (root->getParent()) root = root->getParent();
    }
    bool findData(KeyType key) const { return root->findData(key); }
    //void removeData(KeyType key);
    //void updateData(KeyType key, DataType &data);
    void print() const { root->print(); }
private:

private:
    BPlusNode<KeyType, DataType, rank> *root;
};

template<typename KeyType, typename DataType, int rank>
BPlusTree<KeyType, DataType, rank>::~BPlusTree() {
    
}