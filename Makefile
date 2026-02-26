# Compiler
CXX = c++
CXX_FLAGS = -Wall -Werror -Wextra -std=c++98

# Sources
SERVER_SRC = server.cpp main.cpp
CLIENT_SRC = client.cpp

# Objects
SERVER_OBJ = $(SERVER_SRC:.cpp=.o)
CLIENT_OBJ = $(CLIENT_SRC:.cpp=.o)

# Executables
SERVER_NAME = server
CLIENT_NAME = client

# Rules
all: $(SERVER_NAME) $(CLIENT_NAME)

$(SERVER_NAME): $(SERVER_OBJ)
	$(CXX) $(CXX_FLAGS) $(SERVER_OBJ) -o $(SERVER_NAME)

$(CLIENT_NAME): $(CLIENT_OBJ)
	$(CXX) $(CXX_FLAGS) $(CLIENT_OBJ) -o $(CLIENT_NAME)

%.o: %.cpp
	$(CXX) $(CXX_FLAGS) -c $< -o $@

clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ)

fclean: clean
	rm -f $(SERVER_NAME) $(CLIENT_NAME)

re: fclean all

.PHONY: all clean fclean re
