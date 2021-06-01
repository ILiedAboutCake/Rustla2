#pragma once

#include <folly/String.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sqlite_modern_cpp.h>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include <boost/functional/hash.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "Channel.h"
#include "Status.h"

namespace rustla2 {

class User {
 public:
  User(sqlite::database db, const boost::uuids::uuid id,
       const int64_t twitch_id, const std::string &name, const Channel &channel,
       const std::string &last_ip, const time_t last_seen, const bool left_chat,
       const bool is_admin, const bool show_hidden, const bool show_dgg_chat,
       const bool enable_public_state, const time_t created_at)
      : db_(db),
        id_(id),
        twitch_id_(twitch_id),
        name_(name),
        channel_(std::shared_ptr<Channel>(channel)),
        last_ip_(last_ip),
        last_seen_(last_seen),
        left_chat_(left_chat),
        is_admin_(is_admin),
        show_hidden_(show_hidden),
        show_dgg_chat_(show_dgg_chat),
        enable_public_state_(enable_public_state),
        created_at_(created_at) {}

  User(sqlite::database db, const uint64_t twitch_id, const Channel &channel,
       const std::string &last_ip)
      : db_(db),
        id_(boost::uuids::random_generator()()),
        twitch_id_(twitch_id),
        channel_(std::shared_ptr<Channel>(channel)),
        last_ip_(last_ip),
        last_seen_(time(nullptr)),
        left_chat_(false),
        is_admin_(false),
        show_hidden_(false),
        show_dgg_chat_(false),
        enable_public_state_(true),
        created_at_(time(nullptr)) {}

  User(const User &user)
      : db_(user.db_),
        id_(user.id_),
        twitch_id_(user.twitch_id_),
        name_(user.name_),
        channel_(user.channel_),
        last_ip_(user.last_ip_),
        last_seen_(user.last_seen_),
        left_chat_(user.left_chat_),
        is_admin_(user.is_admin_),
        show_hidden_(user.show_hidden_),
        show_dgg_chat_(user.show_dgg_chat_),
        enable_public_state_(user.enable_public_state_),
        created_at_(user.created_at_) {}

  inline boost::uuids::uuid GetID() {
    boost::shared_lock<boost::shared_mutex> read_lock(lock_);
    return id_;
  }

  inline const std::string GetIDString() {
    boost::shared_lock<boost::shared_mutex> read_lock(lock_);
    return boost::uuids::to_string(id_);
  }

  inline uint64_t GetTwitchID() {
    boost::shared_lock<boost::shared_mutex> read_lock(lock_);
    return twitch_id_;
  }

  inline std::string GetName() {
    boost::shared_lock<boost::shared_mutex> read_lock(lock_);
    return name_;
  }

  inline std::shared_ptr<Channel> GetChannel() {
    boost::shared_lock<boost::shared_mutex> read_lock(lock_);
    return channel_;
  }

  inline std::string GetLastIP() {
    boost::shared_lock<boost::shared_mutex> read_lock(lock_);
    return last_ip_;
  }

  inline time_t GetLastSeen() {
    boost::shared_lock<boost::shared_mutex> read_lock(lock_);
    return last_seen_;
  }

  inline bool GetLeftChat() {
    boost::shared_lock<boost::shared_mutex> read_lock(lock_);
    return left_chat_;
  }

  inline bool GetIsAdmin() {
    boost::shared_lock<boost::shared_mutex> read_lock(lock_);
    return is_admin_;
  }

  inline bool GetShowHidden() {
    boost::shared_lock<boost::shared_mutex> read_lock(lock_);
    return show_hidden_;
  }

  inline bool GetShowDggChat() {
    boost::shared_lock<boost::shared_mutex> read_lock(lock_);
    return show_dgg_chat_;
  }

  inline bool GetEnablePublicState() {
    boost::shared_lock<boost::shared_mutex> read_lock(lock_);
    return enable_public_state_;
  }

  inline time_t GetCreatedAt() {
    boost::shared_lock<boost::shared_mutex> read_lock(lock_);
    return created_at_;
  }

  std::string GetStreamJSON();

  std::string GetUsernameJSON();

  std::string GetProfileJSON();

  void WriteJSON(rapidjson::Writer<rapidjson::StringBuffer> *writer);

  Status SetName(const std::string &name);

  bool SetChannel(const Channel &channel);

  bool SetLeftChat(bool left_chat);

  bool SetLastIP(const std::string &last_ip);

  bool SetLastSeen(const time_t last_seen);

  bool SetShowHidden(bool show_hidden);

  bool SetShowDggChat(bool show_dgg_chat);

  bool SetEnablePublicState(bool set_enable_public_state);

  bool Save();

  bool SaveNew();

 private:
  sqlite::database db_;
  boost::shared_mutex lock_;
  boost::uuids::uuid id_;
  uint64_t twitch_id_;
  std::string name_;
  std::shared_ptr<Channel> channel_;
  std::string last_ip_;
  time_t last_seen_;
  bool left_chat_;
  bool is_admin_;
  bool show_hidden_;
  bool show_dgg_chat_;
  bool enable_public_state_;
  time_t created_at_;

  friend class Users;
  friend std::ostream &operator<<(std::ostream &os, const User &user);
};

struct NameHash : public std::unary_function<std::string, std::size_t> {
  std::size_t operator()(const std::string &k) const {
    std::string k_lower = k;
    folly::toLowerAscii(k_lower);
    return std::hash<std::string>{}(k_lower);
  }
};

struct NameEqual : public std::binary_function<std::string, std::string, bool> {
  bool operator()(const std::string &a, const std::string &b) const {
    return strcasecmp(a.c_str(), b.c_str()) == 0;
  }
};

class Users {
 public:
  explicit Users(sqlite::database db);

  void InitTable();

  std::shared_ptr<User> GetByID(const std::string &id) {
    boost::uuids::uuid uuid;
    try {
      uuid = boost::uuids::string_generator()(id);
    } catch (std::runtime_error &e) {
      return nullptr;
    }

    return GetByID(uuid);
  }

  std::shared_ptr<User> GetByID(const boost::uuids::uuid &id);

  std::shared_ptr<User> GetByTwitchID(const uint64_t twitch_id);

  std::shared_ptr<User> GetByName(const std::string &name);

  std::shared_ptr<User> GetByStreamPath(const std::string &stream_path);

  std::shared_ptr<User> Emplace(const uint64_t twitch_id,
                                const Channel &channel, const std::string &ip);

  Status Save(std::shared_ptr<User> user);

  void WriteJSON(rapidjson::Writer<rapidjson::StringBuffer> *writer);

 private:
  sqlite::database db_;
  boost::shared_mutex lock_;

  std::unordered_map<boost::uuids::uuid, std::shared_ptr<User>,
                     boost::hash<boost::uuids::uuid>>
      data_by_id_;
  std::unordered_map<uint64_t, std::shared_ptr<User>> data_by_twitch_id_;
  std::unordered_map<std::string, std::shared_ptr<User>, NameHash, NameEqual>
      data_by_name_;
  std::unordered_map<std::string, std::shared_ptr<User>> data_by_stream_path_;
};

}  // namespace rustla2
