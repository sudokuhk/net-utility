/*************************************************************************
    > File Name: uimgworker.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Tue Nov 29 14:26:13 CST 2016
 ************************************************************************/

#include "uimgworker.h"
#include <stdio.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <ulog/ulog.h>
#include <3rd/cjson/cJSON.h>


#define SUC_TIPS "Image upload successfully! You can get this image via this address:<br/><br/>\n"

static const char * s_post_errors[] = {
    "Upload Success.",
    "Internal error.",
    "File type not support.",
    "Request method error.",
    "Access error.",
    "Request body parse error.",
    "Content-Length error.",
    "Content-Type error.",
    "File too large.",
    "Request url illegal.",
    "Image not existed.",
};

const uimgworker::handler_type uimgworker::handlers[] = {
    {"/favicon.ico",    &uimgworker::favicon},
    {"/echo",           &uimgworker::echo},
    {"/",               &uimgworker::index},
    {"/index",          &uimgworker::index},
    {"/upload",         &uimgworker::upload},
    {"/download",       &uimgworker::download},
    {NULL, NULL},
};

uimgworker::uimgworker(umonitor & monitor, const uimg_conf & config,
    uschedule & schudule, int fd, const char * ip)
    : monitor_(monitor)
    , config_(config)
    , schudule_(schudule)
    , socket_(4096, fd, &schudule_)
    , http_(socket_, *this, false)
    , md5_()
    , io_(config_.server_datapath)
    , peerip_(ip)
{
    socket_.set_timeout(5000);
}

uimgworker::~uimgworker()
{
    //printf("disconnect fd:%d\n", socket_.socket());
    close(socket_.socket());
}

void uimgworker::run()
{
    http_.run();

    monitor_.onworkerfinish(socket_.socket());
    
    delete this;
}

int uimgworker::onhttp(const uhttprequest & request, 
        uhttpresponse & response, int errcode)
{
    const std::string path = request.uri().path();

    // get real client ip. use to access control.
    const char * xff = request.get_header(uhttpresponse::HEADER_XFF);
    std::string clientip;
    std::string proxyips;
    bool has_proxy = false;
    
    if (xff != NULL) {
        has_proxy = true;
        const char * dot = strchr(xff, ',');
        if (dot == NULL) {
            clientip = xff;
            proxyips = peerip_;
        } else {
            clientip.append(xff, dot - xff);
            proxyips.append(xff + 1).append(",").append(peerip_);
        }
    } else {
        clientip = peerip_;
    }

    if (errcode == uhttp::en_socket_reset) {
        ulog(ulog_debug, "[fd:%d,%s] reset!\n", 
            socket_.socket(), clientip.c_str());
        return -1;
    }

    //if (response.statuscode() != 0) {
    //    return -1;        
    //}

    ulog(ulog_debug, "[fd:%d,%s] [%s %s] proxy[%s]\n", 
        socket_.socket(), clientip.c_str(), path.c_str(), request.methodname(),
        has_proxy ? proxyips.c_str() : "");
    
    const uimgworker::handler_type * h;
    int ret = 0;
    
    for (h = handlers; h->cmd != NULL; h ++) {
        if (strcmp(h->cmd, path.c_str()) == 0) {
            ulog(ulog_debug, "find handler for path:%s!\n", h->cmd);
            break;
        }
    }
    
    if (h->cmd == NULL) {
        ulog(ulog_error, "not find handler for path:%s!\n", h->cmd);
        ret = notfind(request, response, errcode);
    } else {
        handler hf = h->func;
        ret = (this->*hf)(request, response, errcode);
    }

    ulog(ulog_info, "[%s] [%s %s] --> [%d %s] proxy[%s]\n", 
        clientip.c_str(), path.c_str(), request.methodname(),
        response.statuscode(), response.reasonphrase(), 
        has_proxy ? proxyips.c_str() : "");
    
    monitor_.onworkerevent(request.method(), path.c_str(), 
        request.content().size(), 
        response.statuscode(), response.content().size());
    
    return ret;
}

int uimgworker::notfind(const uhttprequest & request, 
    uhttpresponse & response, int errcode)
{
    ulog(ulog_debug, "notfind!\n");
    response.set_statuscode(uhttp_status_not_found);

    response.set_header(uhttpresponse::HEADER_CONTENT_TYPE, "text/html");
    response.set_content("<html><body><h1>404 Not Found!</h1></body></html>");

    return -1;
}

