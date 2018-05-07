
#include <map>
#include <iostream>
#include <string>
#include <sstream>

template <typename T>
class CUMap {
    typedef std::map<std::string, T> StringTMap_t;
public:
    CUMap(void) {}
    
    ~CUMap(void) {}
    
    std::string str(void) const {
        std::stringstream ss;
        typename StringTMap_t::const_iterator it = _m_mapT.begin();
        for ( ; it != _m_mapT.end(); ++ it) {
            ss << it->first << ":" << it->second << ", ";
        }
        return ss.str();
    }
    
    void insert(const std::string & key, const T & value) {
        _m_mapT.insert(typename StringTMap_t::value_type(key, value));
    }
    
    T get(const std::string & key) {
        typename StringTMap_t::const_iterator it = _m_mapT.find(key);
        if (it != _m_mapT.end()) {
            return it->second;
        }
        return _defaultT;
    }
private:
    StringTMap_t _m_mapT;
    const static T _defaultT;
};

int main(void)
{
    CUMap<int> cumap;
    
    cumap.insert("a", 1);
    cumap.insert("b", 2);
    cumap.insert("c", 3);
    
    std::cout << cumap.str() << std::endl;
    
    return 0;
}
