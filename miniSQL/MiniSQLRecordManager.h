#pragma once
#include "MiniSQLBufferManager.h"
#include "MiniSQLCatalogManager.h"
#include "MiniSQLException.h"
#include <vector>
#include <set>
#include <map>
#include <string>
using namespace std;

class RecordManager {
public:
    RecordManager(BufferManager *buffer) : buffer(buffer) {}

	void createTable(const string &tablename);
	void dropTable(const string &tablename);
	ReturnTable selectRecord(const string &filename, const Table &table, const Predicate &pred);
	void deleteRecord(const Position &pos);
	void insertRecord(const string &filename, const Table &table, const Record &record);
private:
	//����������ļ��ж��ٿ�
	int getBlockNum(const Table &table);
	//����һ����¼�ж���λ������valid bit��
	int getRecordLength(const Table &table);
	//�жϼ�¼�Ƿ��������
	bool isFit(const Value &v, const Predicate &pred, const string &attr);
	//���ɷ��������ļ�¼
	Record AddRecord(char *q, Table table);
	
	BufferManager *buffer;

};
