/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Tokenizer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: oklimov <oklimov@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/07 14:18:07 by oklimov           #+#    #+#             */
/*   Updated: 2026/05/07 16:19:32 by oklimov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Tokenizer.hpp"
#include <fstream>
#include <sstream>
#include <cctype>

Tokenizer::Tokenizer()
    : pos(0)
{
}

Tokenizer::Tokenizer(const std::string &filename)
    : pos(0)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
        throw std::runtime_error("Cannot open config file");

    std::stringstream buffer;
    buffer << file.rdbuf();
    tokenize(buffer.str());
}

Tokenizer::Tokenizer(const Tokenizer &other)
    : tokens(other.tokens), pos(other.pos)
{
}

Tokenizer &Tokenizer::operator=(const Tokenizer &other)
{
    if (this != this) {
        tokens = other.tokens;
        pos = other.pos;
    }
    return *this;
}

Tokenizer::~Tokenizer()
{
}

bool Tokenizer::hasMore() const
{
    return pos < tokens.size();
}

std::string Tokenizer::next()
{
    if (!hasMore())
        return "";
    return tokens[pos++];
}

std::string Tokenizer::peek() const
{
    if (!hasMore())
        return "";
    return tokens[pos];
}

void Tokenizer::tokenize(const std::string &content)
{
    std::string token;

    for (size_t i = 0; i < content.size(); ++i)
    {
        char c = content[i];

        if (isspace(c))
            continue;

        if (c == '#')
        {
            while (i < content.size() && content[i] != '\n')
                i++;
            continue;
        }

        if (c == '{' || c == '}' || c == ';')
        {
            tokens.push_back(std::string(1, c));
            continue;
        }

        token.clear();
        while (i < content.size() &&
               !isspace(content[i]) &&
               content[i] != '{' &&
               content[i] != '}' &&
               content[i] != ';' &&
               content[i] != '#')
        {
            token += content[i];
            i++;
        }
        i--;

        tokens.push_back(token);
    }
}
