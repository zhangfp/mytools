#include <stdio.h>
#include <string.h>
#include "http_comm.h"

extern int http_general_field_encode(HTTP_GENERAL_FIELD *p_general_field, char *p_buf, int len);
//extern int http_entity_field_encode(HTTP_ENTITY_FIELD *p_entity_field, char *p_buf, int len);
extern int http_extra_field_encode(HTTP_EXTRA_FIELD *p_extra_field, char *p_buf, int len);

static int http_req_field_encode(HTTP_REQ_FIELD *p_req_field, char *p_buf, int len);

int http_req_header_encode(HTTP_REQ_HEADER *p_req_header, char *p_buf, int len)
{
	char *p_pos;
	int rlt;
	char method_buf[HTTP_FILED_LEN];
	
	p_pos = p_buf;

	if (!p_req_header->http_version_flag)
	{
		strcpy(p_req_header->http_version, HTTP_VERSION);
	}

	memset(&method_buf, 0, sizeof(method_buf));
	
	switch (p_req_header->method)
	{
	case HTTP_METHOD_OPTIONS:
		strcpy(method_buf, "OPTIONS");
		break;
		
	case HTTP_METHOD_GET:
		strcpy(method_buf, "GET");
		break;
		
	case HTTP_METHOD_HEAD:
		strcpy(method_buf, "HEAD");
		break;
		
	case HTTP_METHOD_POST:
		strcpy(method_buf, "POST");
		break;
		
	case HTTP_METHOD_PUT:
		strcpy(method_buf, "PUT");
		break;
		
	case HTTP_METHOD_DELETE:
		strcpy(method_buf, "DELETE");
		break;
		
	case HTTP_METHOD_TRACE:
		strcpy(method_buf, "TRACE");
		break;
		
	case HTTP_METHOD_CONNECT:
		strcpy(method_buf, "CONNECT");
		break;

	default:
		goto FAILED;
		break;
	}
	
	rlt = sprintf(p_pos, "%s %s %s"CRLF, method_buf, p_req_header->request_uri, p_req_header->http_version);
	if (rlt < 0)
	{
		goto FAILED;
	}
	
	p_pos += rlt;
	
	rlt = http_general_field_encode(&p_req_header->general_field, p_pos, len - (p_pos - p_buf));
	if (rlt < 0)
	{
		goto FAILED;
	}
	
	p_pos += rlt;

	rlt = http_extra_field_encode(&p_req_header->extra_field, p_pos, len - (p_pos - p_buf));
	if (rlt < 0)
	{
		goto FAILED;
	}

	p_pos += rlt;

	rlt = http_req_field_encode(&p_req_header->req_field, p_pos, len - (p_pos - p_buf));
	if (rlt < 0)
	{
		goto FAILED;
	}

	p_pos += rlt;

	rlt = http_req_entity_field_encode(&p_req_header->entity_field, p_pos, len - (p_pos - p_buf));
	if (rlt < 0)
	{
		goto FAILED;
	}
	
	p_pos += rlt;

	rlt = sprintf(p_pos, CRLF);
	p_pos += rlt;
	
	return p_pos - p_buf;

FAILED:

	return -1;
}

/*
int isld_http_entity_field_encode(ISLD_HTTP_ENTITY_FIELD *p_entity_field, char *p_buf, int len)
{
	return 0;
}
*/

int http_req_field_encode(HTTP_REQ_FIELD *p_req_field, char *p_buf, int len)
{
	char *p_pos;
	int rlt;
	
	p_pos = p_buf;

	if (p_req_field->accept_flag)
	{
		rlt = sprintf(p_pos, "Accept: %s"CRLF, p_req_field->accept);

		p_pos += rlt;
	}

	if (p_req_field->accept_charset_flag)
	{
		rlt = sprintf(p_pos, "Accept-Charset: %s"CRLF, p_req_field->accept_charset);

		p_pos += rlt;
	}

	if (p_req_field->accept_encoding_flag)
	{
		rlt = sprintf(p_pos, "Accept-Encoding: %s"CRLF, p_req_field->accept_encoding);

		p_pos += rlt;
	}

	if (p_req_field->accept_language_flag)
	{
		rlt = sprintf(p_pos, "Accept-Language: %s"CRLF, p_req_field->accept_language);

		p_pos += rlt;
	}

	if (p_req_field->user_agent_flag)
	{
		rlt = sprintf(p_pos, "User-Agent: %s"CRLF, p_req_field->user_agent);

		p_pos += rlt;
	}

	if (p_req_field->host_flag)
	{
		rlt = sprintf(p_pos, "Host: %s"CRLF, p_req_field->host);

		p_pos += rlt;
	}

	return p_pos - p_buf;
}

int http_req_entity_field_encode(HTTP_ENTITY_FIELD *p_entity_field, char *p_buf, int len)
{
	int rlt;
	char *p_pos;
	
	p_pos = p_buf;

	if (p_entity_field->allow_flag)
	{
		rlt = sprintf(p_pos, "Allow: %s"CRLF, p_entity_field->allow);

		p_pos += rlt;
	}

	if (p_entity_field->content_encoding_flag)
	{
		rlt = sprintf(p_pos, "Content-Encoding: %s"CRLF, p_entity_field->content_encoding);

		p_pos += rlt;
	}

	if (p_entity_field->content_language_flag)
	{
		rlt = sprintf(p_pos, "Content-Language : %s"CRLF, p_entity_field->content_language);

		p_pos += rlt;
	}

	if (p_entity_field->content_length_flag)
	{
		rlt = sprintf(p_pos, "Content-Length: %s"CRLF, p_entity_field->content_length);

		p_pos += rlt;
	}

	if (p_entity_field->content_location_flag)
	{
		rlt = sprintf(p_pos, "Content-Location: %s"CRLF, p_entity_field->content_location);

		p_pos += rlt;
	}
	
	if (p_entity_field->content_md5_flag)
	{
		rlt = sprintf(p_pos, "Content-MD5: %s"CRLF, p_entity_field->content_md5);

		p_pos += rlt;
	}
	
	if (p_entity_field->content_range_flag)
	{
		rlt = sprintf(p_pos, "Content-Range: %s"CRLF, p_entity_field->content_range);

		p_pos += rlt;
	}
	
	if (p_entity_field->content_type_flag)
	{
		rlt = sprintf(p_pos, "Content-Type: %s"CRLF, p_entity_field->content_type);

		p_pos += rlt;
	}

	if (p_entity_field->expires_flag)
	{
		rlt = sprintf(p_pos, "Expires : %s"CRLF, p_entity_field->expires);

		p_pos += rlt;
	}
	
	if (p_entity_field->last_modified_flag)
	{
		rlt = sprintf(p_pos, "Last-Modified: %s"CRLF, p_entity_field->last_modified);

		p_pos += rlt;
	}

	return p_pos - p_buf;
}