int uimgworker::favicon(const uhttprequest & request, 
    uhttpresponse & response, int errcode)
{
    ulog(ulog_debug, "favicon request!\n");
    response.set_statuscode(uhttp_status_ok);
    response.set_header(uhttpresponse::HEADER_CONTENT_TYPE, "text/html");
    add_extheaders(response);
    
    return 0;
}

int uimgworker::echo(const uhttprequest & request, 
    uhttpresponse & response, int errcode)
{   
    ulog(ulog_debug, "echo request!\n");

    response.set_statuscode(uhttp_status_not_found);
    response.set_header(uhttpresponse::HEADER_CONTENT_TYPE, "text/html");
    response.set_content("<html><body><h1>uimg works!</h1></body></html>");
    
    return 0;
}

int uimgworker::index(const uhttprequest & request, 
    uhttpresponse & response, int errcode)
{
    int ret = 0;
    std::string filename = config_.root_path + "index.html";
    ulog(ulog_debug, "access index file(%s)!\n", filename.c_str());
    
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        ulog(ulog_error, "open index file(%s) failed!\n", filename.c_str());
        ret = notfind(request, response, errcode);
    } else {
        struct stat sb;
        if (stat(filename.c_str(), &sb) != 0) {
            sb.st_size = 0;
            ulog(ulog_error, "stat file(%s) failed!\n", filename.c_str());
        }
        ulog(ulog_debug, "index file(%s) size:%ld!\n", filename.c_str(), sb.st_size);
        
        std::string content;
        content.resize(sb.st_size);

        ssize_t rdn = 0;
        if (sb.st_size == 0 || 
            (rdn = read(fd, (void *)&content[0], sb.st_size)) != sb.st_size) {
            ulog(ulog_error, "not enough, need:%ld, read:%ld!\n", sb.st_size, rdn);
            ret = notfind(request, response, errcode);
        } else {
            response.set_statuscode(uhttp_status_ok);
            response.content().swap(content);
            response.set_header(uhttpresponse::HEADER_CONTENT_TYPE, "text/html");
        }
        close(fd);
    }
    
    return ret;
}

int uimgworker::upload(const uhttprequest & request, 
    uhttpresponse & response, int errcode)
{
    typedef const char * const_pchar;
    int result  = en_post_succeed;
    const_pchar ptype, pboundary, pintype, pequal, plquote, prquote, ptypeend;
    std::string md5;
    int fsize   = 0;
    
    if (request.method() == uhttp_method_post) {
        do {
            if (config_.limitsize > 0 && 
                (int)request.content().size() > config_.limitsize) {
                fsize   = (int)request.content().size();
                result  = en_post_file_too_big;
                break;
            } else if (errcode != uhttp::en_succeed) {
                result = en_post_internal_error;
                break;
            }
            
            result = en_post_content_type_error;
            ptype = request.get_header(uhttpmessage::HEADER_CONTENT_TYPE);
            if (ptype == NULL) {
                break;
            }
            ptypeend = ptype + strlen(ptype);
            
            pintype = strstr(ptype, "multipart/form-data");
            if (pintype != NULL) {
                if ((pboundary = strstr(pintype, "boundary")) == NULL) {
                    break;
                }

                if ((pequal = strchr(pboundary, '=')) == NULL) {
                    break;
                }

                pequal ++;
                if (pequal > ptypeend || (plquote = strchr(pequal, '"')) == NULL) {
                    plquote = pequal;
                    prquote = strpbrk(pequal, ",;");
                    if (prquote == NULL) {
                        prquote = ptypeend;
                    }
                } else {
                    pequal = plquote ++;
                    if (pequal > ptypeend || (prquote = strchr(pequal, '"')) == NULL) {
                        break;
                    }
                }
                std::string boundary(plquote, prquote - plquote);

                result = en_post_body_error;
                uboundaryparser parser;
                std::vector<uboundaryparser::uboundaryinfo> info;
                if (parser.parse(request.content(), boundary.c_str(), info)) {
                    ulog(ulog_info, "uboundaryparser ok!, output:%ld\n", info.size());
                    
                    for (size_t i = 0; i < info.size(); i++) {
                        
                        md5_.reset();
                        md5_.update(info[i].data, info[i].len);
                        
                        md5     = md5_.toString();
                        fsize   = info[i].len;

                        if (io_.save(md5, info[i].data, info[i].len) == 0) {
                            result = en_post_succeed;
                        } else {
                            result  = en_post_internal_error;
                        }
                        /*if (io_.save(md5, info[i].data, info[i].len) == 0) {
                            response.append_content("<h1>MD5: ");
                            response.append_content(md5);
                            response.append_content("</h1>\n");
                            response.append_content(SUC_TIPS);
                        } else {
                            response.append_content("<h1>File:");
                            response.append_content(info[i].filename);
                            response.append_content(" Upload Failed!</h1>\n");
                        }*/

                        break;
                    }
                } else {
                    ulog(ulog_error, "uboundaryparser parse failed!\n");
                    break;
                }
            } else { //binary.
                const std::string & data = request.content();

                ulog(ulog_debug, "upload, binary. size:%ld!\n", data.size());

                md5_.reset();
                md5_.update(data.data(), data.size());
                
                md5     = md5_.toString();
                fsize   = data.size();
                
                ulog(ulog_debug, "calc md5:%s\n", md5.c_str());

                int saveret = io_.save(md5, data.data(), data.size());
                if (saveret == 0) {
                    result  = en_post_succeed;
                } else {
                    result  = en_post_internal_error;
                }
                
                ulog(ulog_debug, "save result:%d\n", saveret);
            }
        } while (0);
    } else {
        result = en_post_req_method_error;
    }

    response.set_statuscode(uhttp_status_ok);
    response.set_header(uhttpresponse::HEADER_CONTENT_TYPE, "application/json");
    response.set_content(generate_json(result, md5, fsize));
    
    return result;
}

