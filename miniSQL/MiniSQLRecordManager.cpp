#include "MiniSQLRecordManager.h"
#include <cmath>
#include <cstdio>
#include <cstring>
//����������ļ��ж��ٿ�
int RecordManager::getBlockNum(Table table) {
	float n = 1.0* table.record_count / table.record_count;
	return ceil(n);
}

//����һ����¼�ж���λ������valid bit��
int RecordManager::getRecordLength(Table table) {
	int length = 1;//����valid bit
	for (auto iter = table.attrs.begin(); iter != table.attrs.end(); iter++) {
		if (iter->type.btype==BaseType::CHAR) {
			length += iter->type.size;
		}
		else {//int��float����4���ֽ�
			length += 4;
		}
	}
	return length;
}
//�жϼ�¼�Ƿ��������
bool RecordManager::isFit(Value v, Predicate pred, const string &attr) {
	int flag = 0;
	for (auto iter = pred[attr].begin(); iter != pred[attr].end(); iter++) {
		switch (iter->comp)
		{
		case Compare::EQ:
			if (v==iter->data)flag = 1;
			else flag = 0;
			break;
		case Compare::LE:
			if (v <= iter->data)flag = 1;
			else flag = 0;
			break;
		case Compare::GE:
			if (v >= iter->data)flag = 1;
			else flag = 0;
			break;
		case Compare::NE:
			if (v != iter->data)flag = 1;
			else flag = 0;
			break;
		case Compare::LT:
			if (v < iter->data)flag = 1;
			else flag = 0;
			break;
		case Compare::GT:
			if (v > iter->data)flag = 1;
			else flag = 0;
			break;
		default:
			break;
		}
		if (flag == 0) return false;//ֻҪ��һ�����������ϾͲ�ѡ
	}
	return true;
}

//���ɷ��������ļ�¼
Record RecordManager::AddRecord(char *q, Table table) {
	Record rec;
	for (auto iter = table.attrs.begin(); iter != table.attrs.end(); iter++) {
		int attr_length = iter->type.size;
		char *data = nullptr;
		data = new char[30];
		strncpy(data, q, attr_length);
		Value v = Value::Value(iter->type, data);
		rec.push_back(v);
		q += attr_length;
	}
	return rec;
}

//�жϼ�¼�Ƿ��ͻ��
bool RecordManager::isConflict(Table table, Record record, const string &tablename, int i) {
	int searched_record = 0;
	int block_num = getBlockNum(table);
	int record_length = getRecordLength(table);
	for (int k = 0; k < block_num; k++) {
		char* head = buffer->getBlockContent(tablename, k);//���ظ�ҳ��ͷָ��
		char* p = head;
		if (k == block_num - 1) { //�����һ��
			do {
				if (*(p + 1) == '1') {
					p++;
					int j = 0;
					auto iter = table.attrs.begin();
					for (; j < i; j++, iter++) {
						p += iter->type.size;
					}
					//pָ���i�����Ե�ֵ,iterָ���i��Attr�ṹ
					char *data = nullptr;
					data = new char[30];
					strncpy(data, p, iter->type.size);
					Value v = Value::Value(iter->type, data);
					if (v==record[i])return true;
					for (; iter != table.attrs.end(); iter++) {
						p += iter->type.size;
					}
				}
				else {
					p += record_length;
				}
				searched_record++;
				
			} while (searched_record != table.record_count);
		}
		else {
			do {
				if (*(p + 1) == '1') {
					p++;
					int j = 0;
					auto iter = table.attrs.begin();
					for (; j < i; j++, iter++) {
						p += iter->type.size;
					}
					//pָ���i�����Ե�ֵ,iterָ���i��Attr�ṹ
					char *data = nullptr;
					data = new char[30];
					strncpy(data, p, iter->type.size);
					Value v = Value::Value(iter->type, data);
					if (v == record[i])return true;
					for (; iter != table.attrs.end(); iter++) {
						p += iter->type.size;
					}
				}
				else {
					p += record_length;
				}
				searched_record++;
			} while (searched_record % (table.record_per_block) != 0);
		}
	}
	return false;
}


void RecordManager::createTable(const string &tablename) {
	FILE* fp;
	fopen_s(&fp, tablename.c_str(), "rb+");
	if (fp == nullptr) throw MiniSQLException("Fail to create table file!"); //�����ļ�ʧ��
	fclose(fp);
}

