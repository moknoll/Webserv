NAME = libutils.a

SRC = string_utils.cpp \

OBJ = $(SRC:.cpp=.o)

HEADERS = utilities.hpp

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
AR = ar
ARFLAGS = rc
RM = rm -f

all: $(NAME)

$(NAME): $(OBJ)
	$(AR) $(ARFLAGS) $(NAME) $(OBJ)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re

