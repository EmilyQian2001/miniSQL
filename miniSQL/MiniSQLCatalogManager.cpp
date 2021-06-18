#include "MiniSQLCatalogManager.h"

const Table &CatalogManager::getTableInfo(const string &tablename) {
    auto t = table.find(tablename);
    if (table.end() == t) throw MiniSQLException("Table Doesn't Exist!");
    return table[tablename];
}

void CatalogManager::addTableInfo(const string &tablename, const vector<Attr> &attrs) {
    if (table.end() != table.find(tablename)) throw MiniSQLException("Duplicate Table Name!");

    int length = 1;
    for (auto attr : attrs) length += attr.type.size;
    table[tablename] = {attrs, PAGESIZE / length, 0};
}

void CatalogManager::deleteTableInfo(const string &tablename) {
    auto t = table.find(tablename);
    if (table.end() == t) throw MiniSQLException("Table Doesn't Exist!");
    table.erase(t);
}

bool CatalogManager::findIndex(const string &tablename, const string &indexname) const {
    if (index.end() == index.find(tablename)) return false;
    for (auto index_data = index.at(tablename).begin(); index_data != index.at(tablename).end(); index_data++) {
        if (index_data->name == indexname) return true;
    }
    return false;
}

void CatalogManager::addIndexInfo(const string &tablename, const string &indexname, initializer_list<string> keys) {
    if (findIndex(tablename, indexname)) throw MiniSQLException("Duplicate Index Name!");
    index[tablename].push_back({ indexname,keys });
}

void CatalogManager::deleteIndexInfo(const string &tablename, const string &indexname) {
    if (index.end() == index.find(tablename)) throw MiniSQLException("Index Doesn't Exist!");
    for (auto index_data = index[tablename].begin(); index_data != index[tablename].end(); index_data++) {
        if (index_data->name == indexname) index[tablename].erase(index_data);
        return;
    }
    throw MiniSQLException("Index doesn't exist!");
}

void CatalogManager::deleteIndexInfo(const string &tablename) {
    auto ind = index.find(tablename);
    if (index.end() == ind) throw MiniSQLException("Index Doesn't Exist!");
    index.erase(ind);
}