/**
 * engine/core/reflection.hpp — コンパイル時+ランタイム リフレクション
 *
 * TypeInfo でメンバのオフセット・サイズ・名前を保持。
 * シリアライズ / hajimuバインディング / エディタ連携の基盤。
 */
#pragma once

#include "types.hpp"
#include <string_view>
#include <vector>
#include <unordered_map>

namespace engine {

// ── フィールド記述子 ────────────────────────────────────
struct FieldInfo {
    std::string_view name;
    TypeID           type_id;
    usize            offset;
    usize            size;
};

// ── 型情報 ──────────────────────────────────────────────
struct TypeInfo {
    std::string_view name;
    TypeID           id;
    usize            size;
    usize            alignment;
    std::vector<FieldInfo> fields;

    // 指定フィールドへのポインタ取得
    template <typename T>
    T* field_ptr(void* instance, std::string_view field_name) const {
        for (auto& f : fields) {
            if (f.name == field_name) {
                return reinterpret_cast<T*>(static_cast<u8*>(instance) + f.offset);
            }
        }
        return nullptr;
    }
};

// ── グローバル型レジストリ ──────────────────────────────
class TypeRegistry {
public:
    static TypeRegistry& instance();

    void register_type(TypeInfo info);

    [[nodiscard]] const TypeInfo* find(TypeID id) const;
    [[nodiscard]] const TypeInfo* find(std::string_view name) const;
    [[nodiscard]] const auto& all() const { return by_id_; }

private:
    TypeRegistry() = default;
    std::unordered_map<TypeID, TypeInfo>     by_id_;
    std::unordered_map<std::string_view, TypeID> by_name_;
};

// ── 登録マクロ ──────────────────────────────────────────
#define ENG_REFLECT_BEGIN(T) \
    namespace { struct _Reflect_##T { _Reflect_##T() { \
        ::engine::TypeInfo info; \
        info.name = #T; \
        info.id = ::engine::type_id<T>(); \
        info.size = sizeof(T); \
        info.alignment = alignof(T);

#define ENG_REFLECT_FIELD(T, field) \
        info.fields.push_back({#field, ::engine::type_id<decltype(T::field)>(), \
            offsetof(T, field), sizeof(T::field)});

#define ENG_REFLECT_END(T) \
        ::engine::TypeRegistry::instance().register_type(std::move(info)); \
    }}; static _Reflect_##T _reflect_instance_##T; }

} // namespace engine
