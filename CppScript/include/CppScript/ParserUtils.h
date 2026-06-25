#pragma once

#include <array>
#include <string_view>
#include <initializer_list>
#include <cassert>
#include <algorithm>

namespace CppScript
{

template <typename T, unsigned short N, auto H>
class TokenMap
{
public:
    static constexpr unsigned short hashSize{ N };

    struct TokenData
    {
        std::string_view token;
        T data;
    };

    constexpr explicit TokenMap(std::initializer_list<TokenData> data)
#if defined(TESTING_ENABLED)
        : tokens(data)
#endif
    {
        assert(data.size() <= N && "TokenMap initialization failed: too many tokens for the given size");
        assert(validateTokens(Tokens(data)) && "TokenMap initialization failed: invalid or duplicit tokens");
        assert(validateHash(Tokens(data))
            && "TokenMap initialization failed: invalid hash function, run unit tests for hash suggestion");
        for (const auto& item : data)
            tokenData[calculateHash(item.token)] = std::move(item);
    }

    constexpr const T& operator[](const std::string_view token) const
    {
        assert(std::find_if(tokenData.begin(), tokenData.end(),
            [token](const TokenData& data) { return data.token == token; }) != tokenData.end()
            && "Token not found in TokenMap");
        assert(calculateHash(token) < N && "Token hash out of bounds");
        assert(tokenData[calculateHash(token)].token == token && "Token hash points to a different token");
        return tokenData[calculateHash(token)].data;
    }

protected:
    constexpr TokenMap(const TokenMap& other) = default;

    static constexpr unsigned short calculateHash(const std::string_view token)
    {
        return H(token);
    }

    class Tokens
    {
    public:
        constexpr Tokens(std::initializer_list<TokenData> data)
        {
             for (const auto& item : data)
                tokens[count++] = item.token;
        }

        constexpr auto begin()
        {
            return tokens.begin();
        }

        constexpr auto end()
        {
            return std::next(tokens.begin(), count);
        }

    private:
        std::array<std::string_view, N> tokens{};
        std::size_t count{ 0 };
    };

#if defined(TESTING_ENABLED)
    Tokens tokens;
#endif

private:
    static constexpr bool validateTokens(Tokens tokens)
    {
        std::sort(tokens.begin(), tokens.end());
        const std::string_view* prevToken{ nullptr };
        return std::all_of(tokens.begin(), tokens.end(),
            [&prevToken](const std::string_view& token)
            {
                if (token.empty() || (prevToken != nullptr && token == *prevToken))
                    return false;
                prevToken = &token;
                return true;
            });
    }

    static constexpr bool validateHash(Tokens tokens)
    {
        return std::all_of(tokens.begin(), tokens.end(),
            [](const std::string_view& token)
            {
                return calculateHash(token) < N;
            });
    }

    std::array<TokenData, N> tokenData{};
};

}