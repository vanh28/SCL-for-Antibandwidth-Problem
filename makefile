OBJDIR=build
OBJECTS= utils.o math_extension.o reduced_encoder.o sequential_encoder.o product_encoder.o duplex_encoder.o ladder_encoder.o encoder.o bdd.o clause_cont.o cadical_clauses.o antibandwidth_encoder.o
OBJS = $(patsubst %.o,$(OBJDIR)/%.o,$(OBJECTS))

SRCDIR=src

FLAGS= -Wall -Werror -Wextra -O3 -DNDEBUG
IGNORE_ASSERTVARS= -Wno-unused-but-set-variable
STANDARD= -std=c++11

CADICAL_INC=./cadical/
CADICAL_LIB_DIR=./cadical/
CADICAL_LIB=-lcadical

all : $(OBJDIR)/main.o
	g++ $(FLAGS) $(OBJDIR)/main.o $(OBJS) -L$(CADICAL_LIB_DIR) $(CADICAL_LIB) -o build/abw_enc

$(OBJDIR)/main.o : main.cpp $(OBJS) $(SRCDIR)/antibandwidth_encoder.h
	g++ $(FLAGS) $(STANDARD) -I$(CADICAL_INC) -c $< -o $@

$(OBJDIR)/antibandwidth_encoder.o : $(SRCDIR)/antibandwidth_encoder.cpp $(SRCDIR)/antibandwidth_encoder.h $(SRCDIR)/reduced_encoder.h $(SRCDIR)/sequential_encoder.h $(SRCDIR)/product_encoder.h $(SRCDIR)/duplex_encoder.h $(SRCDIR)/ladder_encoder.h $(SRCDIR)/utils.h $(SRCDIR)/math_extension.h $(SRCDIR)/clause_cont.h $(SRCDIR)/cadical_clauses.h
	g++ $(FLAGS) $(STANDARD) -I$(CADICAL_INC) -c $< -o $@

$(OBJDIR)/reduced_encoder.o : $(SRCDIR)/reduced_encoder.cpp $(SRCDIR)/reduced_encoder.h $(SRCDIR)/encoder.h
	g++ $(FLAGS) $(STANDARD) -c $< -o $@

$(OBJDIR)/product_encoder.o : $(SRCDIR)/product_encoder.cpp $(SRCDIR)/product_encoder.h $(SRCDIR)/encoder.h
	g++ $(FLAGS) $(STANDARD) -c $< -o $@

$(OBJDIR)/sequential_encoder.o : $(SRCDIR)/sequential_encoder.cpp $(SRCDIR)/sequential_encoder.h $(SRCDIR)/encoder.h
	g++ $(FLAGS) $(STANDARD) -c $< -o $@

$(OBJDIR)/duplex_encoder.o : $(SRCDIR)/duplex_encoder.cpp $(SRCDIR)/duplex_encoder.h $(SRCDIR)/encoder.h $(SRCDIR)/bdd.h
	g++ $(FLAGS) $(IGNORE_ASSERTVARS) $(STANDARD) -c $< -o $@
	
$(OBJDIR)/ladder_encoder.o : $(SRCDIR)/ladder_encoder.cpp $(SRCDIR)/ladder_encoder.h $(SRCDIR)/encoder.h $(SRCDIR)/math_extension.h 
	g++ $(FLAGS) $(STANDARD) -c $< -o $@

$(OBJDIR)/cadical_clauses.o : $(SRCDIR)/cadical_clauses.cpp $(SRCDIR)/cadical_clauses.h $(SRCDIR)/clause_cont.h
	g++ $(FLAGS) $(STANDARD) -I$(CADICAL_INC) -c $< -o $@

$(OBJDIR)/clause_cont.o : $(SRCDIR)/clause_cont.cpp $(SRCDIR)/clause_cont.h $(SRCDIR)/utils.h
	g++ $(FLAGS) $(STANDARD) -c $< -o $@

$(OBJDIR)/encoder.o : $(SRCDIR)/encoder.cpp $(SRCDIR)/encoder.h $(SRCDIR)/clause_cont.h
	g++ $(FLAGS) $(STANDARD) -c $< -o $@

$(OBJDIR)/utils.o : $(SRCDIR)/utils.cpp $(SRCDIR)/utils.h
	g++ $(FLAGS) $(STANDARD) -c $< -o $@
	
$(OBJDIR)/math_extension.o : $(SRCDIR)/math_extension.cpp $(SRCDIR)/math_extension.h
	g++ $(FLAGS) $(STANDARD) -c $< -o $@

$(OBJDIR)/bdd.o : $(SRCDIR)/bdd.cpp $(SRCDIR)/bdd.h
	g++ $(FLAGS) $(IGNORE_ASSERTVARS) $(STANDARD) -c $< -o $@

#.PHONY : clean
clean:
	rm -f *.a $(OBJDIR)/*.o *~ *.out  $(OBJDIR)/abw_enc

tar:
	tar cfv abw_enc.tar main.cpp makefile $(SRCDIR)/*.cpp $(SRCDIR)/*.h cadical/*.a cadical/*.hpp
