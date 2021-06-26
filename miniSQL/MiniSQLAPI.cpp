#include "MiniSQLAPI.h"
#include <iostream>

void API::checkPredicate(const string &tablename, const Predicate &pred) const {
    const Table &table = CM->getTableInfo(tablename);
    for (const auto &pred : pred) {
        bool attr_exists = false;
        for (const auto &attr : table.attrs) {
            if (pred.first == attr.name) {
                attr_exists = true;
                break;
            }
        }
        if (!attr_exists) throw MiniSQLException("Invalid Attribute Identifier!");
    }
}

std::map<Compare, std::set<Value>> API::filterCondition(const std::vector<Condition> &conds) const {
    std::map<Compare, std::set<Value>> newCond;

    //�ϲ���Χ����
    for (auto cond : conds) {
        Compare comp = cond.comp;
        Value data = cond.data;
        if (comp == Compare::GE || comp == Compare::GT) {
            auto gCond = newCond.find(Compare::GE);
            if (newCond.end() == gCond) gCond = newCond.find(Compare::GT);
            if (newCond.end() == gCond) newCond[comp].insert(data);
            else {
                Compare old_comp = gCond->first;
                Value old_data = *(gCond->second.begin());
                if (old_data < data || (old_comp == Compare::GE && comp == Compare::GT && old_data == data)) {
                    newCond.erase(gCond);
                    newCond[comp].insert(data);
                }
            }
        }
        else if (comp == Compare::LE || comp == Compare::LT) {
            auto lCond = newCond.find(Compare::LE);
            if (newCond.end() == lCond) lCond = newCond.find(Compare::LT);
            if (newCond.end() == lCond) newCond[comp].insert(data);
            else {
                Compare old_comp = lCond->first;
                Value old_data = *(lCond->second.begin());
                if (old_data > data || (old_comp == Compare::LE && comp == Compare::LT && old_data == data)) {
                    newCond.erase(lCond);
                    newCond[comp].insert(data);
                }
            }
        }
    }

    //������Χ��ͻ�ж�
    auto gCond = newCond.find(Compare::GE);
    if (newCond.end() == gCond) gCond = newCond.find(Compare::GT);
    auto lCond = newCond.find(Compare::LE);
    if (newCond.end() == lCond) lCond = newCond.find(Compare::LT);

    bool hasLCond = (newCond.end() != lCond);
    bool hasGCond = (newCond.end() != gCond);
    if (hasLCond && hasGCond) {
        if (*(lCond->second.begin()) < *(gCond->second.begin()) ||
           (*(lCond->second.begin()) == *(gCond->second.begin()) && !(lCond->first == Compare::LE && gCond->first == Compare::GE))) {
            return std::map<Compare, std::set<Value>>();
        }
    }

    //�ϲ��������
    for (auto cond : conds) {
        if (cond.comp == Compare::EQ) {
            Value data = cond.data;
            //�ж��Ƿ��ڷ�Χ��
            if (hasLCond && (data > *(lCond->second.begin()) || (data == *(lCond->second.begin()) && lCond->first == Compare::LT))) {
                return std::map<Compare, std::set<Value>>();
            }
            if (hasGCond && (data < *(gCond->second.begin()) || (data == *(gCond->second.begin()) && gCond->first == Compare::GT))) {
                return std::map<Compare, std::set<Value>>();
            }
            //�жϵȺ��Ƿ��ͻ
            auto eqCond = newCond.find(Compare::EQ);
            if (newCond.end() != eqCond) {
                if(data != *(eqCond->second.begin())) return std::map<Compare, std::set<Value>>();
            }
            else {
                newCond.clear();
                hasLCond = hasGCond = false;
                newCond[Compare::EQ].insert(data);
            }
        }
    }

    //�ϲ���������
    for (auto cond : conds) {
        if (cond.comp == Compare::NE) {
            Value data = cond.data;
            
            //�жϵȺ��Ƿ��ͻ
            auto eqCond = newCond.find(Compare::EQ);
            if (newCond.end() != eqCond) {
                if (data == *(eqCond->second.begin())) return std::map<Compare, std::set<Value>>();
            }
            //�ж��Ƿ��ڷ�Χ��
            else if ((!hasLCond || (hasLCond && (data < *(lCond->second.begin()) || (data == *(lCond->second.begin()) && lCond->first == Compare::LE)))) &&
                (!hasGCond || (hasGCond && (data > *(gCond->second.begin()) || (data == *(gCond->second.begin()) && gCond->first == Compare::GE))))) {
                newCond[Compare::NE].insert(data);
            }
        }
    }

    return newCond;
}

