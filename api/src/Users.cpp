#include "Users.h"

#include <glog/logging.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include "Config.h"
#include "Strings.h"

namespace rustla2 {

std::string User::GetStreamJSON() {
  rapidjson::StringBuffer buf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buf);

  writer.StartObject();
  writer.Key("service");
  writer.String(channel_->GetService());
  writer.Key("channel");
  writer.String(channel_->GetChannel());
  writer.EndObject();

  return buf.GetString();
}

std::string User::GetUsernameJSON() {
  rapidjson::StringBuffer buf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buf);

  writer.StartObject();
  writer.Key("username");
  writer.String(name_);
  writer.Key("created_at");
  writer.Int(created_at_);
  writer.EndObject();

  return buf.GetString();
}

std::string User::GetProfileJSON() {
  rapidjson::StringBuffer buf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buf);

  writer.StartObject();
  writer.Key("username");
  writer.String(name_);
  writer.Key("stream_path");
  writer.String(channel_->GetStreamPath());
  writer.Key("service");
  writer.String(channel_->GetService());
  writer.Key("channel");
  writer.String(channel_->GetChannel());
  writer.Key("left_chat");
  writer.Bool(left_chat_);
  writer.Key("is_admin");
  writer.Bool(is_admin_);
  writer.Key("show_hidden");
  writer.Bool(show_hidden_);
  writer.Key("show_dgg_chat");
  writer.Bool(show_dgg_chat_);
  writer.Key("enable_public_state");
  writer.Bool(enable_public_state_);
  writer.Key("created_at");
  writer.Int(created_at_);
  writer.EndObject();

  return buf.GetString();
}

void User::WriteJSON(rapidjson::Writer<rapidjson::StringBuffer> *writer) {
  writer->StartObject();
  writer->Key("username");
  writer->String(name_);
  writer->Key("stream_path");
  writer->String(channel_->GetStreamPath());
  writer->Key("channel");
  channel_->WriteJSON(writer);
  writer->Key("left_chat");
  writer->Bool(left_chat_);
  writer->Key("last-ip");
  writer->String(last_ip_);
  writer->Key("last_seen");
  writer->Int(last_seen_);
  writer->Key("is_admin");
  writer->Bool(is_admin_);
  writer->Key("show_hidden");
  writer->Bool(show_hidden_);
  writer->Key("show_dgg_chat");
  writer->Bool(show_dgg_chat_);
  writer->Key("enable_public_state");
  writer->Bool(enable_public_state_);
  writer->Key("created_at");
  writer->Int(created_at_);
  writer->EndObject();
}

Status User::SetName(const std::string &name) {
  if (name == name_) {
    return Status::OK;
  }

  // based on destinygg/website username validation
  // https://github.com/destinygg/website/blob/0e984436d2d381f02666272e8bf38eb9ebda476a/lib/Destiny/Common/Authentication/AuthenticationService.php

  const boost::regex valid_name_regex("^[A-Za-z0-9_]{3,20}$");
  if (!boost::regex_match(name, valid_name_regex)) {
    return Status(StatusCode::VALIDATION_ERROR,
                  "Username may only contain A-z 0-9 or underscores and must "
                  "be betwee 3 and 20 characters in length.");
  }

  const auto prefix_check_size =
      Config::Get().GetEmoteSimilarityPrefixCheckSize();

  std::string name_norm = name;
  folly::toLowerAscii(name_norm);
  auto name_start = name_norm.substr(0, prefix_check_size);

  for (const auto &emote : Config::Get().GetEmotes()) {
    std::string emote_norm = emote;
    folly::toLowerAscii(emote_norm);

    if (emote.size() >= Config::Get().GetEmoteSubstringMinLength() &&
        folly::StringPiece(name_norm).contains(emote_norm)) {
      return Status(StatusCode::VALIDATION_ERROR,
                    "Username cannot contain emote (" + emote + ")");
    }

    if (emote.size() < Config::Get().GetEmoteSimilarityMinLength()) {
      continue;
    }

    auto emote_start = emote_norm.substr(0, prefix_check_size);
    auto name_trunc = name_norm.substr(0, std::min(emote.size(), name.size()));

    const auto min_edit_distance =
        Config::Get().GetEmoteSimilarityMinEditDistance();
    if (emote_start == name_start &&
        levenshtein_distance(emote_norm, name_trunc) <= min_edit_distance) {
      return Status(StatusCode::VALIDATION_ERROR,
                    "Username too similar to an emote, try changing the first "
                    "characters.");
    }
  }

  const boost::regex repeated_number_regex("[0-9]{3}");
  if (boost::regex_match(name, repeated_number_regex)) {
    return Status(StatusCode::VALIDATION_ERROR,
                  "Too many numbers in a row in username.");
  }

  const boost::regex repeated_underscore_regex("[_]{2}|(_.*_.*_)");
  if (boost::regex_match(name, repeated_underscore_regex)) {
    return Status(StatusCode::VALIDATION_ERROR,
                  "Too many underscores in username.");
  }

  const boost::regex non_numbers_regex("[^0-9]");
  auto name_numbers = boost::regex_replace(name, non_numbers_regex, "");
  if (name_numbers.size() > (name.size() + 1) / 2) {
    return Status(StatusCode::VALIDATION_ERROR,
                  "Number ratio is too high in username.");
  }

  boost::unique_lock<boost::shared_mutex> write_lock(lock_);
  name_ = name;
  return Status::OK;
}

