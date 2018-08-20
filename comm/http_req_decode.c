
#include <stdio.h>
#include <string.h>
#include "http_comm.h"

extern int get_next_line(char *p_buf, char *p_line, int line_buf_len, int *p_line_len);
extern int http_general_field_decode(char *p_buf, int len, HTTP_GENERAL_FIELD *p_general_field);
extern int http_entity_field_decode(char *p_buf, int len, HTTP_ENTITY_FIELD *p_general_field);
extern int http_extra_field_decode(char *p_buf, int len, HTTP_EXTRA_FIELD *p_extra_field);

static int http_req_field_decode(char *p_buf, int len, HTTP_REQ_FIELD *p_req_field);

int http_req_header_decode(char *p_buf, int len, HTTP_REQ_HEADER *p_req_header)
{
	int rlt;
	char *p_pos;
	char line[2048];
	int line_len;

	char method_name[64];
	
	p_pos = p_buf;

	memset(p_req_header, 0, sizeof(HTTP_REQ_HEADER));
	memset(line, 0, sizeof(line));

	rlt = get_next_line(p_pos, line, sizeof(line), &line_len);
	if (rlt < 0)
	{
		return -1;
	}
	p_pos += rlt;

	memset(method_name, 0, sizeof(method_name));
	
	rlt = sscanf(line, "%[^: \t]%*[: \t]%[^ \t]%*[: \t]%[^\r]",
			method_name,
			p_req_header->request_uri,
			p_req_header->http_version);

	if (rlt != 3)
	{
		return -1;
	}

	if (strcasecmp(method_name, "Get") == 0)
	{
		p_req_header->method = HTTP_METHOD_GET;
	}
	else if (strcasecmp(method_name, "Post") == 0)
	{
		p_req_header->method = HTTP_METHOD_POST;
	}
	else if (strcasecmp(method_name, "Put") == 0)
	{
		p_req_header->method = HTTP_METHOD_PUT;
	}
	else
	{
	}
	
	http_general_field_decode(line, line_len, &p_req_header->general_field);

	http_req_field_decode(p_pos, len - (p_pos - p_buf), &p_req_header->req_field);

	http_entity_field_decode(p_pos, len - (p_pos - p_buf), &p_req_header->entity_field);

	http_extra_field_decode(p_pos, len - (p_pos - p_buf), &p_req_header->extra_field);

	
	return p_pos - p_buf;
}

int http_req_field_decode(char *p_buf, int len, HTTP_REQ_FIELD *p_req_field)
{
	int rlt;
	char *p_pos;
	char line[2048];
	int line_len;

	char field_name[HTTP_FILED_LEN];
		
	p_pos = p_buf;

	while (p_pos - p_buf < len)
	{
		memset(line, 0, sizeof(line));

		rlt = get_next_line(p_pos, line, sizeof(line), &line_len);
		if (rlt < 0)
		{
			break;
		}
		p_pos += rlt;

		memset(field_name, 0, sizeof(field_name));

		rlt = sscanf(line, "%[^:]", field_name);

		if (strcasecmp(field_name, "Accept") == 0)
		{
			rlt = sscanf(line, "%*[^:]%*[: \t]%[^\r]", p_req_field->accept);
			p_req_field->accept_flag = 1;

		}
		else if (strcasecmp(field_name, "Accept-Charset") == 0)
		{
			rlt = sscanf(line, "%*[^:]%*[: \t]%[^\r]", p_req_field->accept_charset);
			p_req_field->accept_charset_flag= 1;
		}
		else if (strcasecmp(field_name, "Accept-Encoding") == 0)
		{
			rlt = sscanf(line, "%*[^:]%*[: \t]%[^\r]", p_req_field->accept_encoding);
			p_req_field->accept_encoding_flag = 1;
		}
		else if (strcasecmp(field_name, "Accept-Language") == 0)
		{
			rlt = sscanf(line, "%*[^:]%*[: \t]%[^\r]", p_req_field->accept_language);
			p_req_field->accept_language_flag = 1;
		}
		else if (strcasecmp(field_name, "Host") == 0)
		{
			rlt = sscanf(line, "%*[^:]%*[: \t]%[^\r]", p_req_field->host);
			p_req_field->host_flag = 1;
		}
		else if (strcasecmp(field_name, "User-Agent") == 0)
		{
			rlt = sscanf(line, "%*[^:]%*[: \t]%[^\r]", p_req_field->user_agent);
			p_req_field->user_agent_flag = 1;
		}
		else
		{
		}
		
	} 

	return p_pos - p_buf;
}