void RecordManager::dropTable(const string &tablename, Table table) {
	buffer->setEmpty(tablename);
	//����ļ�����,remove�ɹ�����0
	if(remove(tablename.c_str())!=0)throw MiniSQLException("Fail to drop table file!");
}
/*
select
input:table_name,Table,Predicate
output:���ؼ�¼�����ɣ�
���ļ�ͷ��һ��һ������buffer��ÿ����һ���һ��һ���飬��valid bitΪ1�ļ�¼�бȽ�Predicate
Ȼ�����һ��set
*/
ReturnTable RecordManager::selectRecord(const string &tablename, Table table, Predicate pred){
	int searched_record = 0;
	int block_num = getBlockNum(table);
	int record_length = getRecordLength(table);
	ReturnTable T;
	for (int i = 0; i < block_num; i++) {
		char* head=buffer->getBlockContent(tablename, i);//���ظ�ҳ��ͷָ��
		char* p = head;
		if (i == block_num - 1) { //�����һ��
			do {
				if (*(p + 1) == '1') { //valid bitΪ1
					p++;//�Ƶ���һ������
					//һ�������Ժ�pred�ȶ�
					int flag = 1;
					for (auto iter = table.attrs.begin(); iter != table.attrs.end(); iter++) {
						string attr = iter->name;
						int attr_length = iter->type.size;
						if (pred.end() != pred.find(attr)) {//�ҵ�����������ϵ�����
							//��ȡdata���бȽ�
							char *data = nullptr;
							data = new char[30];
							strncpy(data, p, attr_length);
							Value v = Value::Value(iter->type, data);
							if (!isFit(v, pred, attr)) {//��������
								flag = 0;
								break;
							}
						}
						p += attr_length;
					}
					if (flag == 1) {//ѭ��֮��flag��Ϊ1 or û��where����
						//pos
						Position pos;
						pos.filename = tablename;
						pos.block_id = i;
						pos.offset = searched_record * record_length;
						//����set
						RecordInfo rec;
						rec.pos = pos;
						char *q = p - record_length + 1;//ָ���һ������
						rec.content = AddRecord(q, table);
						T.insert(rec);
					}
				}
				else {
					p += record_length;
				}
				searched_record++;//����һ����¼
			} while (searched_record != table.record_count);
		}
		else {//֮ǰ�Ŀ鶼�����ģ���¼��Ϊrecord_per_block
			do {
				if (*(p+1)=='1') { //valid bitΪ1
					p++;//�Ƶ���һ������
					//һ�������Ժ�pred�ȶ�
					int flag = 1;
					for (auto iter = table.attrs.begin(); iter != table.attrs.end(); iter++) {
						string attr = iter->name;
						int attr_length = iter->type.size;
						if (pred.end() != pred.find(attr)) {//�ҵ�����������ϵ�����
							//��ȡdata���бȽ�
							char *data = nullptr;
							data = new char[30];
							strncpy(data, p, attr_length);
							Value v = Value::Value(iter->type,data);
							if (!isFit(v, pred, attr)) {//��������
								flag = 0;
								break;
							}
						}
						p += attr_length;
					}
					if (flag == 1) {//ѭ��֮��flag��Ϊ1 or û��where����
						//pos
						Position pos;
						pos.filename = tablename;
						pos.block_id = i;
						pos.offset = searched_record * record_length;
						//����set
						RecordInfo rec;
						rec.pos = pos;
						char *q = p - record_length + 1;//ָ���һ������
						rec.content = AddRecord(q,table);
						T.insert(rec);
					}
				}
				else {
					p += record_length;
				}
				searched_record++;//����һ����¼
			} while (searched_record % (table.record_per_block) != 0);//searched_blockΪ������˵����һ��ļ�¼������
			
		}
	}
	return T;
}

/*
delete
input:table_name,Table,Predicate
output:none
����position����buffer����Ӧ��dirty=true���ü�¼��valid bit��Ϊfalse
*/
void RecordManager::deleteRecord(Position pos) {
	string tablename = pos.filename;
	int block_id = pos.block_id;
	int offset = pos.offset;
	char *data = "0";
	buffer->setBlockContent(tablename, block_id, offset, data, 1);
}
/*
insert
input:table_name,Table��Record
output:none
������Ҫ���unique���Ի������������Ƿ��ظ���throw�쳣
Ȼ���ҵ��ļ����һ�����ĩβ�����¼��valid bit��Ϊ1
*/
void RecordManager::insertRecord(const string &tablename, Table table, Record record) {
	//����ͻ
	int i = 0;//��i������
	for (auto iter = table.attrs.begin(); iter != table.attrs.end(); iter++,i++) {
		string attr = iter->name;
		if (iter->unique) {
			if (isConflict(table, record, tablename, i)) throw MiniSQLException("Duplicate value on unique attribute!");
		}
	}
	//����
	int block_num = getBlockNum(table);
	int record_length = getRecordLength(table);
	int offset = (table.record_count - table.record_per_block*(block_num-1))*record_length;
	//����valid
	char *valid = "1";
	buffer->setBlockContent(tablename, block_num-1, offset, valid, 1);
	//д����
	offset++;
	for (auto iter = record.begin(); iter != record.end(); iter++) {
		char *data = iter->translate<char*>();
		buffer->setBlockContent(tablename, block_num - 1, offset, data, iter->type.size);
		offset += iter->type.size;
	}
}
