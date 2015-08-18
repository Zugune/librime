//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-4-6 Zou xu <zouivex@gmail.com>
//
#ifndef RIME_CONFIG_H_
#define RIME_CONFIG_H_

#include <iostream>
#include <type_traits>
#include <rime/common.h>
#include <rime/component.h>

namespace rime {

// config item base class
class ConfigItem {
 public:
  enum ValueType { kNull, kScalar, kList, kMap };

  ConfigItem() = default;  // null
  virtual ~ConfigItem() = default;

  ValueType type() const { return type_; }

 protected:
  ConfigItem(ValueType type) : type_(type) {}

  ValueType type_ = kNull;
};

class ConfigValue : public ConfigItem {
 public:
  ConfigValue() : ConfigItem(kScalar) {}
  ConfigValue(bool value);
  ConfigValue(int value);
  ConfigValue(double value);
  ConfigValue(const char* value);
  ConfigValue(const string& value);

  // schalar value accessors
  bool GetBool(bool* value) const;
  bool GetInt(int* value) const;
  bool GetDouble(double* value) const;
  bool GetString(string* value) const;
  bool SetBool(bool value);
  bool SetInt(int value);
  bool SetDouble(double value);
  bool SetString(const char* value);
  bool SetString(const string& value);

  const string& str() const { return value_; }

 protected:
  string value_;
};

class ConfigList : public ConfigItem {
 public:
  using Sequence = vector<a<ConfigItem>>;
  using Iterator = Sequence::iterator;

  ConfigList() : ConfigItem(kList) {}
  a<ConfigItem> GetAt(size_t i) const;
  a<ConfigValue> GetValueAt(size_t i) const;
  bool SetAt(size_t i, a<ConfigItem> element);
  bool Insert(size_t i, a<ConfigItem> element);
  bool Append(a<ConfigItem> element);
  bool Resize(size_t size);
  bool Clear();
  size_t size() const;

  Iterator begin();
  Iterator end();

 protected:
  Sequence seq_;
};

// limitation: map keys have to be strings, preferably alphanumeric
class ConfigMap : public ConfigItem {
 public:
  using Map = map<string, a<ConfigItem>>;
  using Iterator = Map::iterator;

  ConfigMap() : ConfigItem(kMap) {}
  bool HasKey(const string& key) const;
  a<ConfigItem> Get(const string& key) const;
  a<ConfigValue> GetValue(const string& key) const;
  bool Set(const string& key, a<ConfigItem> element);
  bool Clear();

  Iterator begin();
  Iterator end();

 protected:
  Map map_;
};

class ConfigData;
class ConfigListEntryRef;
class ConfigMapEntryRef;

class ConfigItemRef {
 public:
  ConfigItemRef(const a<ConfigData>& data) : data_(data) {
  }
  operator a<ConfigItem> () const {
    return GetItem();
  }
  ConfigListEntryRef operator[] (size_t index);
  ConfigMapEntryRef operator[] (const string& key);

  bool IsNull() const;
  bool IsValue() const;
  bool IsList() const;
  bool IsMap() const;

  bool ToBool() const;
  int ToInt() const;
  double ToDouble() const;
  string ToString() const;

  a<ConfigList> AsList();
  a<ConfigMap> AsMap();
  void Clear();

  // list
  bool Append(a<ConfigItem> item);
  size_t size() const;
  // map
  bool HasKey(const string& key) const;

  bool modified() const;
  void set_modified();

 protected:
  virtual a<ConfigItem> GetItem() const = 0;
  virtual void SetItem(a<ConfigItem> item) = 0;