bool User::SetChannel(const Channel &channel) {
  boost::unique_lock<boost::shared_mutex> write_lock(lock_);
  channel_ = std::make_shared<Channel>(channel);
  return true;
}

bool User::SetLeftChat(bool left_chat) {
  boost::unique_lock<boost::shared_mutex> write_lock(lock_);
  left_chat_ = left_chat;
  return true;
}

bool User::SetLastIP(const std::string &last_ip) {
  boost::unique_lock<boost::shared_mutex> write_lock(lock_);
  last_ip_ = last_ip;
  return true;
}

bool User::SetLastSeen(const time_t last_seen) {
  boost::unique_lock<boost::shared_mutex> write_lock(lock_);
  last_seen_ = last_seen;
  return true;
}

bool User::SetShowHidden(bool show_hidden) {
  boost::unique_lock<boost::shared_mutex> write_lock(lock_);
  show_hidden_ = show_hidden;
  return true;
}

bool User::SetShowDggChat(bool show_dgg_chat) {
  boost::unique_lock<boost::shared_mutex> write_lock(lock_);
  show_dgg_chat_ = show_dgg_chat;
  return true;
}

bool User::SetEnablePublicState(bool enable_public_state) {
  boost::unique_lock<boost::shared_mutex> write_lock(lock_);
  enable_public_state_ = enable_public_state;
  return true;
}

bool User::Save() {
  boost::shared_lock<boost::shared_mutex> read_lock(lock_);
  try {
    const auto sql = R"sql(
        UPDATE `users` SET
          `name` = ?,
          `stream_path` = ?,
          `service` = ?,
          `channel` = ?,
          `last_ip` = ?,
          `last_seen` = datetime(?, 'unixepoch'),
          `left_chat` = ?,
          `is_admin` = ?,
          `show_hidden` = ?,
          `show_dgg_chat` = ?,
          `enable_public_state` = ?,
          `updated_at` = datetime()
        WHERE `id` = ?
      )sql";
    db_ << sql << name_ << channel_->GetStreamPath() << channel_->GetService()
        << channel_->GetChannel() << last_ip_ << last_seen_ << left_chat_
        << is_admin_ << show_hidden_ << show_dgg_chat_ << enable_public_state_
        << GetIDString();
  } catch (const sqlite::sqlite_exception &e) {
    LOG(ERROR) << "error updating user " << this << ", "
               << "error: " << e.what() << ", "
               << "code: " << e.get_extended_code();

    return false;
  }
  return true;
}

bool User::SaveNew() {
  boost::shared_lock<boost::shared_mutex> read_lock(lock_);
  try {
    const auto sql = R"sql(
        INSERT INTO `users` (
          `id`,
          `twitch_id`,
          `twitch_username`,
          `name`,
          `stream_path`,
          `service`,
          `channel`,
          `last_ip`,
          `last_seen`,
          `left_chat`,
          `is_admin`,
          `show_hidden`,
          `show_dgg_chat`,
          `enable_public_state`,
          `ban_reason`,
          `created_at`,
          `updated_at`
        )
        VALUES (
          ?,
          ?,
          ?,
          ?,
          ?,
          ?,
          ?,
          ?,
          datetime(?, 'unixepoch'),
          ?,
          ?,
          ?,
          ?,
          ?,
          '',
          datetime(),
          datetime()
        )
      )sql";
    db_ << sql << GetIDString() << twitch_id_ << channel_->GetChannel() << name_
        << channel_->GetStreamPath() << channel_->GetService()
        << channel_->GetChannel() << last_ip_ << last_seen_ << left_chat_
        << is_admin_ << show_hidden_ << show_dgg_chat_ << enable_public_state_;
  } catch (const sqlite::sqlite_exception &e) {
    LOG(ERROR) << "error creating user " << this << ", "
               << "error: " << e.what() << ", "
               << "code: " << e.get_extended_code();

    return false;
  }
  return true;
}