int uimgworker::download(const uhttprequest & request, 
    uhttpresponse & response, int errcode)
{
    ulog(ulog_debug, "download request\n");

    const std::string key("md5");
    const uuri::query_map & querys = request.uri().query();
    const uuri::query_map::const_iterator it = querys.find(key);
    std::string md5;
    int ret = 0;

    if (it != querys.end()) {
        md5 = it->second;
    }

    if (umd5sum::ismd5(md5)) {
        ret = io_.get(md5, querys, response.content());
        response.set_header(uhttpresponse::HEADER_CONTENT_TYPE, "image/jpeg");
    } else {
        ulog(ulog_error, "invalid md5:%s!", md5.c_str());
        response.set_header(uhttpresponse::HEADER_CONTENT_TYPE, "text/html");
        response.content().append("<h1>Invalid MD5:")
            .append(md5).append("!</h1></body></html>");
        ret = -1;
    }

    if (ret == 0) {
        add_extheaders(response);
        
        if (check_etag(request, response, md5)) {
            response.set_statuscode(uhttp_status_not_modified);
            response.content().clear();
        } else {
            response.set_statuscode(uhttp_status_ok);
        }
    } else {
        ret = notfind(request, response, errcode);
    }
    
    return ret;
}

void uimgworker::add_extheaders(uhttpresponse & response)
{
    typedef std::map<std::string, std::string> header_map;
    header_map::const_iterator it;
    
    for (it = config_.add_header.begin(); it != config_.add_header.end(); ++ it) {
        response.set_header(it->first.c_str(), it->second.c_str());
    }
}

bool uimgworker::check_etag(const uhttprequest & request, 
    uhttpresponse & response, const std::string & md5)
{
    bool ret = false;
    
    if (config_.etag) {
        const char * tag = request.get_header("If-None-Match");
        if (tag == NULL) {
            ulog(ulog_debug, "No If-None-Match! MD5:%s\n", md5.c_str());
            response.set_header("Etag", md5.c_str());
        } else {
            ulog(ulog_debug, "If-None-Match:%s, MD5:%s\n", tag, md5.c_str());
            
            if (strlen(tag) != MD5_LEN || strncmp(md5.c_str(), tag, MD5_LEN)) {
                response.set_header("Etag", md5.c_str());
            } else {
                ret = true;
            }
        }
    }

    return ret;
}

std::string uimgworker::generate_json(int ret, 
    const std::string & md5, int upsize)
{
    std::string jsonstr("{ret:-1, code:\"unkown error\"}");

    if (ret < 0 || ret > en_post_max) {
        return jsonstr;
    }

    cJSON *j_ret        = cJSON_CreateObject();

    cJSON_AddNumberToObject(j_ret, "ret", ret);
    cJSON_AddStringToObject(j_ret, "md5", 
        ret == en_post_succeed ? md5.c_str() : "");
    cJSON_AddNumberToObject(j_ret, "size", upsize);
    cJSON_AddStringToObject(j_ret, "message", s_post_errors[ret]);
        
    char * pjson = cJSON_PrintUnformatted(j_ret);
    ulog(ulog_debug, "ret json: %s\n", pjson);

    std::string(pjson).swap(jsonstr);
    
    cJSON_Delete(j_ret);
    free(pjson);
    
    return jsonstr;
}

