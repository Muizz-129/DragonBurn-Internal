#pragma once
#include <string>
#include <array>
#include <utility>

namespace Security {
    template <size_t N, size_t Key>
    class XorString {
    private:
        std::array<char, N> buffer;

        // Compile-time XOR logic
        static constexpr char crypt(char c, size_t i) {
            return c ^ static_cast<char>((Key + (i * 7)) % 255);
        }

    public:
        // Compile-time constructor: the original text is XORed before the build is ready
        template <size_t... Is>
        constexpr XorString(const char(&str)[N], std::index_sequence<Is...>)
            : buffer{ crypt(str[Is], Is)... } {}

        // Decrypt the temporary string at runtime
        std::string decrypt() const {
            std::string res;
            res.reserve(N);
            for (size_t i = 0; i < N - 1; ++i) {
                res.push_back(crypt(buffer[i], i));
            }
            return res;
        }
    };
}

// A macro to simplify usage using a unique key based on the code line number.
#define _xor_(s) (Security::XorString<sizeof(s), (__LINE__ + 137)>(s, std::make_index_sequence<sizeof(s)>()).decrypt())