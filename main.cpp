/*
################################################################################
# Описание бинарного протокола
################################################################################

По сети ходят пакеты вида
packet : = size payload

size - размер последовательности, в количестве элементов, может быть 0.

payload - поток байтов(blob)
payload состоит из последовательности сериализованных переменных разных типов :

Описание типов и порядок их сериализации

type : = id(uint64_t) data(blob)

data : =
    IntegerType - uint64_t
    FloatType - double
    StringType - size(uint64_t) blob
    VectorType - size(uint64_t) ...(сериализованные переменные)

Все данные передаются в little endian порядке байтов

Необходимо реализовать сущности IntegerType, FloatType, StringType, VectorType
Кроме того, реализовать вспомогательную сущность Any
Сделать объект Serialisator с указанным интерфейсом.

Конструкторы(ы) типов IntegerType, FloatType и StringType должны обеспечивать инициализацию, аналогичную инициализации типов uint64_t, double и std::string соответственно.
Конструктор(ы) типа VectorType должен позволять инициализировать его списком любых из перечисленных типов(IntegerType, FloatType, StringType, VectorType) переданных как по ссылке, так и по значению.
Все указанные сигнатуры должны быть реализованы.
Сигнатуры шаблонных конструкторов условны, их можно в конечной реализации делать на усмотрение разработчика, можно вообще убрать.
Vector::push должен быть именно шаблонной функцией. Принимает любой из типов: IntegerType, FloatType, StringType, VectorType.
Serialisator::push должен быть именно шаблонной функцией.Принимает любой из типов: IntegerType, FloatType, StringType, VectorType, Any
Реализация всех шаблонных функций, должна обеспечивать constraint requirements on template arguments, при этом, использование static_assert - запрещается.
Код в функции main не подлежит изменению. Можно только добавлять дополнительные проверки.

Архитектурные требования :
1. Стаедарт - c++17
2. Запрещаются виртуальные функции.
3. Запрещается множественное и виртуальное наследование.
4. Запрещается создание каких - либо объектов в куче, в том числе с использованием умных указателей.
   Это требование не влечет за собой огранечений на использование std::vector, std::string и тп.
5. Запрещается любое дублирование кода, такие случаи должны быть строго обоснованы. Максимально использовать обобщающие сущности.
   Например, если в каждой из реализаций XType будет свой IdType getId() - это будет считаться ошибкой.
6. Запрещается хранение value_type поля на уровне XType, оно должно быть вынесено в обобщающую сущность.
7. Никаких других ограничений не накладывается, в том числе на создание дополнительных обобщающих сущностей и хелперов.
8. XType должны реализовать сериализацию и десериализацию аналогичную Any.

Пример сериализации VectorType(StringType("qwerty"), IntegerType(100500))
{0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x71,0x77,0x65,0x72,0x74,0x79,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x94,0x88,
 0x01,0x00,0x00,0x00,0x00,0x00}
*/

#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <type_traits>
#include <variant>
#include <cstring>
#include <limits>

using Id = uint64_t;
using Buffer = std::vector<std::byte>;

class IntegerType;
class FloatType;
class StringType;
class VectorType;


enum class TypeId : Id {
    Uint,
    Float,
    String,
    Vector
};

namespace tools {
    template<typename T>
    void serialize(T value, Buffer& buff) {
        if constexpr (std::is_same_v<T, double>) {
            const unsigned char* p = reinterpret_cast<const unsigned char*>(&value);
            for (size_t i = 0; i < sizeof(T); ++i) {
                buff.push_back(static_cast<std::byte>(p[i]));
            }
        }
        else {
            for (size_t i = 0; i < sizeof(T); ++i) {
                buff.push_back(static_cast<std::byte>((value >> (i * 8)) & 0xFF));
            }
        }
    }

    template<typename T>
    T deserialize(Buffer::const_iterator& it) {
        T value = 0;
        if constexpr (std::is_same_v<T, double>) {
            unsigned char bytes[sizeof(T)];
            for (size_t i = 0; i < sizeof(T); ++i) {
                bytes[i] = static_cast<unsigned char>(*it);
                ++it;
            }
            std::memcpy(&value, bytes, sizeof(T));
        }
        else {
            for (size_t i = 0; i < sizeof(T); ++i) {
                value |= static_cast<T>(static_cast<uint8_t>(*it)) << (i * 8);
                ++it;
            }
        }
        return value;
    }
}

