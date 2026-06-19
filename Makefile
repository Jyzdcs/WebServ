NAME    = webserv
CXX     = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS    = $(shell find src -name "*.cpp")
OBJS    = $(SRCS:src/%.cpp=obj/%.o)
INCS    = -I include

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCS) -c $< -o $@

clean:
	rm -rf obj

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re