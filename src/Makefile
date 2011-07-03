OBJDIR=../obj/

$(OBJDIR)/ripv1: $(OBJDIR)/rip_simulation.o $(OBJDIR)/ripv1.o $(OBJDIR)/timer.o $(OBJDIR)/list.o $(OBJDIR)/mem.o
	cc -o $(OBJDIR)/ripv1 $(OBJDIR)/rip_simulation.o $(OBJDIR)/ripv1.o\
        $(OBJDIR)/timer.o $(OBJDIR)/list.o $(OBJDIR)/mem.o -lpthread -lm	

$(OBJDIR)/rip_simulation.o: rip_simulation.c rip.h list.h
	cc -c rip_simulation.c -o $(OBJDIR)/rip_simulation.o
	
$(OBJDIR)/ripv1.o: rip.c rip.h log.h timer.h list.h
	cc -c rip.c -o $(OBJDIR)/ripv1.o

$(OBJDIR)/timer.o: timer.c timer2.h mem1.h log.h
	cc -c timer.c  -o $(OBJDIR)/timer.o

$(OBJDIR)/list.o: list.c list.h
	cc -c list.c -o $(OBJDIR)/list.o

$(OBJDIR)/mem.o: mem_mgmt.c
	cc -c mem_mgmt.c -o $(OBJDIR)/mem.o

.PHONY : clean

clean: 
	rm -f $(OBJDIR)/*.o *~
	rm -f $(OBJDIR)/ripv1