void API::createTable(const string &tablename, const std::vector<Attr> &attrs, const set<string> &primary_key) {

    CM->addTableInfo(tablename, attrs);
    RM->createTable(tablename);
    if(primary_key.size() > 0) createIndex(tablename, "PRIMARY_KEY", primary_key);
}

void API::dropTable(const string &tablename) {
    const auto &indexes = CM->getIndexInfo(tablename);
    for (const auto &index : indexes) IM->dropIndex(tablename, index.name);

    CM->deleteTableInfo(tablename);
    CM->deleteIndexInfo(tablename);
    RM->dropTable(tablename);
}

void API::createIndex(const string &tablename, const string &indexname, const set<string> &keys) {
    set<int> key_positions;
    
    const Table &table = CM->getTableInfo(tablename);
    Type primary_key_type;
    int attr_pos = 0;
    size_t basic_length = sizeof(bool) + sizeof(int) * 3;
    for (const auto &key : keys) {
        bool key_exists = false;
        for (const auto &attr : table.attrs) {
            if (key == attr.name) {
                if (false == attr.unique) throw MiniSQLException("Index Key Is Not Unique!");
                primary_key_type = attr.type;
                key_exists = true;
                key_positions.insert(attr_pos);
                break;
            }
            attr_pos++;
        }
        if(!key_exists) throw MiniSQLException("Invalid Index Key Identifier!");
    }

    size_t rank = (PAGESIZE - basic_length) / (sizeof(int) + sizeof(Position) + primary_key_type.size) - 1;

    CM->addIndexInfo(tablename, indexname, rank, key_positions);
    IM->createIndex(tablename, indexname, primary_key_type, rank);

    ReturnTable T = selectFromTable(tablename, Predicate()).ret;
    for (const auto &record : T) {
        IM->insertIntoIndex(tablename, indexname, primary_key_type, rank, (record.content)[attr_pos], record.pos);
    }
}

void API::dropIndex(const string &tablename, const string &indexname) {
    CM->deleteIndexInfo(tablename, indexname);
    IM->dropIndex(tablename, indexname);
}

void API::insertIntoTable(const string &tablename, Record &record) {
    const Table &table = CM->getTableInfo(tablename);
    if (table.attrs.size() != record.size()) throw MiniSQLException("Wrong Number of Inserted Values!");

    //Valueת��
    auto value_ptr = record.begin();
    for (const auto &attr : table.attrs) {
        value_ptr->convertTo(attr.type);
        value_ptr++;
    }

    Position insertPos = RM->insertRecord(tablename, table, record);
    CM->increaseRecordCount(tablename);

    const auto &indexes = CM->getIndexInfo(tablename);
    for (const auto &index : indexes) {
        int key_position = *(index.key_positions.begin());
        Type index_key_type = table.attrs[key_position].type;
        const Value &value = record[key_position];
        IM->insertIntoIndex(tablename, index.name, index_key_type, index.rank, value,insertPos);
    }
}

/*
create table t (
 id int,
 name char(20),
 money float unique,
 primary key (id)
);

insert into t values (3, "abc", 4.5);
insert into t values (4, "abc", 5.5);
insert into t values (7, "abc", 10.5);
delete from t where id = 7;
insert into t values (7, "abc", 10.5);
select * from t where id = 4 and money > 600 and name <> "amy";

create table table2 (
 no int,
 name char(20) unique,
 salary float
);
*/

