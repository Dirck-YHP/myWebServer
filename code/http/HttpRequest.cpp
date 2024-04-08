#include "HttpRequest.h"


// 网页名称，和一般的前端跳转不同，这里需要将请求信息放到后端来验证一遍再上传
const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML {
    "/index", "/register", "/login", "/welcome", "/video", "/picture",
};

// 登录/注册——资源及其对应的标签深度
const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG {
    {"/login.html", 1}, {"/register.html", 0}
};

// 初始化，清零
void HttpRequest::Init() {
    state_ = REQUEST_LINE;
    method_ = path_ = version_ = body_ = "";
    header_.clear();
    post_.clear();
}

// 解析处理
bool HttpRequest::Parse(Buffer& buff) {
    const char END[] = "\r\n";

    // 没有可读的字节
    if (buff.ReadableBytes() <= 0) return false;

    while (buff.ReadableBytes() && state_ != FINISH) {
        // 从buff中的读指针开始到读指针结束，这块区域是未读的，找到\r\n，返回其第一次出现的位置，即去掉\r\n的最后一位
        const char* line_end = std::search(buff.Peek(), buff.BeginWriteConst(), END, END + 2);
        std::string line(buff.Peek(), line_end);
        switch (state_) {
            case REQUEST_LINE: {
                if (!ParseRequestLine_(line)) return false; // 解析错误
                ParsePath_();   // 解析路径
                break;
            }                
            case HEADERS: {
                ParseHeader_(line); // 解析头部
                if (buff.ReadableBytes() <= 2) state_ = FINISH;     // 说明是get请求，后面为\r\n，提前结束
                break;
            }            
            case BODY: {
                ParseBody_(line);   // 解析请求体
                break;
            }
            default:
                break;
        }

        // 如果找到了缓冲区的末尾，则清空缓冲区并跳出循环
        if (line_end == buff.BeginWrite()) {
            buff.RetrieveAll();
            break;
        }

        // 将缓冲区中已处理的数据移出，包括\r\n
        buff.RetrieveUntil(line_end + 2);    
    }
    // 记录解析结果
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

// 解析传入的字符串 line，提取出其中的请求方法、路径和 HTTP 版本
bool HttpRequest::ParseRequestLine_(const std::string& line) {
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch Match;      // 用来匹配patten得到结果

    // 在匹配规则中，以括号()的方式来划分组别 一共三个括号 [0]表示整体，[1]表示第一个括号，[2]表示第二个括号，[3]表示第三个括号
    if (std::regex_match(line, Match, patten)) {
        method_ = Match[1];
        path_ = Match[2];
        version_ = Match[3];
        state_ = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

// 解析路径，统一一下path名称，方便后面解析资源
void HttpRequest::ParsePath_() {
    if (path_ == "/") path_ = "/index.html";        // 是否为根路径
    else {      // 是否在 DEFAULT_HTML 集合
        if (DEFAULT_HTML.find(path_) != DEFAULT_HTML.end()) {
            path_ += ".html";
        }
    }
}

void HttpRequest::ParseHeader_(const std::string& line) {
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch Match;
    if (std::regex_match(line, Match, patten)) {
        header_[Match[1]] = Match[2];
    }   // 匹配失败说明首部行匹配完了，状态改变
    else state_ = BODY;
}

void HttpRequest::ParseBody_(const std::string& line) {
    body_ = line;
    ParsePost_();
    state_ = FINISH;
    LOG_DEBUG("Body: %s, len: %d", line.c_str(), line.size());
}

// 16进制转化为10进制
int HttpRequest::ConverHex(char ch) {
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' +10;
    return ch;
}

// 处理post请求
void HttpRequest::ParsePost_() {
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        ParseFromUrlEncoded_();     
        if (DEFAULT_HTML_TAG.count(path_)) { // 如果是登录/注册的path
            int tag = DEFAULT_HTML_TAG.find(path_)->second; 
            LOG_DEBUG("Tag:%d", tag);
            if (tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);  // 为1则是登录
                if (UserVerify(post_["username"], post_["password"], isLogin)) {
                    path_ = "/welcome.html";
                } 
                else {
                    path_ = "/error.html";
                }
            }
        }
    }   
}

// 从url中解析编码
void HttpRequest::ParseFromUrlEncoded_() {
    if(body_.size() == 0) { return; }

    std::string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for(; i < n; i++) {
        char ch = body_[i];
        switch (ch) {
        case '=':       // 当遇到'='时，将其前面的部分作为键
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':       // 将'+'替换为' '
            body_[i] = ' ';
            break;
        case '%':       // 将%后面两个字符转换为对应的ASCII码
            num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':       // 当遇到'&'时，将'='后面到'&'之间的部分作为值，并存储到post_中
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    // 如果请求体中还有剩余的内容且尚未存储到post_中，则将其作为值存储到post_中
    if(post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

// 用户验证
bool HttpRequest::UserVerify(const std::string& name, const std::string& pwd, bool isLogin) {
    // 如果用户名或密码为空，则直接返回false
    if(name == "" || pwd == "") { return false; }

    // 记录验证的用户名和密码
    LOG_INFO("Verify name: %s, password: %s", name.c_str(), pwd.c_str());

    // 获取数据库连接
    MYSQL* sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);

    bool flag = false;
    // unsigned int j = 0;
    char order[256] = {0};
    // MYSQL_FIELD* fields = nullptr;
    MYSQL_RES* res = nullptr;

    // 如果是登录操作，则设置标志位为true
    if (!isLogin) flag = true;
    // 构造查询用户及密码的SQL语句
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    // 执行SQL查询
    if (mysql_query(sql, order)) {
        mysql_free_result(res);
        return false;
    }
    // 获取查询结果
    res = mysql_store_result(sql);   
    // j = mysql_num_fields(res);           // 获取查询结果中的字段（列）数量   
    // fields = mysql_fetch_fields(res);    // 获取 MySQL 查询结果中字段（列）的描述信息

    // 遍历查询结果
    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);
        
        // 如果是登录操作，检查密码是否匹配
        if(isLogin) {
            if(pwd == password) { flag = true; }
            else {
                flag = false;
                LOG_INFO("pwd error!");
            }
        } 
        else {  // 如果是注册操作，检查用户名是否已被使用
            flag = false; 
            LOG_INFO("user used!");
        }
    }
    // 释放查询结果
    mysql_free_result(res);

    // 如果是注册操作且用户名未被使用，则执行插入用户数据的SQL语句
    if (!isLogin && flag == true) {
        LOG_DEBUG("regirster!");
        bzero(order, 256);
        snprintf(order, 256,"INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG( "%s", order);
        if (mysql_query(sql, order)) { 
            LOG_DEBUG( "Insert error!");
            flag = false; 
        }
        flag = true;
    }
    // SqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG( "UserVerify success!!");
    return flag;
}

std::string HttpRequest::Path() const {
    return path_;
}

std::string& HttpRequest::Path() {
    return path_;
}

std::string HttpRequest::Method() const {
    return method_;
}

std::string HttpRequest::Version() const {
    return version_; 
}

std::string HttpRequest::GetPost(const std::string& key) const {
    assert(key != "");
    if (post_.count(key) == 1) return post_.find(key)->second;
    return "";
}

std::string HttpRequest::GetPost(const char* key) const {
    assert(key != nullptr);
    if (post_.count(key) == 1) return post_.find(key)->second;
    return "";
}

bool HttpRequest::IsKeepAlive() const {
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}