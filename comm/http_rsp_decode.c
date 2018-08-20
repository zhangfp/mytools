#include <stdio.h>
#include <string.h>
#include "http_comm.h"


extern int get_next_line(char *p_buf, char *p_line, int line_buf_len, int *p_line_len);
extern int http_entity_field_decode(char *p_buf, int len, HTTP_ENTITY_FIELD *p_general_field);
extern int http_extra_field_decode(char *p_buf, int len, HTTP_EXTRA_FIELD *p_extra_field);

static int rsp_status_line_decode(char *p_line, int line_len, HTTP_RSP_HEADER *p_rsp_header);

static int http_rsp_field_decode(char *p_buf, int len, HTTP_RSP_FIELD*p_rsp_field);

int http_rsp_header_decode(char *p_buf, int len, HTTP_RSP_HEADER *p_rsp_header)
{
	int rlt;
	char *p_pos;
	char line[2048];
	int line_len;

	p_pos = p_buf;

	memset(line, 0, sizeof(line));

	rlt = get_next_line(p_pos, line, sizeof(line), &line_len);
	if (rlt < 0)
	{
		return -1;
	}
	p_pos += rlt;

	rsp_status_line_decode(line, line_len, p_rsp_header);

	http_entity_field_decode(p_pos, len - (p_pos - p_buf), &p_rsp_header->entity_field);

	http_extra_field_decode(p_pos, len - (p_pos - p_buf), &p_rsp_header->extra_field);

	http_rsp_field_decode(p_pos, len - (p_pos - p_buf), &p_rsp_header->rsp_field);
	
	return p_pos - p_buf;
}


int http_rsp_field_decode(char *p_buf, int len, HTTP_RSP_FIELD *p_rsp_field)
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

		if (strcasecmp(field_name, "Accept-Ranges") == 0)
		{
			rlt = sscanf(line, "%*[^:]%*[: \t]%[^\r]", p_rsp_field->accept_ranges);
			p_rsp_field->accept_ranges_flag = 1;
		}
		else if (strcasecmp(field_name, "Age") == 0)
		{
			rlt = sscanf(line, "%*[^:]%*[: \t]%[^\r]", p_rsp_field->age);
			p_rsp_field->age_flag = 1;
		}
		else if (strcasecmp(field_name, "ETag") == 0)
		{
			rlt = sscanf(line, "%*[^:]%*[: \t]%[^\r]", p_rsp_field->etag);
			p_rsp_field->etag_flag = 1;
		}
		else if (strcasecmp(field_name, "Location") == 0)
		{
			rlt = sscanf(line, "%*[^:]%*[: \t]%[^\r]", p_rsp_field->location);
			p_rsp_field->location_flag = 1;
		}
		else if (strcasecmp(field_name, "Proxy-Authenticate") == 0)
		{
			rlt = sscanf(line, "%*[^:]%*[: \t]%[^\r]", p_rsp_field->proxy_authenticate);
			p_rsp_field->proxy_authenticate_flag = 1;
		}
		else if (strcasecmp(field_name, "Retry-After") == 0)
		{
			rlt = sscanf(line, "%*[^:]%*[: \t]%[^\r]", p_rsp_field->retry_after);
			p_rsp_field->retry_after_flag= 1;
		}
		else if (strcasecmp(field_name, "Server") == 0)
		{
			rlt = sscanf(line, "%*[^:]%*[: \t]%[^\r]", p_rsp_field->server);
			p_rsp_field->server_flag = 1;
		}
		else if (strcasecmp(field_name, "Vary") == 0)
		{
			rlt = sscanf(line, "%*[^:]%*[: \t]%[^\r]", p_rsp_field->vary);
			p_rsp_field->vary_flag = 1;
		}
		else if (strcasecmp(field_name, "WWW-Authenticate") == 0)
		{
			rlt = sscanf(line, "%*[^:]%*[: \t]%[^\r]", p_rsp_field->www_authenticate);
			p_rsp_field->www_authenticate_flag = 1;
		}
		else
		{
		}
	}	
	
	return p_pos - p_buf;
}


int rsp_status_line_decode(char *p_line, int line_len, HTTP_RSP_HEADER *p_rsp_header)
{
	int rlt;
	
	rlt = sscanf(p_line, "%s %d %s", p_rsp_header->http_version, &p_rsp_header->status_code, p_rsp_header->reason_phrase);

	if (rlt != 3)
	{
		return -1;
	}
	
	return 0;
}

