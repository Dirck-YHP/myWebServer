#include "HttpResponse.h"

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const std::unordered_map<int, std::string> HttpResponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

HttpResponse::HttpResponse() {
    code_ = -1;
    path_ = srcDir_ = "";
    mmFile_ = nullptr;
    mmFileStat_ = { 0 };
}

HttpResponse::~HttpResponse() {
    UnmapFile();
}

void HttpResponse::Init(const std::string& srcDir, std::string& path, bool isKeepAlive, int code) {
    assert(srcDir != "");
    if (mmFile_) UnmapFile();
    code_ = code;
    isKeepAlive_ = isKeepAlive;
    path_ = path;
    srcDir_ = srcDir;
    mmFile_ = nullptr;
    mmFileStat_ = { 0 };
}

void HttpResponse::MakeResponse(Buffer& buff) {
    // 判断请求的资源文件是否存在或者是否为目录
    if (stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
        code_ = 404;        // 如果资源文件不存在或者为目录，则设置响应状态码为404
    }
    else if (!(mmFileStat_.st_mode & S_IROTH)) {
        code_ = 403;        // 如果资源文件不可读，则设置响应状态码为403
    }
    else if (code_ == -1) {
        code_ = 200;        // 如果状态码未设置，则默认为200，表示请求成功  
    }
    ErrorHtml_();           // 根据错误状态码生成对应的 HTML 内容
    AddStateLine_(buff);    // 添加响应状态行到缓冲区
    AddHeader_(buff);       // 添加响应头到缓冲区
    AddContent_(buff);      // 添加响应内容到缓冲区
}

char* HttpResponse::File() {
    return mmFile_;
}

size_t HttpResponse::FileLen() const {
    return mmFileStat_.st_size;
}

// 释放资源
void HttpResponse::UnmapFile() {
    if (mmFile_) {
        munmap(mmFile_, mmFileStat_.st_size);   // 调用 munmap 函数取消映射 mmFile_ 指向的内存区域
        mmFile_ = nullptr;
    }
}

// 构建错误页面内容
void HttpResponse::ErrorContent(Buffer& buff, std::string message) {
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";

    // 获取状态码对应的状态信息
    if (CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    }
    else {
        status = "Bad Request";
    }

    // 构建错误页面的状态行
    body += std::to_string(code_) + " : " + status  + "\n";
    // 添加错误消息
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    // 添加 Content-length 头部信息
    buff.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    // 将错误页面内容添加到缓冲区
    buff.Append(body);

}

void HttpResponse::ErrorHtml_() {
    // 检查是否有与状态码对应的错误页面路径
    if (CODE_PATH.count(code_) == 1) {
        path_ = CODE_PATH.find(code_)->second;
        // 获取错误页面的文件状态，存到 mmFileStat_ 中
        stat((srcDir_ + path_).data(), &mmFileStat_);
    }
}

// 添加 HTTP 响应的状态行到指定的缓冲区
void HttpResponse::AddStateLine_(Buffer& buff) {
    std::string status;
    // 检查状态码是否在已知的状态码集合中
    if (CODE_STATUS.count(code_) == 1) {
        // 如果是，获取对应的状态信息
        status = CODE_STATUS.find(code_)->second;
    }
    else {
        // 如果状态码未知，默认为400 Bad Request
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    // 将状态行添加到缓冲区中
    buff.Append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

// 添加 HTTP 响应的头部信息到指定的缓冲区
void HttpResponse::AddHeader_(Buffer& buff) {
    // 添加 Connection 头部
    buff.Append("Connection: ");
    if (isKeepAlive_) {
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6, timeout=120\r\n");
    }
    else {
        buff.Append("close\r\n");
    }
    // 添加 Content-type 头部
    buff.Append("Content-type: " + GetFileType_() + "\r\n");
}

// 向缓冲区中添加响应内容
void HttpResponse::AddContent_(Buffer& buff) {
    int srcFd = open((srcDir_ + path_).data(), O_RDONLY);
    if (srcFd < 0) {
        // 如果文件打开失败，添加文件未找到的错误内容并返回
        ErrorContent(buff, "File NotFound!");
        return;
    }

    LOG_DEBUG("file path %s", (srcDir_ + path_).data());
    // 将文件映射到内存提高文件的访问速度  MAP_PRIVATE 建立一个写入时拷贝的私有映射
    // 私有映射意味着对映射内存的修改不会影响到原文件
    int* mmRet = (int*)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if (*mmRet == -1) {
        ErrorContent(buff, "File NotFound!");
        return;
    }
    // 将映射的内存地址保存到 mmFile_ 中
    mmFile_ = (char*)mmRet;
    close(srcFd);
    // 添加 Content-length 头部
    buff.Append("Content-length: " + std::to_string(mmFileStat_.st_size) + "\r\n\r\n");
}

/*
MIME（Multipurpose Internet Mail Extensions）类型是一种在互联网上用于标识文件类型的标准。
它是在网络传输过程中对数据进行标识和分类的一种方法。
MIME 类型由两部分组成：主类型和子类型，中间用斜杠分隔。
例如，text/plain 中的 text 是主类型，表示文本文件，
而 plain 是子类型，表示纯文本文件。常见的 MIME 类型包括：

text/plain：纯文本文件
text/html：HTML 文档
image/jpeg：JPEG 图像文件
image/png：PNG 图像文件
application/json：JSON 数据
application/pdf：PDF 文档
audio/mpeg：MPEG 音频文件
video/mp4：MP4 视频文件

在 HTTP 协议中，MIME 类型常用于指定要传输的数据的类型，
以便接收方能够正确解析数据。
通常在 HTTP 响应头中的 Content-Type 字段中指定 MIME 类型。
*/

// 根据文件路径的后缀来确定文件的 MIME 类型
std::string HttpResponse::GetFileType_() {
    // 查找路径中最后一个点的位置
    std::string::size_type idx = path_.find_last_of('.');
    // 如果找不到点，则默认返回文本类型
    if (idx == std::string::npos) {     // 最大值 find函数在找不到指定值得情况下会返回string::npos
        return "text/plain";    
    }

    // 提取后缀
    std::string suffix = path_.substr(idx);
    // 检查后缀是否在已知的后缀类型映射中
    if (SUFFIX_TYPE.count(suffix) == 1) {
        // 如果是，则返回对应的 MIME 类型
        return SUFFIX_TYPE.find(suffix)->second;
    }
    // 如果后缀未知，默认返回文本类型
    return "text/plain";
}



