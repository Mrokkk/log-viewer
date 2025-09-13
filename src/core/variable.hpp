#pragma once

#include <cstddef>
#include <functional>

#include "core/fwd.hpp"
#include "core/type.hpp"
#include "utils/buffer.hpp"
#include "utils/fwd.hpp"
#include "utils/noncopyable.hpp"

namespace core
{

struct Variable final : utils::NonCopyable
{
    enum class Access
    {
        readOnly,
        readWrite,
    };

    struct Value
    {
        Value(const long value)
            : type(Type::integer)
            , integer(value)
        {
        }

        Value(const std::string* value)
            : type(Type::string)
            , string(value)
        {
        }

        Value(const bool value)
            : type(Type::boolean)
            , boolean(value)
        {
        }

        Value(std::nullptr_t)
            : type(Type::null)
            , ptr(nullptr)
        {
        }

        Type                   type;
        union
        {
            const void*        ptr;
            const long         integer;
            const std::string* string;
            const bool         boolean;
        };
    };

    using Reader = std::function<Value(::core::Context& context)>;
    using Writer = std::function<bool(Value, ::core::Context& context)>;

    const char* name;
    const char* help;
    Type        type;
    Access      access;
    Reader      reader;
    Writer      writer;
};

struct Variables final
{
    Variables() = delete;

    static Variable* find(const std::string& name);
    static void $register(Variable variable);

    template <typename T>
    static void addUserDefined(const std::string& name, Type type, const T& value);
};

struct VariableWithContext final
{
    const Variable& variable;
    Context&        context;
};

#define READER() \
    ::core::Variable::Value Variable::reader([[maybe_unused]] ::core::Context& context)

#define WRITER() \
    bool Variable::writer([[maybe_unused]] ::core::Variable::Value value, [[maybe_unused]] ::core::Context& context)

#define DEFINE_READONLY_VARIABLE(NAME, TYPE, HELP) \
    namespace NAME \
    { \
        struct Variable \
        { \
            static ::core::Variable::Value reader(::core::Context& context); \
            static void init() \
            { \
                ::core::Variables::$register( \
                    ::core::Variable{ \
                        .name = #NAME, \
                        .help = HELP, \
                        .type = ::core::Type::TYPE, \
                        .access = ::core::Variable::Access::readOnly, \
                        .reader = &Variable::reader, \
                    }); \
            } \
            static inline bool registered = (Variable::init(), true); \
        }; \
    } \
    namespace NAME

#define DEFINE_READWRITE_VARIABLE(NAME, TYPE, HELP) \
    namespace NAME \
    { \
        struct Variable \
        { \
            static ::core::Variable::Value reader(::core::Context& context); \
            static bool writer(::core::Variable::Value value, ::core::Context& context); \
            static void init() \
            { \
                ::core::Variables::$register( \
                    ::core::Variable{ \
                        .name = #NAME, \
                        .help = HELP, \
                        .type = ::core::Type::TYPE, \
                        .access = ::core::Variable::Access::readWrite, \
                        .reader = &Variable::reader, \
                        .writer = &Variable::writer, \
                    }); \
            } \
            static inline bool registered = (Variable::init(), true); \
        }; \
    } \
    namespace NAME

utils::Buffer& operator<<(utils::Buffer& buf, const VariableWithContext& wrapped);
std::string getValueString(const Variable::Value& value);

}  // namespace core