  a<ConfigData> data_;
};

namespace {

template <class T>
a<ConfigItem> AsConfigItem(const T& x, const std::false_type&) {
  return New<ConfigValue>(x);
};

template <class T>
a<ConfigItem> AsConfigItem(const T& x, const std::true_type&) {
  return x;
};

}  // namespace

class ConfigListEntryRef : public ConfigItemRef {
 public:
  ConfigListEntryRef(a<ConfigData> data,
                     a<ConfigList> list, size_t index)
      : ConfigItemRef(data), list_(list), index_(index) {
  }
  template <class T>
  ConfigListEntryRef& operator= (const T& x) {
    SetItem(AsConfigItem(x, std::is_convertible<T, a<ConfigItem>>()));
    return *this;
  }
 protected:
  a<ConfigItem> GetItem() const {
    return list_->GetAt(index_);
  }
  void SetItem(a<ConfigItem> item) {
    list_->SetAt(index_, item);
    set_modified();
  }
 private:
  a<ConfigList> list_;
  size_t index_;
};

class ConfigMapEntryRef : public ConfigItemRef {
 public:
  ConfigMapEntryRef(a<ConfigData> data,
                    a<ConfigMap> map, const string& key)
      : ConfigItemRef(data), map_(map), key_(key) {
  }
  template <class T>
  ConfigMapEntryRef& operator= (const T& x) {
    SetItem(AsConfigItem(x, std::is_convertible<T, a<ConfigItem>>()));
    return *this;
  }
 protected:
  a<ConfigItem> GetItem() const {
    return map_->Get(key_);
  }
  void SetItem(a<ConfigItem> item) {
    map_->Set(key_, item);
    set_modified();
  }
 private:
  a<ConfigMap> map_;
  string key_;
};

inline ConfigListEntryRef ConfigItemRef::operator[] (size_t index) {
  return ConfigListEntryRef(data_, AsList(), index);
}

inline ConfigMapEntryRef ConfigItemRef::operator[] (const string& key) {
  return ConfigMapEntryRef(data_, AsMap(), key);
}

// Config class

class Config : public Class<Config, const string&>, public ConfigItemRef {
 public:
  // CAVEAT: Config instances created without argument will NOT
  // be managed by ConfigComponent
  Config();
  virtual ~Config();
  // instances of Config with identical file_name share a copy of config data
  // that could be reloaded by ConfigComponent once notified changes to the file
  explicit Config(const string& file_name);

  bool LoadFromStream(std::istream& stream);
  bool SaveToStream(std::ostream& stream);
  bool LoadFromFile(const string& file_name);
  bool SaveToFile(const string& file_name);

  // access a tree node of a particular type with "path/to/key"
  bool IsNull(const string& key);
  bool IsValue(const string& key);
  bool IsList(const string& key);
  bool IsMap(const string& key);
  bool GetBool(const string& key, bool* value);
  bool GetInt(const string& key, int* value);
  bool GetDouble(const string& key, double* value);
  bool GetString(const string& key, string* value);

  a<ConfigItem> GetItem(const string& key);
  a<ConfigValue> GetValue(const string& key);
  a<ConfigList> GetList(const string& key);
  a<ConfigMap> GetMap(const string& key);

  // setters
  bool SetBool(const string& key, bool value);
  bool SetInt(const string& key, int value);
  bool SetDouble(const string& key, double value);
  bool SetString(const string& key, const char* value);
  bool SetString(const string& key, const string& value);
  // setter for adding / replacing items to the tree
  bool SetItem(const string& key, a<ConfigItem> item);

  template <class T>
  Config& operator= (const T& x) {
    SetItem(AsConfigItem(x, std::is_convertible<T, a<ConfigItem>>()));
    return *this;
  }

 protected:
  a<ConfigItem> GetItem() const;
  void SetItem(a<ConfigItem> item);
 };

// ConfigComponent class

class ConfigComponent : public Config::Component {
 public:
  ConfigComponent(const string& pattern) : pattern_(pattern) {}
  Config* Create(const string& config_id);
  string GetConfigFilePath(const string& config_id);
  const string& pattern() const { return pattern_; }

 private:
  string pattern_;
};

}  // namespace rime

#endif  // RIME_CONFIG_H_
