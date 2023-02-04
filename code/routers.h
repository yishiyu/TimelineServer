#pragma once
#include <cppconn/prepared_statement.h>

#include <json11.hpp>
#include <string>

#include "buffer/buffer.h"
#include "http/http_request.h"
#include "log/log.h"
#include "pool/sql_conn_pool.h"
#include "server/server.h"

using std::string;
using TimelineServer::Buffer;
using TimelineServer::HttpRequest;

struct task {
  string task_id_;
  string time_;
  string task_;
  string priority_;
  task(const string& task_id, const string& time, const string& task,
       const string& priority)
      : task_id_(task_id), time_(time), task_(task), priority_(priority) {}
  Json to_json() const {
    return Json::object{{"task_id", task_id_},
                        {"time", time_},
                        {"task", task_},
                        {"priority", priority_}};
  }
};

bool router_login(const HttpRequest& request, Buffer& buffer);
bool router_logout(const HttpRequest& request, Buffer& buffer);
bool router_query(const HttpRequest& request, Buffer& buffer);
bool router_add(const HttpRequest& request, Buffer& buffer);
bool router_update(const HttpRequest& request, Buffer& buffer);
bool router_delete(const HttpRequest& request, Buffer& buffer);