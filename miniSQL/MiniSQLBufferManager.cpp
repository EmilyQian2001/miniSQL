#include "MiniSQLBufferManager.h"
#include "MiniSQLException.h"
#include <iostream>

BufferManager::Page::Page() {
    filename = "";
    block_id = -1;//�ļ����ǵ�0�鿪ʼ
    dirty = false;
    pin = false;
    ref = false;
    empty = true;
    memset(buffer, 0, sizeof(char)*PAGESIZE);
}

//���캯��(��ʼ��ҳ����)
BufferManager::BufferManager(int page_num) {
    frame = new Page[page_num];
    this->page_num = page_num;
    replace_position = 0;
}

//��������:������ȫ��д�ش���
BufferManager::~BufferManager() {
    for (int i = 0; i < page_num; i++) {//ÿһҳ��д��ÿһ��
        if(frame[i].dirty) writeBackToDisk(i, frame[i].filename, frame[i].block_id);
    }
    delete[] frame;
}

//��ȡ�ļ��п��Ӧ���ڴ����ҳ��(û�ҵ��͵���������������һҳ)
int BufferManager::getPageID(const string &filename, int block_id) {
    auto id = nameID.find(make_pair(filename,block_id));
    if (nameID.end() != id) return (*id).second;
    
    //buffer������Ӧ��
    int page_id = getEmptyPage();
    loadBlockToPage(page_id, filename, block_id);
    return page_id;
}

//��ȡĳҳ�����ݣ�ֱ��ʹ���ļ�����
char* BufferManager::getBlockContent(const string &filename, int block_id) {
    int page_id = getPageID(filename, block_id);
    char* head = frame[page_id].buffer;
    frame[page_id].ref = true;
    return head;
}

//��ȡĳҳ�����ݣ�ʹ��ҳ�ţ�
char* BufferManager::getBlockContent(int page_id) {
    char* head = frame[page_id].buffer;
    frame[page_id].ref = true;
    return head;
}

//�޸�ĳҳ�����ݣ�ֱ��ʹ���ļ�����
void BufferManager::setBlockContent(const string &filename, int block_id, int offset, char* data, size_t length) {
    int page_id = getPageID(filename, block_id);
    if (frame[page_id].empty == true) throw MiniSQLException("Empty Page!");
    if (offset >= PAGESIZE) throw MiniSQLException("Write Page Out of range!");
    memcpy_s(frame[page_id].buffer + offset, PAGESIZE - offset, data, length);
    frame[page_id].dirty = true;
    frame[page_id].ref = true;
}

//�޸�ĳҳ�����ݣ�ʹ��ҳ�ţ�
void BufferManager::setBlockContent(int page_id, int offset, char* data, size_t length) {
    if (frame[page_id].empty == true) throw MiniSQLException("Empty Page!");
    if (offset >= PAGESIZE) throw MiniSQLException("Write Page Out of range!");
    memcpy_s(frame[page_id].buffer + offset, PAGESIZE - offset, data, length);
    frame[page_id].dirty = true;
    frame[page_id].ref = true;
}

//���ļ����¿�һ�飬���ض�Ӧ�Ŀ��
int BufferManager::allocNewBlock(const string &filename) {
    int page_id = getEmptyPage();

    FILE* fp;
    fopen_s(&fp, filename.c_str(), "rb+");
    if (fp == nullptr) throw MiniSQLException("Fail to open file!"); //���ļ�ʧ��
    fseek(fp, 0, SEEK_END);
    int block_id = ftell(fp) / PAGESIZE;
    char blank[PAGESIZE] = { 0 };
    fwrite(blank, sizeof(blank[0]), sizeof(blank), fp);
    fclose(fp);

    frame[page_id].filename = filename;
    frame[page_id].block_id = block_id;
    frame[page_id].dirty = false;
    frame[page_id].pin = false;
    frame[page_id].ref = true;
    frame[page_id].empty = false;
    nameID[make_pair(filename, block_id)] = page_id;

    return block_id;
}

//���ĳ�ļ���ص�����ҳ
void BufferManager::setEmpty(const string &filename) {
    for (int i = 0; i < page_num; i++) {
        if (frame[i].filename == filename) {
            auto it = nameID.find(make_pair(filename, frame[i].block_id));
            nameID.erase(it);
            frame[i] = Page();
        }
    }
}

//�̶�/����̶�
void BufferManager::setPagePin(int page_id, bool pin) {
    frame[page_id].pin = pin;
}

//ʱ���滻������һ������/���滻��ҳ,����page_id
int BufferManager::getEmptyPage() {
    for (int i = 0; i < page_num; i++) {
        if (frame[i].empty == true) return i;
    }
    //û�пյģ�����ʱ���滻����
    while (1) {
        if (frame[replace_position].ref == true)
            frame[replace_position].ref = false;
        else if (frame[replace_position].pin == false) {//û����ס
            string filename = frame[replace_position].filename;
            int block_id = frame[replace_position].block_id;
            if (frame[replace_position].dirty == true) {
                //д��
                writeBackToDisk(replace_position, filename, block_id);
                //��ո�ҳ���ݣ����³�ʼ����
                frame[replace_position] = Page();
            }
            auto it = nameID.find(make_pair(filename, block_id));
            nameID.erase(it);
            break;
        }
        replace_position = (replace_position + 1) % page_num;
    }
    return replace_position;
}
//���ļ��еĿ���ص��ڴ��һҳ��
void BufferManager::loadBlockToPage(int page_id, const string &filename, int block_id) {
    FILE* fp;
    fopen_s(&fp, filename.c_str(), "rb");
    if (fp == nullptr) throw MiniSQLException("Fail to open file!"); //���ļ�ʧ��

    //��λ�Ͷ�ȡ
    fseek(fp, sizeof(char) * PAGESIZE * block_id, SEEK_SET);
    char* head = frame[page_id].buffer;
    fread(head, sizeof(char), PAGESIZE, fp);
    fclose(fp);
    frame[page_id].filename = filename;
    frame[page_id].block_id = block_id;
    frame[page_id].dirty = false;
    frame[page_id].pin = false;
    frame[page_id].ref = true;
    frame[page_id].empty = false;
    nameID[make_pair(filename,block_id)] = page_id;
}

//��ҳд�ش���
void BufferManager::writeBackToDisk(int page_id, const string &filename, int block_id) {
    FILE* fp;
    fopen_s(&fp, filename.c_str(), "rb+");
    if (fp == nullptr) throw MiniSQLException("Fail to open file!"); //���ļ�ʧ��

    //��λ��д��
    fseek(fp, sizeof(char) * PAGESIZE * block_id, SEEK_SET);
    char* head = frame[page_id].buffer;
    fwrite(head, sizeof(char), PAGESIZE, fp);
    fclose(fp);
}

void BufferManager_test() {
    BufferManager BM = BufferManager();
    try {
        char *head = BM.getBlockContent("../test.txt", 2);
        //int newBlock = BM.allocNewBlock("../test.txt");
        //head = BM.getBlockContent("../test.txt", newBlock);
        std::cout << head;
        char mod[] = "abc";
        BM.setBlockContent("../test.txt", 2, 0, mod, sizeof(mod));
    } catch (MiniSQLException &e){
        std::cout << e.getMessage();
    }
}