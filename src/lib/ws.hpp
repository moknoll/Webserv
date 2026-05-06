/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ws.hpp                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmagomad <nmagomad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/01 13:20:06 by nmagomad          #+#    #+#             */
/*   Updated: 2026/05/01 13:20:07 by nmagomad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <cstddef>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

class ws
{
  public:
	/**
	 * Splits a string into a vector of substrings based on a specific
	 * delimiter.
	 * * @param s The input string to be split.
	 * @param delim The delimiter string used to identify split points.
	 * @return A vector of strings containing the extracted tokens.
	 */
	static std::vector< std::string > strSplit(const std::string& s,
	                                           const std::string& delim);

	/**
	 * Removes leading and trailing whitespace characters from a string.
	 * * @param s The input string to be trimmed.
	 * @return A new string with all leading and trailing whitespaces removed.
	 */
	static std::string                strip(const std::string& s);

	/**
	 * @brief Converts a string to a size_t integer.
	 *
	 * This function extracts a size_t value from the beginning of the input
	 * string using std::istringstream. Leading whitespace is skipped, and
	 * parsing stops at the first non-digit character.
	 *
	 * @param s The input string to convert.
	 * @return size_t The converted unsigned integer value.
	 *
	 * @example
	 * size_t a = stosize("123");      // 123
	 * size_t b = stosize("  456abc"); // 456
	 * size_t c = stosize("abc");      // 0 (no error reported)
	 */
	static size_t                     stosize(const std::string& s);

	// convert string to tolower case string
	static void                       toLowerCase(std::string& s);

	/**
	 * @brief Retrieves the size of a file in bytes.
	 * @param path Path to the file.
	 * @return File size in bytes if successful, 0 otherwise.
	 * @note Returns 0 for directories, non-existent files, or files without
	 * read permissions. Use checkFile() first to distinguish actual zero-byte
	 *       files from error conditions.
	 */
	static size_t                     getFileSize(const std::string& path);
	/**
	 * @brief Checks whether a given path refers to a regular file.
	 * @param path The file system path to check.
	 * @return true if the path exists and is a regular file, false otherwise.
	 * @note Uses stat() and S_ISREG macro. Returns false for directories,
	 *       symlinks, special files, or non-existent paths.
	 */
	static bool                       isFile(const std::string& path);

	/**
	 * @brief Checks whether a given path refers to a directorie.
	 * @param path The file system path to check.
	 * @return true if the path exists and is a directorie, false otherwise.
	 * @note Uses stat() and S_ISREG macro. Returns false for directories,
	 *       symlinks, special files, or non-existent paths.
	 */
	static bool                       isDirectory(const std::string& path);

	/**
	 * @brief Extracts and normalizes the file extension from a given file path.
	 * @param path The file path (e.g., "/home/file.TXT" or "archive.tar.gz").
	 * @return The lowercase file extension without the dot (e.g., "txt", "gz").
	 *         Returns an empty string if no valid extension is found.
	 * @note Ignores dots in directory names. Only the last dot after the last
	 * slash is considered as the extension delimiter.
	 */
	static const std::string          getFileExtension(const std::string& path);

	/**
	 * @brief Reads the entire contents of a file into a string.
	 * @param path: Path to the file to read.
	 * @param file_content: Reference to a string where the file content will be
	 * stored.
	 * @return true if the file was successfully opened and read, false
	 * otherwise.
	 * @note Clears any existing content in 'out' and replaces it with file
	 * data. Uses stream iterators, so it reads until EOF. Large files may cause
	 *       memory allocation issues.
	 */
	static bool readFile(const char* path, std::string& file_content);

	/**
	 * @brief Checks a file's existence and accessibility without reading it.
	 * @param path Path to the file to check.
	 * @return Status code: FILE_OK if accessible, or
	 *                an error code (NOT_FOUND, PERMISSION, IS_DIR, UNKNOWN)
	 *                indicating the failure reason.
	 * @note Uses open() in read-only mode. The file is closed immediately
	 *       if successfully opened, so no resources are leaked.
	 */
	static int  checkFile(const char* path);

	/**
	 * @brief Gets the last modification time of a file as a formatted string.
	 * @param path Path to the file.
	 * @return Formatted time string (e.g., "01-Jan-2024 15:30") or empty string
	 * on error.
	 * @note Format used: day-month-year hour:minute (e.g., "05-May-2026
	 * 14:23"). Returns empty string if the file doesn't exist, stat() fails, or
	 *       formatting fails. Uses local time (not UTC).
	 */
	static const std::string getFileModificationTime(const std::string& path);

	/**
	 * @brief Converts a value of any type to its string representation.
	 *
	 * Convert the given value to a
	 * string. It works for any type that supports the stream insertion operator
	 * (operator<<).
	 *
	 * @tparam T The type of the value to convert. Must be stream-insertable.
	 * @param value The value to be converted to a string.
	 * @return std::string The string representation of the value.
	 *
	 * @example
	 * std::string s1 = to_string(42);          // "42"
	 * std::string s2 = to_string(3.14);        // "3.14"
	 */
	template < typename T >
	static std::string to_string(const T& value)
	{
		std::ostringstream oss;
		oss << value;
		return oss.str();
	}
};