SQLResult API::selectFromTable(const string &tablename, Predicate &pred) {
    checkPredicate(tablename, pred);

    const Table &table = CM->getTableInfo(tablename);
    ReturnTable result;

    //��������
    const auto &indexes = CM->getIndexInfo(tablename);
    for (auto pred_ptr = pred.begin(); pred_ptr != pred.end(); pred_ptr++) {
        for (const auto &index : indexes) {
            int attr_pos = 0;
            const Table &table = CM->getTableInfo(tablename);
            for (const auto &attr : table.attrs) {
                if (attr.name == pred_ptr->first) break;
                attr_pos++;
            }

            if (index.key_positions.end() == index.key_positions.find(attr_pos)) continue;
            int key_position = *(index.key_positions.begin());
            Type index_key_type = table.attrs[key_position].type;

            vector<Position> possible_poses;

            //�����ϲ�
            auto newCond = filterCondition(pred_ptr->second);
            if (newCond.size() == 0) return SQLResult();
            if(newCond.end() != newCond.find(Compare::EQ)) {
                const Value &eqValue = *(newCond.find(Compare::EQ)->second.begin());
                Position pos;
                pos = IM->findOneFromIndex(tablename, index.name, index_key_type, index.rank, eqValue);
                if (pos.block_id < 0) return SQLResult();
                possible_poses.push_back(pos);
            } else {
                std::pair<Compare, Value> startKey = make_pair(Compare::EQ, Value());
                std::pair<Compare, Value> endKey = make_pair(Compare::EQ, Value());
                std::set<Value> neKeys;
                if (newCond.end() != newCond.find(Compare::GE)) startKey = make_pair(Compare::GE, *(newCond.find(Compare::GE)->second.begin()));
                else if (newCond.end() != newCond.find(Compare::GT)) startKey = make_pair(Compare::GT, *(newCond.find(Compare::GT)->second.begin()));
                if (newCond.end() != newCond.find(Compare::LE)) endKey = make_pair(Compare::LE, *(newCond.find(Compare::LE)->second.begin()));
                else if (newCond.end() != newCond.find(Compare::LT)) endKey = make_pair(Compare::LT, *(newCond.find(Compare::LT)->second.begin()));
                if (newCond.end() != newCond.find(Compare::NE)) neKeys = newCond.find(Compare::NE)->second;
                IM->findRangeFromIndex(tablename, index.name, index_key_type, index.rank, startKey, endKey, neKeys, possible_poses);
            }

            pred.erase(pred_ptr);
            result = RM->selectRecord(tablename, table, pred, possible_poses);
            return SQLResult{ table, result };
        }
    }

    result = RM->selectRecord(tablename, table, pred);
    return SQLResult{ table, result };
}

int API::deleteFromTable(const string &tablename, Predicate &pred) {
    checkPredicate(tablename, pred);

    const Table &table = CM->getTableInfo(tablename);
    SQLResult records = selectFromTable(tablename, pred);
    const ReturnTable &result = records.ret;
    for (const auto &record : result) RM->deleteRecord(tablename, record.pos);

    const auto &indexes = CM->getIndexInfo(tablename);
    for (const auto &index : indexes) {
        int key_position = *(index.key_positions.begin());
        Type index_key_type = table.attrs[key_position].type;
        for (const auto &record : result) {
            int key_position = *(index.key_positions.begin());
            const Value &value = record.content[key_position];
            IM->removeFromIndex(tablename, index.name, index_key_type, index.rank, value);
        }
    }

    return result.size();
}

void API_test() {
    BufferManager BM;
    CatalogManager CM(META_TABLE_FILE_PATH, META_INDEX_FILE_PATH);
    RecordManager RM(&BM);
    IndexManager IM(&BM);
    API core(&CM, &RM, &IM);
    try {
        core.createIndex("table1", "index1", { "a" });
        core.dropIndex("table1", "index1");
    } catch (MiniSQLException &e) {
        std::cout << e.getMessage() << std::endl;
    }
}