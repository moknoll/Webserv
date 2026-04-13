# Compiler
CXX = c++
CXX_FLAGS = -Wall -Werror -Wextra -std=c++98
INCLUDE_FLAGS = -I./src

# Sources
SERVER_SRC = src/main.cpp \
			 src/server/server.cpp \
			 src/ConfigParser/ConfigParser.cpp \
			 src/ConfigParser/ServerConfig.cpp

CLIENT_SRC = src/server/client.cpp

# Objects
SERVER_OBJ = obj/server/main.o \
			 obj/server/server.o \
			 obj/ConfigParser/ConfigParser.o \
			 obj/ConfigParser/ServerConfig.o

CLIENT_OBJ = obj/server/client.o

# Executables
SERVER_NAME = server
CLIENT_NAME = client

# Rules
all: $(SERVER_NAME) $(CLIENT_NAME)

$(SERVER_NAME): $(SERVER_OBJ)
	$(CXX) $(CXX_FLAGS) $(SERVER_OBJ) -o $(SERVER_NAME)

$(CLIENT_NAME): $(CLIENT_OBJ)
	$(CXX) $(CXX_FLAGS) $(CLIENT_OBJ) -o $(CLIENT_NAME)

obj/server/%.o: src/server/%.cpp
	@mkdir -p obj/server
	$(CXX) $(CXX_FLAGS) $(INCLUDE_FLAGS) -c $< -o $@

obj/ConfigParser/%.o: src/ConfigParser/%.cpp
	@mkdir -p obj/ConfigParser
	$(CXX) $(CXX_FLAGS) $(INCLUDE_FLAGS) -c $< -o $@

obj/server/main.o: src/main.cpp
	@mkdir -p obj/server
	$(CXX) $(CXX_FLAGS) $(INCLUDE_FLAGS) -c $< -o $@

clean:
	rm -rf obj/

fclean: clean
	rm -f $(SERVER_NAME) $(CLIENT_NAME)

re: fclean all

.PHONY: all clean fclean re
