#include "MiniSQLRecordManager.h"

#define TABLE_FILE_PATH(tablename) ("../" + (tablename) + ".table")

//����������ļ��ж��ٿ�
int RecordManager::getBlockNum(const Table &table) const {
    int record_per_block = PAGESIZE / table.record_length;
	return table.occupied_record_count / record_per_block + 1;
}

//�жϼ�¼�Ƿ��������
bool RecordManager::isFit(const Value &v, const std::vector<Condition> &cond) const {
	for (const auto &iter : cond) {
        switch (iter.comp)
        {
        case Compare::EQ:
            if (v != iter.data) return false;
            break;
        case Compare::LE:
            if (v > iter.data) return false;
            break;
        case Compare::GE:
            if (v < iter.data) return false;
            break;
        case Compare::NE:
            if (v == iter.data) return false;
            break;
        case Compare::LT:
            if (v >= iter.data) return false;
            break;
        case Compare::GT:
            if (v <= iter.data) return false;
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
	for (const auto &attr : table.attrs) {
		record.push_back(Value(attr.type, p));
		p += attr.type.size;
	}
	return record;
}

void RecordManager::createTable(const string &tablename) {
    string filename = TABLE_FILE_PATH(tablename);

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
input:tablename,Table,Predicate
output:���ؼ�¼�����ɣ�
���ļ�ͷ��һ��һ������buffer��ÿ����һ���һ��һ���飬��valid bitΪ1�ļ�¼�бȽ�Predicate
Ȼ�����һ��set
*/
ReturnTable RecordManager::selectRecord(const string &tablename, const Table &table, const Predicate &pred){
    string filename = TABLE_FILE_PATH(tablename);

	int searched_record = 0;
	int block_num = getBlockNum(table);
    int record_length = table.record_length;
    int record_per_block = PAGESIZE / record_length;
	ReturnTable T;
    for (int k = 0; k < block_num; k++) {
        char* curRecord = buffer->getBlockContent(filename, k);//���ظ�ҳ��ͷָ��
        while (true) {
            if (k == block_num - 1 && searched_record == table.occupied_record_count) break;
            char *p = curRecord;
            if (*reinterpret_cast<bool*>(p) == true) { //valid bitΪ1
                p++;//�Ƶ���һ������
                //һ�������Ժ�pred�ȶ�
                bool satisfied = true;
                for (const auto &attr : table.attrs) {
                    string attr_name = attr.name;
                    if (pred.end() != pred.find(attr_name)) {//�ҵ�����������ϵ�����
                        //��ȡdata���бȽ�
                        Value v(attr.type, p);
                        if (!isFit(v, pred.at(attr_name))) {//��������
                            satisfied = false;
                            break;
                        }
                    }
                    p += attr.type.size;
                }
                if (satisfied) {//ѭ��֮��satisfied��Ϊ1 or û��where����
                    //����set
                    RecordInfo rec;
                    rec.pos = { k, searched_record * record_length };
                    rec.content = addRecord(curRecord + sizeof(bool), table);
                    T.push_back(rec);
                }
            }

            curRecord += record_length;
            if (++searched_record % record_per_block == 0) break;
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
void RecordManager::deleteRecord(const string &tablename, const Position &pos) {
    string filename = TABLE_FILE_PATH(tablename);
    bool valid = false;
    buffer->setBlockContent(filename, pos.block_id, pos.offset, reinterpret_cast<char*>(&valid), sizeof(valid));
}
/*
insert
input:tablename,Table��Record
output:none
������Ҫ���unique���Ի������������Ƿ��ظ���throw�쳣
Ȼ���ҵ��ļ����һ�����ĩβ�����¼��valid bit��Ϊ1
*/
Position RecordManager::insertRecord(const string &tablename, const Table &table, const Record &record) {
    string filename = TABLE_FILE_PATH(tablename);

	//����ͻ
    auto value_ptr = record.begin();
    for (auto attr = table.attrs.begin(); attr != table.attrs.end(); attr++, value_ptr++) {
        if (attr->unique) {
            Predicate pred;
            pred[attr->name].push_back({ Compare::EQ, *value_ptr });
            ReturnTable result = selectRecord(tablename, table, pred);
            if(result.size() > 0) throw MiniSQLException("Duplicate Value on Unique Attribute!");
        }
    }
    //����
    int inserted_block_num = getBlockNum(table) - 1;
    int record_per_block = PAGESIZE / table.record_length;
    int offset = (table.occupied_record_count - record_per_block*inserted_block_num)*table.record_length;
    Position pos = { inserted_block_num, offset };
    //����valid
    bool valid = true;
    buffer->setBlockContent(filename, inserted_block_num, offset, reinterpret_cast<char*>(&valid), sizeof(valid));
    //д����
    offset += sizeof(valid);
    for (const auto &value : record) {
        char *data = value.translate<char*>();
        buffer->setBlockContent(filename, inserted_block_num, offset, data, value.type.size);
        offset += value.type.size;
    }

    return pos;
}