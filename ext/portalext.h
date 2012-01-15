#ifndef _PORTALR_H
#define _PORTALR_H

#include "pcm.h"
#include "pinlog.h"
#include "pin_errs.h"
#include "ruby.h"

/*
 *  Debug Macros
 */
#ifdef _DEBUG

#define _D_ fprintf(stderr, "%s - %s:%d\n", __FUNCTION__, __FILE__, __LINE__)
#define TRACE(message) fprintf(stderr,"%s - %s:%ld %s\n", __FUNCTION__, __FILE__, __LINE__, message)
#define TRACE_FUNC(name,label,value) fprintf(stderr,"%s - %s:%ld\n  %s %s %s\n", __FUNCTION__, __FILE__, __LINE__, name, label, value)
#define TRACE_INSPECT(label,obj) \
  if (rb_gv_get("$DEBUG")) fprintf(stderr,"%s - %s:%ld\n  %s: %s\n", __FUNCTION__, __FILE__, __LINE__, label, RSTRING(rb_funcall(obj,id_inspect,0))->ptr );

#else
#define _D_
#define TRACE(x)
#define TRACE_FUNC(name,label,value)
#define TRACE_INSPECT(label,obj)
#endif

/*
 *  Returns a char * using to_s
 */
#define RUBY_TO_S(obj) \
	((TYPE(obj) == T_STRING) ?  StringValuePtr(obj) : StringValuePtr(rb_funcall(obj, id_to_s, 0, 0)) )

/*
 * Convert a Ruby value to an int
 */
#define TO_INT(obj) \
	((TYPE(obj) == T_FIXNUM) ? NUM2INT(obj) : NUM2INT(rb_funcall(obj, id_to_i, 0, 0)))

#define TO_BIG(obj) \
	((TYPE(obj) == T_BIGNUM) ? NUM2DBL(obj) : NUM2DBL(rb_funcall(obj, id_to_f, 0, 0)))

/*
 *  Portal Data
 *  This data struture is created for a couple reasons. One exists in each 
 *  instance of the Class. That is created via alloc.
 *
 *  The Connection is instance specific.
 *
 *  TODO: Have the connection be owned by the class in a pooled fashion such that
 *        an instance borrows the connection to perform an op.
 */
struct pdata {
  int ops;
	int errors;
	int in_transaction;
  pcm_context_t *ctxp;
	pin_flist_t *in_flistp;
  pin_flist_t *out_flistp;
  pin_errbuf_t ebuf;
	long owner_id;
};
typedef struct pdata PortalData;

/*
 *  Shortcut for the following:
 *  pobj = Data_Make_Struct(cPortal,PortalData,0,pdata_free,pd);
 */
#define PDataWrap(klass, obj, data) do { \
  obj = Data_Make_Struct(klass, PortalData, 0, pdata_free, data); \
	data->ctxp = NULL; \
	PIN_ERR_CLEAR_ERR(&data->ebuf); \
	data->ops = 0; \
	data->errors = 0; \
	data->in_transaction = 0; \
	data->owner_id = obj; \
} while (0)

#define PDataGet(obj, datap) {\
  Data_Get_Struct(obj, PortalData, datap);\
}

#define PDataClearErr(pd) PIN_ERR_CLEAR_ERR(&pd->ebuf);

/*
 *  Manage the conversion between hash and flist.
 */
typedef struct convert_data {
	pin_fld_num_t fld_num;
	pin_flist_t *flistp;
  pin_errbuf_t *ebufp;
} ConvertData;

#define CDataWrap(klass, obj, data) do { \
  obj = Data_Make_Struct(klass, ConvertData, 0, cdata_free, data); \
} while (0)

#define CDataGet(obj, data_ptr) {\
  Data_Get_Struct(obj, ConvertData, data_ptr);\
}

/*
 *  level is something like PIN_ERR_NOT_FOUND
 *  msg is written to the logfile.
 */
#define PDataSetError(pd, fld, level, msg) {\
	if ((!PIN_ERR_IS_ERR(pd->ebufp)) { \
		pin_set_err(ebufp, PIN_ERRLOC_APP,\
			PIN_ERRCLASS_SYSTEM_DETERMINATE,\
			level, fld, 0, 0);\
		PIN_ERR_LOG_EBUF(PIN_ERR_LEVEL_ERROR, msg, ebufp); \
	}\
}

#endif
