OBJDIR=build
OBJECTS= utils.o clause_cont.o staircaseEncoder.o
OBJS = $(patsubst %.o,$(OBJDIR)/%.o,$(OBJECTS))

SRCDIR=src

FLAGS= -Wall -Werror -Wextra -O3 -DNDEBUG
IGNORE_ASSERTVARS= -Wno-unused-but-set-variable
STANDARD= -std=c++11

all : $(OBJDIR)/main.o
	g++ $(FLAGS) $(OBJDIR)/main.o $(OBJS) -o build/abw_enc

$(OBJDIR)/main.o : main.cpp $(OBJS) $(SRCDIR)/staircaseEncoder.h
	g++ $(FLAGS) $(STANDARD) -c $< -o $@

$(OBJDIR)/staircaseEncoder.o : $(SRCDIR)/staircaseEncoder.cpp $(SRCDIR)/staircaseEncoder.h $(SRCDIR)/utils.h $(SRCDIR)/clause_cont.h
	g++ $(FLAGS) $(STANDARD) -c $< -o $@

$(OBJDIR)/clause_cont.o : $(SRCDIR)/clause_cont.cpp $(SRCDIR)/clause_cont.h $(SRCDIR)/utils.h
	g++ $(FLAGS) $(STANDARD) -c $< -o $@

$(OBJDIR)/utils.o : $(SRCDIR)/utils.cpp $(SRCDIR)/utils.h
	g++ $(FLAGS) $(STANDARD) -c $< -o $@

#.PHONY : clean
clean:
	rm -f *.a $(OBJDIR)/*.o *~ *.out  $(OBJDIR)/abw_enc

tar:
	tar cfv abw_enc.tar main.cpp makefile $(SRCDIR)/*.cpp $(SRCDIR)/*.h
