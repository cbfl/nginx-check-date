/*
 * (C) 2009
	comapre_dates FORMAT $DATE_VAR $RESULT
*/

#include <ngx_config.h>
#include <ngx_core.h> 
#include <ngx_http.h>

typedef struct {
	ngx_str_t	 format;
	ngx_str_t	 variable_name;
} ngx_http_check_date_conf_t;

/* Variable handlers */
static char *ngx_http_check_date_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_check_date_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data);
static ngx_command_t ngx_http_check_date_commands[] = {
	{
		ngx_string("check_date"),
		NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE3,
		ngx_http_check_date_init,
		0,
		0,
		NULL
	},
	ngx_null_command
};

static ngx_http_module_t ngx_http_check_date_module_ctx = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

ngx_module_t ngx_http_check_date_module = {
	NGX_MODULE_V1,
	&ngx_http_check_date_module_ctx, /* module context */
	ngx_http_check_date_commands, /* module directives */
	NGX_HTTP_MODULE, /* module type */
	NULL, /* init master */
	NULL, /* init module */
	NULL, /* init process */
	NULL, /* init thread */
	NULL, /* exit thread */
	NULL, /* exit process */
	NULL, /* exit master */
	NGX_MODULE_V1_PADDING
};

static char * ngx_http_check_date_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
	ngx_str_t *check_date_vars;
	ngx_http_variable_t *resultVariable;
	check_date_vars = cf->args->elts;

	/* TODO some more validations & checks */
	if (check_date_vars[3].data[0] != '$') {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "check_date_module: invalid parameter: \"%s\"", check_date_vars[3].data);
		return NGX_CONF_ERROR;
	}
	check_date_vars[3].len--;
	check_date_vars[3].data++;
	resultVariable = ngx_http_add_variable(cf, &check_date_vars[3], NGX_HTTP_VAR_CHANGEABLE);
	if (resultVariable == NULL) {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "check_date_module: cannot add variable: \"%s\"", check_date_vars[3].data);
		return NGX_CONF_ERROR;
	}
	if (resultVariable->get_handler == NULL ) {
		resultVariable->get_handler = ngx_http_check_date_variable;
 
		ngx_http_check_date_conf_t  *check_date_conf;
		check_date_conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_check_date_conf_t));
		if (check_date_conf == NULL) {
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "check_date_module: allocation failed");
			return NGX_CONF_ERROR;
		}

		check_date_conf->format.len = check_date_vars[1].len;
		check_date_conf->format.data = ngx_palloc(cf->pool, check_date_vars[1].len + 1);
		if (check_date_conf->format.data == NULL) {
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "check_date_module: allocation failed");
			return NGX_CONF_ERROR;
		}
		ngx_cpystrn(check_date_conf->format.data, check_date_vars[1].data, check_date_vars[1].len + 1);

		if (check_date_vars[2].data[0] != '$') {
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "check_date_module: invalid parameter: \"%s\"", check_date_vars[2].data);
			return NGX_CONF_ERROR;
		}
		check_date_vars[2].len--;
		check_date_vars[2].data++;
		check_date_conf->variable_name.len = check_date_vars[2].len;
		check_date_conf->variable_name.data = ngx_palloc(cf->pool, check_date_vars[2].len + 1);
		if (check_date_conf->variable_name.data == NULL) {
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "check_date_module: allocation failed");
			return NGX_CONF_ERROR;
		}
		ngx_cpystrn(check_date_conf->variable_name.data, check_date_vars[2].data, check_date_vars[2].len + 1);

		resultVariable->data = (uintptr_t) check_date_conf;
	}
	return NGX_CONF_OK;
}

static ngx_int_t ngx_http_check_date_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data) {
	ngx_http_check_date_conf_t  *check_date_conf = (ngx_http_check_date_conf_t *) data;

	/* Reset variable */
	v->valid = 0;
	v->not_found = 1;
	if (check_date_conf == NULL) {
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "check_date_module: runtime error \"data\" is NULL");
		return NGX_OK;
	}
	
	// Evaluate date variable
	ngx_http_variable_value_t  *date_variable;
	ngx_uint_t key = ngx_hash_strlow(check_date_conf->variable_name.data, check_date_conf->variable_name.data, check_date_conf->variable_name.len);
	date_variable = ngx_http_get_variable(r, &check_date_conf->variable_name, key);
	if (date_variable == NULL) {
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "check_date_module: invalid variable '$%s'", check_date_conf->variable_name.data);
		return NGX_OK;
	}
	if (date_variable->not_found) {
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "check_date_module: variable not found '%s'", check_date_conf->variable_name.data);
		return NGX_OK;
	}
	if (ngx_strlen(date_variable->data) == 0){
		return NGX_OK;
	}
	
	//Comparison
	struct tm tm;
	memset(&tm, 0, sizeof tm); //focker!
	if ( strptime((const char *) date_variable->data, (const char *) check_date_conf->format.data, &tm) == NULL) {
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "check_date_module: cannot parse date '%s'", date_variable->data);
		return NGX_OK;
	}
	time_t parsed_time = timelocal(&tm);
	time_t local_time = time(NULL);

	// ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "FORMAT : %s", check_date_conf->format.data);
	// ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "DATE : %s", date_variable->data);
	// ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "TIMESTAMP parsed : %i", (int) parsed_time);
	// ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "TIMESTAMP local: %i", local_time);
	// ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "TIMESTAMP interval: %i", local_time - parsed_time);
	// ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "year: %d; month: %d; day: %d;", tm.tm_year, tm.tm_mon, tm.tm_mday);

	ngx_str_t time_interval = ngx_string("");
	time_interval.len = 10;
	time_interval.data = ngx_pnalloc(r->pool, time_interval.len + 1);
	if (time_interval.data == NULL) {
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "check_date_module: allocation failed");
		return NGX_OK;
	}
	ngx_sprintf(time_interval.data, "%i", local_time - parsed_time);
	time_interval.len = ngx_strlen(time_interval.data);

	// Set return value
	v->data = time_interval.data;
	v->len = ngx_strlen( v->data );
	v->valid = 1;
	v->no_cacheable = 0;
	v->not_found = 0;
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "check_date_module: Result: '%s'", v->data);
	return NGX_OK;
}