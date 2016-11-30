#ifndef _UHTTPDEFS_H__
#define _UHTTPDEFS_H__

#define UHTTP_LINE_END   "\r\n"

enum uhttp_version
{
    uhttp_version_1_0 = 0,
    uhttp_version_1_1,
    uhttp_version_unknown,
};

enum uhttp_scheme {
    uhttp_scheme_none = 0,
    uhttp_scheme_ftp,
    uhttp_scheme_http,
    uhttp_scheme_https,
    uhttp_scheme_nfs,
    uhttp_scheme_unknown
};

enum uhttp_method
{
    uhttp_method_get = 0,
    uhttp_method_head,
    uhttp_method_post,
    uhttp_method_put,
    uhttp_method_delete,
    uhttp_method_mkcol,
    uhttp_method_copy,
    uhttp_method_move,
    uhttp_method_options,
    uhttp_method_propfind,
    uhttp_method_proppatch,
    uhttp_method_lock,
    uhttp_method_unlock,
    uhttp_method_trace,
    uhttp_method_connect, /* rfc 2616 */
    uhttp_method_patch,   /* rfc 5789 */
    uhttp_method_unknown,
};

enum uhttp_parse_error {
    uhttp_parse_error_none = 0,
    uhttp_parse_error_too_big,
    uhttp_parse_error_inval_method,
    uhttp_parse_error_inval_reqline,
    uhttp_parse_error_inval_schema,
    uhttp_parse_error_inval_proto,
    uhttp_parse_error_inval_ver,
    uhttp_parse_error_inval_hdr,
    uhttp_parse_error_inval_chunk_sz,
    uhttp_parse_error_inval_chunk,
    uhttp_parse_error_inval_state,
    uhttp_parse_error_user,
    uhttp_parse_error_status,
    uhttp_parse_error_generic
};

enum uhttp_status_code
{
    //Informational 1xx
    uhttp_status_continue           = 100,
    uhttp_status_switch_protocol    = 101,
    uhttp_status_processing         = 102,  //(WebDAV; RFC 2518)

    //Successful 2xx
    uhttp_status_ok                 = 200,
    uhttp_status_created            = 201,
    uhttp_status_accepted           = 202,
    uhttp_status_no_author_info     = 203,
    uhttp_status_no_content         = 204,
    uhttp_status_reset_content      = 205,
    uhttp_status_partial_content    = 206,  //(RFC 7233)
    uhttp_status_multi_status       = 207,  //(WebDAV; RFC 4918)
    uhttp_status_already_reported   = 208,  //(WebDAV; RFC 5842)
    uhttp_status_im_used            = 226,  //(RFC 3229)

    //Redirection 3xx  
    uhttp_status_multiple_choices   = 300,
    uhttp_status_moved_permanently  = 301,
    uhttp_status_found              = 302,
    uhttp_status_see_other          = 303,
    uhttp_status_not_modified       = 304,  //(RFC 7232)
    uhttp_status_use_proxy          = 305,
    //306 unused.
    uhttp_status_temporary_redirect = 307,
    uhttp_status_permanent_redirect = 308,
    
    //Client Error 4xx
    uhttp_status_bad_request        = 400,
    uhttp_status_unauthorized       = 401,
    uhttp_status_payment_required   = 402,
    uhttp_status_forbidden          = 403,
    uhttp_status_not_found          = 404,
    uhttp_status_method_not_allowed = 405,
    uhttp_status_not_acceptable     = 406,
    uhttp_status_proxy_auth_required= 407,
    uhttp_status_request_timeout    = 408,
    uhttp_status_conflict           = 409,
    uhttp_status_gone               = 410,
    uhttp_status_length_required    = 411,
    uhttp_status_precondition_failed= 412,
    uhttp_status_request_entity_too_large = 413,
    uhttp_status_request_uri_too_long = 414,
    uhttp_status_unspport_media_type = 415,
    uhttp_status_requested_range_not_statisfiable = 416,
    uhttp_status_expectation_failed = 417,

    //Server Error 5xx
    uhttp_status_internal_server_error = 500,
    uhttp_status_not_implemented = 501,
    uhttp_status_bad_gateway = 502,
    uhttp_status_service_unavailable = 503,
    uhttp_status_gateway_timeout = 504,
    uhttp_status_http_version_not_supported = 505,
};

#endif
