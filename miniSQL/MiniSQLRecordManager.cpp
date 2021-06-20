#include "MiniSQLRecordManager.h"

//����������ļ��ж��ٿ�
int RecordManager::getBlockNum(const Table &table) const {
	return table.record_count / table.record_per_block + 1;
}

//����һ����¼�ж���λ������valid bit��
int RecordManager::getRecordLength(const Table &table) const {
	size_t length = 1;//����valid bit
	for (auto attr : table.attrs) length += attr.type.size;
	return length;
}

//�жϼ�¼�Ƿ��������
bool RecordManager::isFit(const Value &v, const std::vector<Condition> &cond) const {
	for (auto iter = cond.begin(); iter != cond.end(); iter++) {
        switch (iter->comp)
        {
        case Compare::EQ:
            if (v != iter->data) return false;
            break;
        case Compare::LE:
            if (v > iter->data) return false;
            break;
        case Compare::GE:
            if (v < iter->data) return false;
            break;
        case Compare::NE:
            if (v == iter->data) return false;
            break;
        case Compare::LT:
            if (v >= iter->data) return false;
            break;
        case Compare::GT:
            if (v <= iter->data) return false;
            break;
        default: throw MiniSQLException("Unsupported Comparation!");
        }
	}
	return true;
}

//���ɷ��������ļ�¼
Record RecordManager::addRecord(const char *const data, const Table &table) {
	Record record;
    const char *p = data;
	for (auto attr : table.attrs) {
		record.push_back(Value(attr.type, p));
		p += attr.type.size;
	}
	return record;
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
        char* curRecord = buffer->getBlockContent(filename, k);//���ظ�ҳ��ͷָ��
        while (true) {
            if (k == block_num - 1 && searched_record == table.record_count) break;
            char *p = curRecord;
            if (*reinterpret_cast<bool*>(p) == true) { //valid bitΪ1
                p++;//�Ƶ���һ������
                //һ�������Ժ�pred�ȶ�
                bool satisfied = true;
                for (auto attr : table.attrs) {
                    string attr_name = attr.name;
                    size_t attr_length = attr.type.size;
                    if (pred.end() != pred.find(attr_name)) {//�ҵ�����������ϵ�����
                        //��ȡdata���бȽ�
                        Value v(attr.type, p);
                        if (!isFit(v, pred.at(attr_name))) {//��������
                            satisfied = false;
                            break;
                        }
                    }
                    p += attr_length;
                }
                if (satisfied) {//ѭ��֮��satisfied��Ϊ1 or û��where����
                    //����set
                    RecordInfo rec;
                    rec.pos = { filename, k, searched_record * record_length };
                    rec.content = addRecord(curRecord, table);
                    T.push_back(rec);
                }
            }

            curRecord += record_length;
            if (++searched_record % (table.record_per_block) == 0) break;
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
    for (auto value : record) {
        char *data = value.translate<char*>();
        buffer->setBlockContent(filename, block_num - 1, offset, data, value.type.size);
        offset += value.type.size;
    }
}