NAME		= Matt_daemon

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++17
INCLUDES	= -Iincludes

SRC_DIR		= srcs
OBJ_DIR		= objs

SRCS		= main.cpp \
			  Tintin_reporter.cpp \
			  Daemon.cpp \
			  Server.cpp

OBJS		= $(addprefix $(OBJ_DIR)/, $(SRCS:.cpp=.o))
DEPS		= $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)



re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re