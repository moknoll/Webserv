/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmagomad <nmagomad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/01 13:20:33 by nmagomad          #+#    #+#             */
/*   Updated: 2026/05/01 13:20:35 by nmagomad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef _HTTP_CONSTANTS_
#define _HTTP_CONSTANTS_

#define SERVER_NAME_STR                      "Webserv"
#define HTTP_VERSION                         "HTTP/1.1"

#define OK                                   0
#define ERROR                                -1
#define AGAIN                                -2
#define BUSY                                 -3
#define DONE                                 -4
#define DECLINED                             -5
#define ABORT                                -6

#define LF                                   '\n'   // Line Feed  (New line)
#define CR                                   '\r'   // Carriage Return
#define CRLF                                 "\x0d\x0a"

#define HTTP_UNKNOWN                         0x0001 // (1 << 0) 1
#define HTTP_GET                             0x0002 // (1 << 1) 2
#define HTTP_POST                            0x0004 // (1 << 2) 4
#define HTTP_DELETE                          0x0008 // (1 << 3) 8
#define HTTP_HEAD                            0x0010 // (1 << 4) 16
#define HTTP_PUT                             0x0020 // (1 << 5) 32
#define HTTP_COPY                            0x0040 // (1 << 6) 64
#define HTTP_MOVE                            0x0080 // (1 << 7) 128
#define HTTP_CONNECT                         0x0100 // (1 << 8) 257

#define HTTP_CONNECTION_CLOSE                1
#define HTTP_CONNECTION_KEEP_ALIVE           2

#define HTTP_OK                              200
#define HTTP_CREATED                         201
#define HTTP_NO_CONTENT                      204
#define HTTP_PARTIAL_CONTENT                 206

#define HTTP_SPECIAL_RESPONSE                300
#define HTTP_MOVED_PERMANENTLY               301
#define HTTP_MOVED_TEMPORARILY               302
#define HTTP_NOT_MODIFIED                    304

#define HTTP_BAD_REQUEST                     400
#define HTTP_UNAUTHORIZED                    401
#define HTTP_FORBIDDEN                       403
#define HTTP_NOT_FOUND                       404
#define HTTP_NOT_ALLOWED                     405
#define HTTP_REQUEST_TIME_OUT                408
#define HTTP_CONFLICT                        409
#define HTTP_LENGTH_REQUIRED                 411
#define HTTP_PRECONDITION_FAILED             412
#define HTTP_CONTENT_TOO_LARGE               413
#define HTTP_URI_TOO_LONG                    414
#define HTTP_UNSUPPORTED_MEDIA_TYPE          415
#define HTTP_RANGE_NOT_SATISFIABLE           416
#define HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE 431

#define HTTP_INTERNAL_SERVER_ERROR           500
#define HTTP_NOT_IMPLEMENTED                 501
#define HTTP_BAD_GATEWAY                     502
#define HTTP_SERVICE_UNAVAILABLE             503
#define HTTP_GATEWAY_TIME_OUT                504
#define HTTP_INSUFFICIENT_STORAGE            507

typedef enum http_state_e
{
	HTTP_INIT_REQUEST_STATE = 0,
	HTTP_READING_STATE,
	HTTP_WRITING_STATE,
	HTTP_KEEPALIVE_STATE,
	HTTP_CLOSE
} http_state_t;

enum FileError_e
{
	FILE_OK = 0,
	ERR_NOT_FOUND,
	ERR_PERMISSION,
	ERR_IS_DIR,
	ERR_UNKNOWN
};

#endif /*  _HTTP_CONSTANTS_ */
