#include <fstream>
#include <cstring>
#include <stdexcept>
#include <string>
#include <list>

class Serializer
{
private:
    char* data;
    size_t data_size;
    std::string label;
    std::list<Serializer> objects;
public:
    Serializer();
    Serializer(const std::string label, const char* data, const size_t data_size);
    Serializer(Serializer&& object);
    Serializer(const char* data, size_t size);
    ~Serializer();
public:
    const std::string& getLabel() const;
    const std::list<Serializer>& getObjects() const;
    size_t unload(void* place, size_t size) const;
    void append(Serializer&& object);
    size_t size() const;
    size_t getDataSize() const;
    const Serializer* getObject(const std::string& label) const;
    Serializer* getObject(const std::string& label);
    size_t write(char* place, const size_t size, unsigned char layer) const;
    size_t read(const char* place, const size_t size);
    void save(const std::string& filename) const;
    void open(const std::string& filename);
};