namespace type_traits {
    template<typename> struct TypeInfo;

    template<> struct TypeInfo<IntegerType> {
        static constexpr TypeId id = TypeId::Uint;
    };

    template<> struct TypeInfo<FloatType> {
        static constexpr TypeId id = TypeId::Float;
    };

    template<> struct TypeInfo<StringType> {
        static constexpr TypeId id = TypeId::String;
    };

    template<> struct TypeInfo<VectorType> {
        static constexpr TypeId id = TypeId::Vector;
    };
}

template <typename T, typename ValueType>
class BaseType {

public:
    BaseType() :value_{} {}
    BaseType(ValueType value) :value_(value) {}

    TypeId getId() const {
        return type_traits::TypeInfo<T>::id;
    };

    void serialize(Buffer& buff) const {
        tools::serialize(static_cast<Id>(getId()), buff);
        if constexpr (std::is_arithmetic_v<ValueType>) {
            tools::serialize(value_, buff);
        }
        else {
            if constexpr (std::is_same_v<T, StringType>) {
                tools::serialize(static_cast<uint64_t>(value_.size()), buff);
                for (char c : value_) {
                    buff.push_back(static_cast<std::byte>(c));
                }
            }
            else {
                tools::serialize(static_cast<uint64_t>(value_.size()), buff);
                for (const auto& item : value_) {
                    item.serialize(buff);
                }
            }
        }
    }

    Buffer::const_iterator deserialize(Buffer::const_iterator begin, Buffer::const_iterator end);

    bool operator==(const T& other) const {
        return value_ == other.value_;
    }

protected:
    ValueType value_;

};

template <typename T, typename ValueType>
class NumericType : public BaseType<T, ValueType> {
public:
    using BaseType<T, ValueType>::BaseType;
};

class IntegerType : public NumericType<IntegerType, uint64_t> {
public:
    using NumericType::NumericType;
};

class FloatType : public NumericType<FloatType, double> {
public:
    using NumericType::NumericType;
};

class StringType : public BaseType<StringType, std::string> {
public:
    template<typename... Args,
        typename = std::enable_if_t<std::is_constructible_v<std::string, Args...>>>
    StringType(Args&&... args) : BaseType(std::string(std::forward<Args>(args)...)) {}
};

class Any;

class VectorType : public BaseType<VectorType, std::vector<Any>> {
public:
    VectorType() : BaseType(std::vector<Any>{}) {}

    explicit VectorType(std::vector<Any> vec) : BaseType(std::move(vec)) {}

    void reserve(size_t size) { this->value_.reserve(size); }

private:
    template<typename Arg>
    void push_back_impl(Arg&& val) {
        this->value_.emplace_back(std::forward<Arg>(val));
    }

};

class Any {
public:
    Any() : data_({}) {}

    template<typename T,
        typename = std::enable_if_t<
        std::is_same_v<std::decay_t<T>, IntegerType> ||
        std::is_same_v<std::decay_t<T>, FloatType> ||
        std::is_same_v<std::decay_t<T>, StringType> ||
        std::is_same_v<std::decay_t<T>, VectorType>>>
        Any(T&& val) : data_(std::forward<T>(val)) {}

    void serialize(Buffer& buff) const {
        std::visit([&buff](const auto& val) {
            val.serialize(buff);
            }, data_);
    }

    Buffer::const_iterator deserialize(Buffer::const_iterator begin, Buffer::const_iterator end) {
        if (std::distance(begin, end) < static_cast<std::ptrdiff_t>(sizeof(Id))) {
            throw std::runtime_error("Invalid buffer size");
        }

        auto it = begin;
        Id id = tools::deserialize<Id>(it);

        switch (static_cast<TypeId>(id)) {
        case TypeId::Uint: {
            IntegerType val;
            it = val.deserialize(begin, end);
            data_ = val;
            break;
        }
        case TypeId::Float: {
            FloatType val;
            it = val.deserialize(begin, end);
            data_ = val;
            break;
        }
        case TypeId::String: {
            StringType val;
            it = val.deserialize(begin, end);
            data_ = val;
            break;
        }
        case TypeId::Vector: {
            VectorType val;
            it = val.deserialize(begin, end);
            data_ = val;
            break;
        }
        default:
            throw std::runtime_error("Unknown type ID");
        }
        return it;
    }

