#include "pool/sql_conn_pool.h"

#include <assert.h>
#include <cppconn/statement.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <stdio.h>

#include <iostream>
#include <string>
#include <vector>

using std::cin;
using std::cout;
using std::endl;
using std::string;

namespace TimelineServer {
class SQLPoolTest : public ::testing::Test {
 protected:
  // // 初始化数据
  void SetUp() override {
    Log::get_instance()->init(LOG_LEVEL::ELL_INFO,
                              "../data/sqlconnpool_test_log", ".log", 0);

    pool = SQLConnPool::get_instance();
    pool->init("localhost", 3306, "root", "explosion", "timelineserver",
               max_conns);
  }

  // override TearDown 来清理数据
  void TearDown() override { pool->close(); }

  int max_conns = 10;

  SQLConnPool* pool;
};

TEST_F(SQLPoolTest, pool) {
  std::vector<sql::Connection*> conns;

  for (int i = 0; i < max_conns + 3; i++) {
    auto conn = pool->get_connect();
    if (conn != nullptr) conns.push_back(conn);
  }

  while (!conns.empty()) {
    auto conn = conns.back();
    conns.pop_back();

    pool->free_connect(conn);
  }
}

TEST_F(SQLPoolTest, RAII) {
  SQLConn conn;
  EXPECT_TRUE(conn.is_valid());

  if (conn.is_valid()) {
    // 选定数据库
    // conn.connection->setSchema("books");

    sql::Statement* stmt = conn.connection->createStatement();

    stmt->execute(
        "DELETE FROM books "
        "WHERE id=111;");

    stmt->execute(
        "INSERT INTO books(id, title, author) "
        "VALUES (111, \"C++ Primer Plus\", \"Stephen Prata\")");

    sql::ResultSet* result = stmt->executeQuery("SELECT * from books");

    while (result->next()) {
      cout << "\tid: " << result->getInt("id")
           << "\ttitle: " << result->getString("title")
           << "\tauthor: " << result->getInt("author") << endl;
    }

    delete result;
    delete stmt;
  }
}
}  // namespace TimelineServer