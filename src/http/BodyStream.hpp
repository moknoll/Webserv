#pragma once

#include <cstddef>
#include <string>

class BodyStream
{
  public:
	enum State
	{
		sw_start = 0,
		sw_header,
		sw_data,
		sw_end
	};

	BodyStream();
	// BodyStream(const BodyStream& other);
	~BodyStream();

	// BodyStream& operator=(const BodyStream& other);

	void               setBoundary(const std::string& boundary);
	void               parse(std::string& buf_);
	bool               findBoundary(size_t& pos, const std::string& delim);
	bool               eof() const;
	const std::string& getFileName() const;
	const std::string& getData() const;

	void               printBodyStream() const;
	void               reset();

  private:
	std::string boundary_;
	std::string delimeter_;
	std::string end_delimeter_;
	std::string buf_;
	std::string name_;
	std::string filename_;

	State       state_;
	std::string data_;

	bool        parseHeaders_(const std::string& headers);

	void        parseMultiPart(std::string& raw_data);
	void        parseChunked(std::string& raw_data);
};

