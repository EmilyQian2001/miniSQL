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
	ReturnTable selectRecord(const string &tablename, const Table &table, const Predicate &pred);
	void deleteRecord(const Position &pos);
	void insertRecord(const string &tablename, const Table &table, const Record &record);
private:
	//����������ļ��ж��ٿ�
	int getBlockNum(Table table);
	//����һ����¼�ж���λ������valid bit��
	int getRecordLength(Table table);
	//�жϼ�¼�Ƿ��������
	bool isFit(Value v, Predicate pred, const string &attr);
	//���ɷ��������ļ�¼
	Record AddRecord(char *q, Table table);
	//�жϼ�¼�Ƿ��ͻ��
	bool isConflict(Table table, Record record, const string &tablename, int i);
	
	BufferManager *buffer;

};
