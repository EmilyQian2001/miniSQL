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
	void deleteRecord(const string &tablename, const Position &pos);
	Position insertRecord(const string &tablename, const Table &table, const Record &record);
private:
	//����������ļ��ж��ٿ�
	int getBlockNum(const Table &table) const;
	//�жϼ�¼�Ƿ��������
	bool isFit(const Value &v, const std::vector<Condition> &cond) const;
	//���ɷ��������ļ�¼
	Record addRecord(const char *const data, const Table &table);
	
	BufferManager *buffer;

};
