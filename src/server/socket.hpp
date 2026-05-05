#pragma once
 

class Socket{
	private: 
		int sockFd;
		int port; 

	public: 
		Socket();
		Socket(int port, int sockFd);
		Socket operator=(const Socket &obj);
		~Socket();

		int getFd()const;
		int getPort()const;
		void setFD();
		void setPort();
};	