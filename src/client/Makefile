NAME		:= client.exe

CXX			:= c++
CXXFLAGS	:= -Wall -Wextra -Werror -std=c++98
RM			:= rm -f
RMDIR		:= rm -rf
MKDIR		:= mkdir
ROOTDIR		:= ../../
OBJDIR		:= .build

SRC			:=  client.cpp

OBJ			:= $(patsubst %.cpp,$(OBJDIR)/%.o,$(SRC))

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(ROOTDIR)$(NAME)

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	
$(OBJDIR):
	$(MKDIR) $(OBJDIR)

clean:
	$(RM) $(OBJ)
	$(RMDIR) $(OBJDIR)

fclean: clean
	$(RM) $(ROOTDIR)$(NAME)

re: fclean all

.PHONY: clean fclean, all, re
