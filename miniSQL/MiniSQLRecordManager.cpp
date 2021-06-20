#include "MiniSQLRecordManager.h"

//����������ļ��ж��ٿ�
int RecordManager::getBlockNum(const Table &table) {
	return table.record_count / table.record_per_block + 1;
}

//����һ����¼�ж���λ������valid bit��
int RecordManager::getRecordLength(const Table &table) {
	size_t length = 1;//����valid bit
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
bool RecordManager::isFit(const Value &v, const Predicate &pred, const string &attr) {
	int flag = 0;
	for (auto iter = pred.at(attr).begin(); iter != pred.at(attr).end(); iter++) {
        switch (iter->comp)
        {
        case Compare::EQ:
            if (v == iter->data)flag = 1;
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
		size_t attr_length = iter->type.size;
		char *data = nullptr;
		data = new char[30];
        memcpy_s(data, 30, q, attr_length);
		Value v = Value::Value(iter->type, data);
		rec.push_back(v);
		q += attr_length;
	}
	return rec;
}

void RecordManager::createTable(const string &tablename) {
    string filename = "../" + tablename + ".table";

	FILE* fp;
	fopen_s(&fp, filename.data(), "w");
	if (fp == nullptr) throw MiniSQLException("Fail to create table file!"); //�����ļ�ʧ��
	fclose(fp);
}

void RecordManager::dropTable(const string &tablename) {
    string filename = "../" + tablename + ".table";

	buffer->setEmpty(filename);
	//����ļ�����,remove�ɹ�����0
    if (remove(filename.data()) != 0) throw MiniSQLException("Fail to drop table file!");
}
/*
select
input:filename,Table,Predicate
output:���ؼ�¼�����ɣ�
���ļ�ͷ��һ��һ������buffer��ÿ����һ���һ��һ���飬��valid bitΪ1�ļ�¼�бȽ�Predicate
Ȼ�����һ��set
*/
ReturnTable RecordManager::selectRecord(const string &filename, const Table &table, const Predicate &pred){
	int searched_record = 0;
	int block_num = getBlockNum(table);
	int record_length = getRecordLength(table);
	ReturnTable T;
    for (int k = 0; k < block_num; k++) {
        char* head = buffer->getBlockContent(filename, k);//���ظ�ҳ��ͷָ��
        char* p = head;
        while (true) {
            if (k == block_num - 1 && searched_record == table.record_count) break;
            if (*reinterpret_cast<bool*>(p) == true) { //valid bitΪ1
                p++;//�Ƶ���һ������
                //һ�������Ժ�pred�ȶ�
                int flag = 1;
                for (auto iter = table.attrs.begin(); iter != table.attrs.end(); iter++) {
                    string attr = iter->name;
                    size_t attr_length = iter->type.size;
                    if (pred.end() != pred.find(attr)) {//�ҵ�����������ϵ�����
                        //��ȡdata���бȽ�
                        char *data = new char[30];
                        memcpy_s(data, 30, p, attr_length);
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
                    pos.filename = filename;
                    pos.block_id = k;
                    pos.offset = searched_record * record_length;
                    //����set
                    RecordInfo rec;
                    rec.pos = pos;
                    char *q = p - record_length + 1;//ָ���һ������
                    rec.content = AddRecord(q, table);
                    T.push_back(rec);
                }
            }
            else p += record_length;
            if (++searched_record % (table.record_per_block) != 0) break;
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
void RecordManager::deleteRecord(const Position &pos) {
    string tablename = pos.filename;
    int block_id = pos.block_id;
    int offset = pos.offset;
    char *data = "0";
    buffer->setBlockContent(tablename, block_id, offset, data, 1);
}
/*
insert
input:filename,Table��Record
output:none
������Ҫ���unique���Ի������������Ƿ��ظ���throw�쳣
Ȼ���ҵ��ļ����һ�����ĩβ�����¼��valid bit��Ϊ1
*/
void RecordManager::insertRecord(const string &filename, const Table &table, const Record &record) {
	//����ͻ
    auto value_ptr = record.begin();
    for (auto attr = table.attrs.begin(); attr != table.attrs.end(); attr++, value_ptr++) {
        if (attr->unique) {
            Predicate pred;
            pred[attr->name].push_back({ Compare::EQ, *value_ptr });
            ReturnTable result = selectRecord(filename, table, pred);
            if(result.size() > 0) throw MiniSQLException("Duplicate Value on Unique Attribute!");
        }
    }
    //����
    int block_num = getBlockNum(table);
    int record_length = getRecordLength(table);
    int offset = (table.record_count - table.record_per_block*(block_num - 1))*record_length;
    //����valid
    bool valid = true;
    buffer->setBlockContent(filename, block_num - 1, offset, reinterpret_cast<char*>(&valid), sizeof(valid));
    //д����
    offset += sizeof(valid);
    for (auto iter = record.begin(); iter != record.end(); iter++) {
        char *data = iter->translate<char*>();
        buffer->setBlockContent(filename, block_num - 1, offset, data, iter->type.size);
        offset += iter->type.size;
    }
}