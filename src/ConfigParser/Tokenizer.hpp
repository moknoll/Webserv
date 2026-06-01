/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Tokenizer.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: oklimov <oklimov@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/07 14:18:00 by oklimov           #+#    #+#             */
/*   Updated: 2026/05/07 14:35:08 by oklimov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <vector>

class Tokenizer
{
private:
    std::vector<std::string> tokens;
    size_t pos;

    void tokenize(const std::string &content);

public:
    Tokenizer(const std::string &filename);

    // OCF
    Tokenizer();
    Tokenizer(const Tokenizer &other);
    Tokenizer &operator=(const Tokenizer &other);
    ~Tokenizer();

    bool hasMore() const;
    std::string next();
    std::string peek() const;
};
