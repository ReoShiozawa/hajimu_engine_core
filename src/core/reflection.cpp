/**
 * src/core/reflection.cpp — 型情報レジストリ実装
 */
#include <engine/core/reflection.hpp>
#include <engine/core/log.hpp>
#include <string>

namespace engine {

TypeRegistry& TypeRegistry::instance() {
    static TypeRegistry reg;
    return reg;
}

void TypeRegistry::register_type(TypeInfo info) {
    auto id = info.id;
    if (by_id_.contains(id)) return; // 二重登録防止
    by_name_[info.name] = id;
    std::string name_copy(info.name);
    by_id_.emplace(id, std::move(info));
    ENG_DEBUG("Reflection: registered type '%s'", name_copy.c_str());
}

const TypeInfo* TypeRegistry::find(TypeID id) const {
    auto it = by_id_.find(id);
    return it != by_id_.end() ? &it->second : nullptr;
}

const TypeInfo* TypeRegistry::find(std::string_view name) const {
    auto it = by_name_.find(name);
    if (it == by_name_.end()) return nullptr;
    return find(it->second);
}

} // namespace engine
