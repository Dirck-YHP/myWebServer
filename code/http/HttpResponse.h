#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <fcntl.h>          
#include <unistd.h>       
#include <sys/stat.h>
#include <sys/mman.h>  
#include <unordered_map>

#include "../buffer/Buffer.h"
#include "../log/Log.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);
    void MakeResponse(Buffer& buff);
    void UnmapFile();
    char* File();
    size_t FileLen() const;
    void ErrorContent(Buffer& buff, std::string message);
    int Code() const { return code_;}

private:
    void AddStateLine_(Buffer& buff);
    void AddHeader_(Buffer& buff);
    void AddContent_(Buffer& buff);

    void ErrorHtml_();
    std::string GetFileType_();

private:
    int code_;                  // 响应状态码
    bool isKeepAlive_;          // 是否保持连接

    std::string path_;          // 请求路径
    std::string srcDir_;

    char* mmFile_;              // 文件映射指针
    struct stat mmFileStat_;    // 文件状态

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE; // 文件后缀类型集
    static const std::unordered_map<int, std::string> CODE_STATUS;         // 状态码集
    static const std::unordered_map<int, std::string> CODE_PATH;           // 编码路径集    
};

#endif // HTTP_RESPONSE_H