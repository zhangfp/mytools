#include <stdio.h>
#include <string.h>
#include "http_comm.h"

extern int http_general_field_encode(HTTP_GENERAL_FIELD *p_general_field, char *p_buf, int len);
extern int http_entity_field_encode(HTTP_ENTITY_FIELD *p_entity_field, char *p_buf, int len);
extern int http_extra_field_encode(HTTP_EXTRA_FIELD *p_extra_field, char *p_buf, int len);

static int rsp_status_line_encode(HTTP_RSP_HEADER *p_rsp_header, char *p_buf, int len);
static int http_rsp_field_encode(HTTP_RSP_FIELD *p_rsp_field, char *p_buf, int len);


int http_rsp_header_encode(HTTP_RSP_HEADER *p_rsp_header, char *p_buf, int len)
{
	char *p_pos;
	int rlt;
	
	p_pos = p_buf;

	rlt = rsp_status_line_encode(p_rsp_header, p_pos,  len - (p_pos - p_buf));
	if (rlt < 0)
	{
		goto FAILED;
	}
	p_pos += rlt;
	
	rlt = http_general_field_encode(&p_rsp_header->general_field, p_pos,  len - (p_pos - p_buf));
	if (rlt < 0)
	{
		goto FAILED;
	}
	p_pos += rlt;


	rlt = http_rsp_field_encode(&p_rsp_header->rsp_field, p_pos,  len - (p_pos - p_buf));
	if (rlt < 0)
	{
		goto FAILED;
	}
	p_pos += rlt;

	rlt = http_entity_field_encode(&p_rsp_header->entity_field, p_pos,  len - (p_pos - p_buf));
	if (rlt < 0)
	{
		goto FAILED;
	}
	p_pos += rlt;

	rlt = http_extra_field_encode(&p_rsp_header->extra_field, p_pos,  len - (p_pos - p_buf));
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

int rsp_status_line_encode(HTTP_RSP_HEADER *p_rsp_header, char *p_buf, int len)
{
	char *p_pos;
	int rlt;

	p_pos = p_buf;

	if (strlen(p_rsp_header->reason_phrase) > 0)
	{
		rlt = snprintf(p_pos, len, "%s %d %s"CRLF, HTTP_VERSION, p_rsp_header->status_code, p_rsp_header->reason_phrase);
	}
	else
	{
		const char *reason;
		reason = http_code_reason(p_rsp_header->status_code);
		
		rlt = snprintf(p_pos, len, "%s %d %s"CRLF, HTTP_VERSION, p_rsp_header->status_code, reason);
	}

	p_pos += rlt;
	

	return p_pos - p_buf;
}

int http_rsp_field_encode(HTTP_RSP_FIELD *p_rsp_field, char *p_buf, int len)
{
	int rlt;
	char *p_pos;
	
	p_pos = p_buf;

	if (p_rsp_field->location_flag )
	{
		//rlt = sprintf(p_pos, "Location: %s"CRLF, p_rsp_field->location);
		rlt = snprintf(p_pos, len, "Location: %s"CRLF, p_rsp_field->location);

		p_pos += rlt;
	}
	
	return p_pos - p_buf;
}


