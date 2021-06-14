#pragma once

#include <string>
#include <map>
using std::string;
using std::map;
using std::pair;

#define PAGESIZE 4096   //һҳ4KB
#define MAXPAGENUM 2 //���100ҳ

class BufferManager {
private:
    struct Page {
        Page();
        char buffer[PAGESIZE];//������
        string filename;//ӳ���ļ���
        int block_id;//ӳ����
        bool dirty;//�޸ı��
        bool pin;//�������
        bool ref;//ʹ�ñ�ǣ�ʱ���滻��
        bool empty;//�ձ��
    };

    //��̬����ҳ����
    int page_num;//ҳ��
    Page* frame;//�����׵�ַָ��
    map<pair<string,int>, int> nameID;
    int replace_position;//ʱ��ָ�루ʱ���滻��
public:
    BufferManager(int page_num = MAXPAGENUM);//���캯��(��ʼ��ҳ����)
    ~BufferManager();//��������

    //��ȡ�ļ��п��Ӧ���ڴ����ҳ��
    int getPageID(const string &filename, int block_id);

    //��ȡĳҳ������
    char* getBlockContent(const string &filename, int block_id);
    char* getBlockContent(int page_id);

    //�޸�ĳҳ������
    void setBlockContent(const string &filename, int block_id, int offset, char* data, size_t length);
    void setBlockContent(int page_id, int offset, char* data, size_t length);

    //���ļ����¿�һ�飬���ض�Ӧ��ҳ��
    int allocNewBlock(const string &filename);

    //���ĳ�ļ���ص�����ҳ
    void setEmpty(const string &filename);
    
    //�̶�/����̶�
    void setPagePin(int page_id, bool pin);

    //ʱ���滻������һ������/���滻��ҳ,����page_id
    int getEmptyPage();

    //���ļ��еĿ���ص��ڴ��һҳ��
    void loadBlockToPage(int page_id, const string &file_name, int block_id);

    //��ҳд�ش���
    void writeBackToDisk(int page_id, const string &file_name, int block_id);
};