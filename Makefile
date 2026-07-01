NAME		:= webserv

CXX			:= c++
CXXFLAGS	:= -Wall -Wextra -Werror -std=c++98 -g
RM			:= rm -f
RMDIR		:= rm -rf
MKDIR		:= mkdir -p
OBJDIR		:= .build

SRC			:= src/main.cpp \
			   src/server/Server.cpp \
			   src/server/Core.cpp \
			   src/server/Socket.cpp \
			   src/server/Client.cpp \
			   src/cgi/cgi.cpp \
			   src/ConfigParser/ConfigParser.cpp \
			   src/ConfigParser/ServerConfig.cpp \
			   src/ConfigParser/Tokenizer.cpp \
			   src/http/HttpRequest.cpp \
			   src/http/HttpResponse.cpp \
			   src/http/HttpHandler.cpp \
			   src/logger/Logger.cpp \

OBJ			:= $(OBJDIR)/main.o \
			   $(OBJDIR)/server/Server.o \
			   $(OBJDIR)/server/Core.o \
			   $(OBJDIR)/server/Socket.o \
			   $(OBJDIR)/server/Client.o \
			   $(OBJDIR)/cgi/cgi.o \
			   $(OBJDIR)/ConfigParser/ConfigParser.o \
			   $(OBJDIR)/ConfigParser/ServerConfig.o \
			   $(OBJDIR)/ConfigParser/Tokenizer.o \
			   $(OBJDIR)/http/HttpRequest.o \
			   $(OBJDIR)/http/HttpResponse.o \
			   $(OBJDIR)/http/HttpHandler.o \
			   $(OBJDIR)/logger/Logger.o \

LIB_NAME	:= libws.a
LIB_DIR		:= src/lib/
LIB			:= $(LIB_DIR)$(LIB_NAME)


# OBJ			:= $(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SRC))

all: $(NAME)

$(NAME): $(OBJ) $(LIB)
	$(CXX) $(CXXFLAGS) $(OBJ) $(LIB) -o $(NAME)

# $(OBJDIR)/%.o: src/%.cpp | $(OBJDIR)
$(OBJDIR)/%.o: src/%.cpp
	$(MKDIR) $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# $(OBJDIR):
	# $(MKDIR) $(OBJDIR)
	# $(MKDIR) $(dir $@)

run: $(NAME)
	./webserv | cat -e

$(LIB):
	# @make -C $(LIB_DIR) --no-print-directory
	@make -C $(LIB_DIR)

clean:
	$(RM) $(OBJ)
	$(RMDIR) $(OBJDIR)
	@make clean -C $(LIB_DIR) --no-print-directory

fclean: clean
	$(RM) $(NAME)
	@make fclean -C $(LIB_DIR) --no-print-directory

re: fclean all

.PHONY: clean fclean, all, re
