#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <errno.h>
#include <mysql/mysql.h>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>

#include "../buffer/Buffer.h"
#include "../log/Log.h"
#include "../pool/SqlConnPool.h"

class HttpRequest {
public:
    enum PARSE_STATE {      // 定义HTTP请求解析状态
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };

    HttpRequest() { Init(); }
    ~HttpRequest() = default;

    void Init();
    bool Parse(Buffer& buff);

    std::string Path() const;
    std::string& Path();
    std::string Method() const;
    std::string Version() const;
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;

    bool IsKeepAlive() const;

private:
    bool ParseRequestLine_(const std::string& line);                   // 解析请求行
    void ParseHeader_(const std::string& line);                        // 解析请求头
    void ParseBody_(const std::string& line);                          // 解析请求体

    void ParsePath_();                                                 // 解析请求路径
    void ParsePost_();                                                 // 解析 POST 请求数据
    void ParseFromUrlEncoded_();                                       // 从url中解析编码

    static bool UserVerify(const std::string& name, const std::string& pwd, bool isLogin); // 用户验证

    PARSE_STATE state_;                                                 // 当前解析状态
    std::string method_, path_, version_, body_;                        // 请求方法、路径、HTTP 版本、消息体
    std::unordered_map<std::string, std::string> header_;               // 请求头键值对
    std::unordered_map<std::string, std::string> post_;                 // POST 参数键值对

    static const std::unordered_set<std::string> DEFAULT_HTML;          // 默认 HTML 内容
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG; // 默认 HTML 标签和对应的标签深度
    static int ConverHex(char ch);      // 16进制转化为10进制
};

#endif // HTTP_REQUEST_H