std::ostream &operator<<(std::ostream &os, const User &user) {
  os << "id: " << user.id_ << ", "
     << "twitch_id: " << user.twitch_id_ << ", "
     << "name: " << user.name_ << ", "
     << "channel: " << *user.channel_ << ", "
     << "last_ip: " << user.last_ip_ << ", "
     << "last_seen: " << user.last_seen_ << ", "
     << "left_chat: " << user.left_chat_ << ", "
     << "is_admin: " << user.is_admin_ << ", "
     << "show_hidden: " << user.show_hidden_ << ", "
     << "show_dgg_chat: " << user.show_dgg_chat_ << ", "
     << "enable_public_state: " << user.enable_public_state_ << ", "
     << "created_at" << user.created_at_;
  return os;
}

Users::Users(sqlite::database db) : db_(db) {
  InitTable();

  LOG(INFO) << "loading users";

  auto sql = R"sql(
      SELECT
        `id`,
        `twitch_id`,
        `name`,
        `stream_path`,
        `service`,
        `channel`,
        `last_ip`,
        strftime('%s', `last_seen`),
        `left_chat`,
        `is_admin`,
        `show_hidden`,
        `show_dgg_chat`,
        `enable_public_state`,
        `created_at`
      FROM `users`
    )sql";
  auto query = db_ << sql;

  query >> [&](const std::string &id, const uint64_t twitch_id,
               const std::string &name, const std::string &stream_path,
               const std::string &service, const std::string &channel,
               const std::string &last_ip, const time_t last_seen,
               const bool left_chat, const bool is_admin,
               const bool show_hidden, const bool show_dgg_chat,
               const bool enable_public_state, const time_t created_at) {
    boost::uuids::string_generator to_uuid;
    auto user_channel = Channel::Create(channel, service, stream_path);
    auto user = std::make_shared<User>(
        db_, to_uuid(id), twitch_id, name, user_channel, last_ip, last_seen,
        left_chat, is_admin, show_hidden, show_dgg_chat, enable_public_state,
        created_at);

    data_by_id_[user->GetID()] = user;
    data_by_twitch_id_[user->GetTwitchID()] = user;
    data_by_name_[user->GetName()] = user;
    data_by_stream_path_[user->GetChannel()->GetStreamPath()] = user;
  };

  LOG(INFO) << "read " << data_by_id_.size() << " users";
}

void Users::InitTable() {
  LOG(INFO) << "initializing users table";

  auto sql = R"sql(
      CREATE TABLE IF NOT EXISTS `users` (
        `id` CHAR(36) NOT NULL,
        `twitch_id` UNSIGNED BIGINT NOT NULL,
        `twitch_username` VARCHAR(32) NOT NULL,
        `name` VARCHAR(32) NOT NULL,
        `stream_path` VARCHAR(255) NOT NULL,
        `service` VARCHAR(255) NOT NULL,
        `channel` VARCHAR(255) NOT NULL,
        `last_ip` VARCHAR(255) NOT NULL,
        `last_seen` DATETIME NOT NULL,
        `left_chat` TINYINT(1) DEFAULT 0,
        `is_banned` TINYINT(1) NOT NULL DEFAULT 0,
        `ban_reason` VARCHAR(255),
        `created_at` DATETIME NOT NULL,
        `updated_at` DATETIME NOT NULL,
        `is_admin` TINYINT(1) DEFAULT 0,
        `show_hidden` TINYINT(1) DEFAULT 0,
        `show_dgg_chat` TINYINT(1) DEFAULT 0,
        `enable_public_state` TINYINT(1) DEFAULT 1,
        UNIQUE (`id`),
        UNIQUE (`twitch_id`)
      )
    )sql";
  db_ << sql;
}

std::shared_ptr<User> Users::GetByID(const boost::uuids::uuid &id) {
  boost::shared_lock<boost::shared_mutex> read_lock(lock_);

  auto i = data_by_id_.find(id);
  if (i == data_by_id_.end()) {
    return nullptr;
  }
  return i->second;
}

