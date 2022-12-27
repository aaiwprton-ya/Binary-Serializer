TARGET = serializer.a

$(TARGET): serializer.o
	ar rcs $@ $^

serializer.o: serializer.cpp serializer.h
	g++ -c -o $@ $<

clean:
	rm -f *.o $(TARGET)
