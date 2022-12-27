#include "serializer.h"

Serializer::Serializer() : data(nullptr), data_size(0)
{}

Serializer::Serializer(const std::string label, const char* data, const size_t data_size)
    : label(label), data_size(data_size)
{
    if (data)
    {
        this->data = new char[data_size];
        std::memcpy(this->data, data, data_size);
    } else
    {
        data = nullptr;
        data_size = 0;
    }
}

Serializer::Serializer(Serializer&& object)
    : label(std::move(object.getLabel())), data_size(object.getDataSize())
{
    if (object.data)
    {
        this->data = object.data;
        this->data_size = object.getDataSize();
        object.data = nullptr;
        object.data_size = 0;
    } else
    {
        data = nullptr;
        data_size = 0;
        for (auto& obj: object.objects)
        {
            objects.emplace_back(std::move(obj));
        }
        object.objects.clear();
    }
}

Serializer::Serializer(const char* data, size_t size)
{
    if (this->data)
    {
        delete[] data;
        data = nullptr;
        data_size = 0;
    }
    size_t readen = this->read(data, size);
    if (readen != size)
    {
        throw std::runtime_error("Object reading error");
    }
}

Serializer::~Serializer()
{
    if (data)
    {
        delete[] data;
    }
}

const std::string& Serializer::getLabel() const
{
    return label;
}

const std::list<Serializer>& Serializer::getObjects() const
{
    return objects;
}

size_t Serializer::unload(void* place, size_t size) const
{
    size_t _size = size < data_size ? size : data_size;
    std::memcpy(place, data, _size);
    return _size;
}

void Serializer::append(Serializer&& object)
{
    objects.push_back(std::move(object));
}

size_t Serializer::size() const
{
    // full size of serialized object
    size_t _size{7};
    _size += label.length();
    _size += this->getDataSize();
    return _size;
}

size_t Serializer::getDataSize() const
{
    if (data)
    {
        return data_size;
    } else
    {
        size_t _size{0};
        for (const auto& obj: objects)
        {
            _size += obj.size();
        }
        return _size;
    }
}

const Serializer* Serializer::getObject(const std::string& label) const
{
    for (const Serializer& obj: objects)
    {
        if (obj.label.compare(label) == 0)
        {
            return &obj;
        }
    }
    return nullptr;
}

Serializer* Serializer::getObject(const std::string& label)
{
    return const_cast<Serializer*>(((const Serializer*)this)->getObject(label));
}

size_t Serializer::write(char* place, const size_t size, unsigned char layer) const
{
// layer        1 byte ------------------------- the object begin -------
// lable size   1 byte
// label        1-255 byte max
// size of data or objects inside this layer - 4 bytes
// 1 if data folows or 0-byte if not   - 1 byte
// data 1 - [max value of 4 bytes] bytes or ---- new object begin -------
   size_t _size = this->size();
    if (size < _size)
    {
        return 0;
    }
    char* ptr = place;
    char* end = ptr + size;
    *ptr = layer;   // layer    1 byte
    ++ptr;
    *ptr = static_cast<unsigned char>(label.length());  // label size   1 byte
    ++ptr;
    for (uint32_t i = 0; i < label.length(); ++i)
    {
        *ptr = label[i];     //  label   1-255 bytes max
        ++ptr;
    }
    *((uint32_t*)(ptr)) = static_cast<uint32_t>(this->getDataSize());   // size of data or objects inside this layer - 4 bytes
    ptr += sizeof(uint32_t);
    *ptr = data ? 1 : 0;    // 1 if data folows or 0-byte if not    - 1 byte
    ++ptr;
    if (data)
    {
        std::memcpy(ptr, data, data_size);  // data 1 - [max value of 4 bytes] bytes
        ptr += data_size;
    } else
    {
        for (const auto& obj: objects)
        {
            size_t writen{0};
            writen = obj.write(ptr, end - ptr, layer + 1);  // the inside list of objects
            if (writen == 0)
            {
                throw std::runtime_error("Object writing error");
            }
            ptr += writen;
        }
    }
    return static_cast<size_t>(ptr - place);
}

size_t Serializer::read(const char* place, const size_t size)
{
    if (data)
    {
        delete[] data;
        data = nullptr;
    }
    data_size = 0;
    if (objects.size() > 0)
    {
        objects.clear();
    }
// layer        1 byte ------------------------- the object begin -------
// lable size   1 byte
// label        1-255 byte max
// size of data or objects inside this layer - 4 bytes
// 1 if data folows or 0-byte if not   - 1 byte
// data 1 - [max value of 4 bytes] bytes or ---- new object begin -------
    const char* ptr = place;
    const char* end = ptr + size;
    const char layer = *ptr;   // layer    1 byte
    ++ptr;
    const char label_size = *ptr; // label size   1 byte
    ++ptr;
    label = std::string();
    for (int i = 0; i < label_size; ++i)
    {
        label += *ptr;     //  label   1-255 bytes max
        ++ptr;
    }
    data_size = *((uint32_t*)(ptr));    // size of data or objects inside this layer - 4 bytes
    ptr += sizeof(uint32_t);
    char is_data = *ptr;    // 1 if data folows or 0-byte if not    - 1 byte
    ++ptr;
    if (is_data)
    {
        data = new char[data_size];
        std::memcpy(data, ptr, data_size);  // data 1 - [max value of 4 bytes] bytes
        ptr += data_size;
    } else
    {
        const char* end_of_data = ptr + data_size;
        while (ptr < end_of_data)
        {
            size_t readen{0};
            Serializer obj;
            readen = obj.read(ptr, end - ptr);  // the inside list of objects
            if (readen == 0)
            {
                throw std::runtime_error("Object reading error");
            }
            objects.emplace_back(std::move(obj));
            ptr += readen;
        }
    }
    return static_cast<size_t>(ptr - place);
}

void Serializer::save(const std::string& filename) const
{
    size_t size = this->size();
    char* data_place = new char[size];
    this->write(data_place, size, 0);
    std::ofstream outfile;
    outfile.open(filename, std::ios_base::binary);
    if (!outfile.is_open())
    {
        delete[] data_place;
        throw std::runtime_error("Open ofstream");
    }
    outfile.write(data_place, size);
    outfile.close();
    delete[] data_place;
}

void Serializer::open(const std::string& filename)
{
    std::ifstream infile;
    infile.open(filename, std::ios_base::binary);
    if (!infile.is_open())
    {
        throw std::runtime_error("Open ifstream");
    }
    infile.seekg(0, std::ios_base::end);
    size_t file_size = infile.tellg();
    infile.seekg(0, std::ios_base::beg);
    char* data_place = new char[file_size];
    infile.read(data_place, file_size);
    infile.close();
    this->read(data_place, file_size);
    delete[] data_place;
}