std::shared_ptr<User> Users::GetByTwitchID(const uint64_t twitch_id) {
  boost::shared_lock<boost::shared_mutex> read_lock(lock_);

  auto i = data_by_twitch_id_.find(twitch_id);
  if (i == data_by_twitch_id_.end()) {
    return nullptr;
  }
  return i->second;
}

std::shared_ptr<User> Users::GetByName(const std::string &name) {
  boost::shared_lock<boost::shared_mutex> read_lock(lock_);

  auto i = data_by_name_.find(name);
  if (i == data_by_name_.end()) {
    return nullptr;
  }
  return i->second;
}

std::shared_ptr<User> Users::GetByStreamPath(const std::string &stream_path) {
  boost::shared_lock<boost::shared_mutex> read_lock(lock_);

  auto i = data_by_stream_path_.find(stream_path);
  if (i == data_by_stream_path_.end()) {
    return nullptr;
  }
  return i->second;
}

std::shared_ptr<User> Users::Emplace(const uint64_t twitch_id,
                                     const Channel &channel,
                                     const std::string &ip) {
  auto user = std::make_shared<User>(db_, twitch_id, channel, ip);

  {
    boost::unique_lock<boost::shared_mutex> write_lock(lock_);
    auto it = data_by_twitch_id_.find(user->GetTwitchID());
    if (it != data_by_twitch_id_.end()) {
      return it->second;
    }

    data_by_id_[user->GetID()] = user;
    data_by_twitch_id_[user->GetTwitchID()] = user;
  }

  bool saved = user->SaveNew();

  if (!saved) {
    boost::unique_lock<boost::shared_mutex> write_lock(lock_);

    data_by_id_.erase(user->GetID());
    data_by_twitch_id_.erase(user->GetTwitchID());

    return nullptr;
  }

  return user;
}

Status Users::Save(std::shared_ptr<User> user) {
  auto old_user = GetByID(user->GetID());

  if (old_user == nullptr) {
    return Status(StatusCode::VALIDATION_ERROR, "No user found.");
  }

  bool path_changed = user->GetChannel()->GetStreamPath() !=
                      old_user->GetChannel()->GetStreamPath();
  bool name_changed = !NameEqual()(user->GetName(), old_user->GetName());

  // optimistically acquire new path and name if they're available
  {
    boost::unique_lock<boost::shared_mutex> write_lock(lock_);

    if (name_changed && data_by_name_.count(user->GetName())) {
      return Status(StatusCode::VALIDATION_ERROR, "Name unavailable.");
    }

    if (path_changed && user->GetChannel()->HasStreamPath() &&
        data_by_stream_path_.count(user->GetChannel()->GetStreamPath())) {
      return Status(StatusCode::VALIDATION_ERROR, "Stream path unavailable.");
    }

    data_by_name_[user->GetName()] = old_user;
    data_by_stream_path_[user->GetChannel()->GetStreamPath()] = old_user;
  }

  bool saved = user->Save();

  boost::unique_lock<boost::shared_mutex> write_lock(lock_);

  if (saved) {
    data_by_id_[user->GetID()] = user;
    data_by_twitch_id_[user->GetTwitchID()] = user;
    data_by_name_[user->GetName()] = user;
    data_by_stream_path_[user->GetChannel()->GetStreamPath()] = user;

    if (name_changed && !old_user->GetName().empty()) {
      data_by_name_.erase(old_user->GetName());
    }

    const auto &old_channel = old_user->GetChannel()->GetStreamPath();
    if (path_changed && old_user->GetChannel()->HasStreamPath()) {
      data_by_stream_path_.erase(old_user->GetChannel()->GetStreamPath());
    }

    return Status::OK;
  }

  if (name_changed && !user->GetName().empty()) {
    data_by_name_.erase(user->GetName());
  }
  if (path_changed && user->GetChannel()->HasStreamPath()) {
    data_by_stream_path_.erase(user->GetChannel()->GetStreamPath());
  }

  return Status(StatusCode::DB_ENGINE_ERROR, "Error saving changes.");
}

void Users::WriteJSON(rapidjson::Writer<rapidjson::StringBuffer> *writer) {
  boost::shared_lock<boost::shared_mutex> read_lock(lock_);

  writer->StartArray();
  for (const auto &it : data_by_id_) {
    it.second->WriteJSON(writer);
  }
  writer->EndArray();
}

}  // namespace rustla2