    TypeId getPayloadTypeId() const {
        return std::visit([](const auto& val) {
            return val.getId();
            }, data_);
    }

    template<typename Type>
    auto& getValue() const {
        return std::get<Type>(data_).getValue();
    }

    template<TypeId kId>
    auto& getValue() const {
        using Type = std::conditional_t<kId == TypeId::Uint, IntegerType,
            std::conditional_t<kId == TypeId::Float, FloatType,
            std::conditional_t<kId == TypeId::String, StringType, VectorType>>>;
        return std::get<Type>(data_).getValue();
    }

    bool operator==(const Any& other) const {
        return data_ == other.data_;
    }

private:
    std::variant<IntegerType, FloatType, StringType, VectorType> data_;
};

template <typename T, typename ValueType>
Buffer::const_iterator BaseType<T,ValueType>::deserialize(Buffer::const_iterator begin, Buffer::const_iterator end)
{
    auto it = begin;
    if (std::distance(it, end) < static_cast<std::ptrdiff_t>(sizeof(Id))) {
        throw std::runtime_error("Buffer too small for type ID");
    }
    auto id = tools::deserialize<Id>(it);
    if (id != static_cast<Id>(getId())) {
        throw std::runtime_error("Type ID mismatch");
    }

    if constexpr (!std::is_arithmetic_v<ValueType>) {
        if (std::distance(it, end) < static_cast<std::ptrdiff_t>(sizeof(uint64_t))) {
            throw std::runtime_error("Buffer too small for size");
        }
        auto size = tools::deserialize<uint64_t>(it);
        if (std::distance(it, end) < static_cast<std::ptrdiff_t>(size)) {
            throw std::runtime_error("Buffer too small for data");
        }
        value_.clear();
        value_.reserve(size);
        if constexpr (std::is_same_v<T, StringType>) {
            for (size_t i = 0; i < size; ++i) {
                value_.push_back(static_cast<char>(static_cast<uint8_t>(*it)));
                ++it;
            }
        }
        else {
            for (size_t i = 0; i < size; ++i) {
                Any any;
                it = any.deserialize(it, end);
                value_.push_back(std::move(any));
            }
        }
    }
    else {
        if (std::distance(it, end) < static_cast<std::ptrdiff_t>(sizeof(ValueType))) {
            throw std::runtime_error("Buffer too small for value");
        }
        value_ = tools::deserialize<ValueType>(it);
    }
    return it;
}

class Serializator {
public:
    template<typename Arg,
        typename = std::enable_if_t<
        std::is_same_v<std::decay_t<Arg>, IntegerType> ||
        std::is_same_v<std::decay_t<Arg>, FloatType> ||
        std::is_same_v<std::decay_t<Arg>, StringType> ||
        std::is_same_v<std::decay_t<Arg>, VectorType> ||
        std::is_same_v<std::decay_t<Arg>, Any>
        >>
        void push(Arg&& val) {
        storage_.emplace_back(std::forward<Arg>(val));
    }

    Buffer serialize() const {
        Buffer buff;
        tools::serialize(static_cast<Id>(storage_.size()), buff);
        for (const auto& item : storage_) {
            item.serialize(buff);
        }
        return buff;
    }

    static std::vector<Any> deserialize(const Buffer& val) {
        Buffer::const_iterator it = val.begin();
        auto size = tools::deserialize<Id>(it);
        std::vector<Any> result;
        result.reserve(size);
        for (Id i = 0; i < size; ++i) {
            Any any;
            it = any.deserialize(it, val.end());
            result.push_back(any);
        }
        return result;
    }

    const std::vector<Any>& getStorage() const {
        return storage_;
    }

private:
    std::vector<Any> storage_;
};


int main() {

    std::ifstream raw;
    raw.open("raw.bin", std::ios_base::in | std::ios_base::binary);
    if (!raw.is_open())
        return 1;
    raw.seekg(0, std::ios_base::end);
    std::streamsize size = raw.tellg();
    raw.seekg(0, std::ios_base::beg);

    Buffer buff(size);
    raw.read(reinterpret_cast<char*>(buff.data()), size);

    std::cout << "Deserialize..." << std::endl;

    auto res = Serializator::deserialize(buff);

    Serializator s;
    for (auto&& i : res)
        s.push(i);

    std::cout << "Serialize..." << std::endl;
    std::cout << "Comparison result: " << std::boolalpha << (buff == s.serialize()) << '\n';

    return 0;
}