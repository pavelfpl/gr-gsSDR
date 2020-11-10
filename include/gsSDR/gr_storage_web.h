#ifndef GR_STORAGE_WEB_H
#define GR_STORAGE_WEB_H

#include <string> 

class gr_storage_web{

public:
    gr_storage_web(int flag = 0, const std::string &str = std::string())
        : flag(flag), str(str) {}  


    int getFlag() const {return flag;}
    std::string getString() const {return str;}
private:
    int flag;
    std::string str; 


}; 

#endif
