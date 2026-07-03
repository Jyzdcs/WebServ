NAME    = webserv
CXX     = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

# main.cpp vit à la racine (pas dans src/) : le CI compile chaque test
# avec tous les fichiers de src/, il ne doit donc pas y avoir de main dedans.
SRCS    = $(shell find src -name "*.cpp")
OBJS    = $(SRCS:src/%.cpp=obj/%.o) obj/main.o
INCS    = -I include

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

obj/main.o: main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCS) -c $< -o $@

obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCS) -c $< -o $@

clean:
	rm -rf obj

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re