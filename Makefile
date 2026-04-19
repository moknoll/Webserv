NAME		:= webserv

CXX			:= c++
CXXFLAGS	:= -Wall -Wextra -Werror -std=c++98 -g
RM			:= rm -f
RMDIR		:= rm -rf
MKDIR		:= mkdir -p
OBJDIR		:= .build

SRC			:= src/main.cpp \
			   src/server/server.cpp \
			   src/ConfigParser/ConfigParser.cpp \
			   src/ConfigParser/ServerConfig.cpp \
			#    src/http/request/HttpHeader.cpp \

OBJ			:= $(OBJDIR)/main.o \
			   $(OBJDIR)/server/server.o \
			   $(OBJDIR)/ConfigParser/ConfigParser.o \
			   $(OBJDIR)/ConfigParser/ServerConfig.o \
			#    $(OBJDIR)/http/request/HttpHeader.o \

LIB_NAME	:= libws.a
LIB_DIR		:= src/lib/
LIB			:= $(LIB_DIR)$(LIB_NAME)


#################################################
CLIENT 		:= client.exe
CLIENT_DIR	:=	src/client/

################################################

# OBJ			:= $(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SRC))

all: $(NAME) $(CLIENT)

$(NAME): $(OBJ) $(LIB)
	$(CXX) $(CXXFLAGS) $(OBJ) $(LIB) -o $(NAME)

# $(OBJDIR)/%.o: src/%.cpp | $(OBJDIR)
$(OBJDIR)/%.o: src/%.cpp
	$(MKDIR) $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# $(OBJDIR):
	# $(MKDIR) $(OBJDIR)
	# $(MKDIR) $(dir $@)

$(LIB):
	# @make -C $(LIB_DIR) --no-print-directory
	@make -C $(LIB_DIR)

$(CLIENT):
	@make -C $(CLIENT_DIR)

clean:
	$(RM) $(OBJ)
	$(RMDIR) $(OBJDIR)
	@make clean -C $(LIB_DIR) --no-print-directory

fclean: clean
	$(RM) $(NAME)
	@make fclean -C $(LIB_DIR) --no-print-directory
	@make fclean -C $(CLIENT_DIR) --no-print-directory

re: fclean all

.PHONY: clean fclean, all, re
